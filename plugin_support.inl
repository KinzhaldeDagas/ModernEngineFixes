	typedef DWORD (WINAPI* ModuleFileNameFn)(HMODULE, LPSTR, DWORD);

	static bool GetModuleSiblingPath(const char* extension, char* path, UInt32 pathSize,
		ModuleFileNameFn moduleFileName = GetModuleFileNameA)
	{
		if (!path || !pathSize)
			return false;

		path[0] = 0;
		if (!extension || !moduleFileName)
			return false;

		const DWORD length = moduleFileName(s_module, path, pathSize);
		// A return equal to capacity is a truncated filename, even when Windows
		// terminates the partial string. Never derive a log path from that prefix.
		if (!length || length >= pathSize)
		{
			path[0] = 0;
			return false;
		}
		path[length] = 0;

		const char* slash = std::strrchr(path, '\\');
		const char* forwardSlash = std::strrchr(path, '/');
		if (!slash || (forwardSlash && forwardSlash > slash))
			slash = forwardSlash;
		char* dot = std::strrchr(path, '.');
		if (dot && slash && dot < slash)
			dot = NULL;
		const size_t prefixLength = dot ? (size_t)(dot - path) : (size_t)length;
		const size_t extensionLength = std::strlen(extension);
		if (extensionLength >= pathSize - prefixLength)
		{
			path[0] = 0;
			return false;
		}
		std::memcpy(path + prefixLength, extension, extensionLength + 1);

		return true;
	}

	static void OpenLog()
	{
		if (s_log != INVALID_HANDLE_VALUE)
			return;

		char logPath[MAX_PATH] = { 0 };

		if (!GetModuleSiblingPath(".log", logPath, sizeof(logPath)))
		{
			strncpy_s(logPath, sizeof(logPath), "Modern Engine Fixes.log", _TRUNCATE);
		}

		s_log = CreateFileA(logPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}

	static void Log(const char* format, ...)
	{
		OpenLog();
		if (s_log == INVALID_HANDLE_VALUE)
			return;

		char buffer[1024];
		va_list args;
		va_start(args, format);
		int length = _vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
		va_end(args);

		if (length < 0)
			length = (int)std::strlen(buffer);

		DWORD written = 0;
		WriteFile(s_log, buffer, (DWORD)length, &written, NULL);
		WriteFile(s_log, "\r\n", 2, &written, NULL);
		FlushFileBuffers(s_log);
	}

	static bool BytesMatch(UInt32 address, const UInt8* expected, UInt32 length)
	{
		const UInt8* actual = (const UInt8*)address;
		for (UInt32 i = 0; i < length; i++)
		{
			if (actual[i] != expected[i])
				return false;
		}

		return true;
	}

	static bool CanReadMemory(UInt32 address, UInt32 length)
	{
		if (!length)
			return true;

		UInt32 end = address + length;
		if (end < address)
			return false;

		UInt32 current = address;
		while (current < end)
		{
			MEMORY_BASIC_INFORMATION info = { 0 };
			if (!VirtualQuery((const void*)current, &info, sizeof(info)))
				return false;

			if (info.State != MEM_COMMIT)
				return false;

			if (info.Protect & (PAGE_NOACCESS | PAGE_GUARD))
				return false;

			UInt32 regionEnd = (UInt32)info.BaseAddress + (UInt32)info.RegionSize;
			if (regionEnd <= current)
				return false;

			current = regionEnd < end ? regionEnd : end;
		}

		return true;
	}

	static void FormatBytes(const UInt8* bytes, UInt32 length, char* buffer, UInt32 bufferSize)
	{
		if (!bufferSize)
			return;

		buffer[0] = 0;
		UInt32 used = 0;
		for (UInt32 i = 0; i < length && used + 4 < bufferSize; i++)
		{
			int written = _snprintf_s(buffer + used,
				bufferSize - used,
				_TRUNCATE,
				i + 1 < length ? "%02X " : "%02X",
				bytes[i]);

			if (written < 0)
				break;

			used += (UInt32)written;
		}
	}

	#include "patch_groups.h"

	class ScopedPatchTransaction
	{
	public:
		mef::PatchTransaction transaction;
		ScopedPatchTransaction(mef::PatchTransaction::ProtectFn protect = VirtualProtect,
			mef::PatchTransaction::FlushFn flush = FlushInstructionCache) : transaction(protect, flush)
		{ s_patchTransaction = &transaction; }
		~ScopedPatchTransaction() { s_patchTransaction = NULL; }
	};

	static bool WriteRelCall(UInt32 source, UInt32 destination)
	{
		UInt8 replacement[5] = { 0xE8 };
		UInt32 displacement = destination - source - 5;
		std::memcpy(replacement + 1, &displacement, sizeof(displacement));
		return StageSelectedWrite(source, replacement, sizeof(replacement));
	}

	static bool WriteNop(UInt32 address, UInt32 length)
	{
		if (!length)
			return true;

		if (length > mef::PatchTransaction::MaxBytes)
			return false;
		UInt8 replacement[mef::PatchTransaction::MaxBytes];
		std::memset(replacement, 0x90, length);
		return StageSelectedWrite(address, replacement, length);
	}

	static bool WriteRelJump(UInt32 source, UInt32 destination, UInt32 length)
	{
		if (length < 5 || length > mef::PatchTransaction::MaxBytes)
			return false;
		UInt8 replacement[mef::PatchTransaction::MaxBytes];
		std::memset(replacement, 0x90, length);
		replacement[0] = 0xE9;
		UInt32 displacement = destination - source - 5;
		std::memcpy(replacement + 1, &displacement, sizeof(displacement));
		return StageSelectedWrite(source, replacement, length);
	}

	static void LogDecodeBranchCandidate(UInt32 address, const UInt8* actual, UInt32 length, UInt32 first)
	{
		// These are byte-pattern candidates, not instruction-boundary proof or
		// compatibility approval. A CALL may differ only in its displacement.
		if (first >= length) return;
		UInt32 offset = first;
		// Any one of the four rel32 bytes can be the first mismatch. Search
		// the complete operand width, still reporting only a bounded candidate.
		while ((actual[offset] != 0xE8 && actual[offset] != 0xE9) || length - offset < 5)
		{
			if (!offset || first - offset == 4) return;
			--offset;
		}
		if (length - offset < 5) return;
		UInt32 displacement;
		std::memcpy(&displacement, actual + offset + 1, sizeof(displacement));
		UInt32 target = address + offset + 5 + displacement; // x86 rel32 wrap
		Log("  possible %s at %08X targets %08X (byte-pattern candidate)",
			actual[offset] == 0xE8 ? "CALL" : "JMP", address + offset, target);
		HMODULE module = NULL;
		if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)target, &module))
		{
			char path[MAX_PATH] = {};
			DWORD copied = GetModuleFileNameA(module, path, sizeof(path));
			if (copied && copied < sizeof(path))
				Log("  target module %s base=%08X RVA=%08X", path, (UInt32)module, target - (UInt32)module);
			else Log("  target module base=%08X RVA=%08X; path unavailable", (UInt32)module, target - (UInt32)module);
		}
		else
		{
			MEMORY_BASIC_INFORMATION info = {};
			if (VirtualQuery((const void*)target, &info, sizeof(info)))
				Log("  target has no loaded module; allocation=%08X state=%08X type=%08X protect=%08X",
					(UInt32)info.AllocationBase, info.State, info.Type, info.Protect);
			else Log("  target memory information unavailable");
		}
	}
	static bool RequireReadableSignature(const char* name, UInt32 address, UInt32 length)
	{
		if (!CanReadMemory(address, length))
		{
			Log("refusing to patch: unreadable %.160s signature at %08X; length=%u", name, address, length);
			return false;
		}
		return true;
	}
	static bool ValidateDecodeBytes(const char* name, UInt32 address, const UInt8* expected, UInt32 length)
	{
		if (!RequireReadableSignature(name, address, length)) return false;
		const UInt8* actual = (const UInt8*)address;
		struct Difference { UInt32 offset, length; } runs[8] = {};
		UInt32 runCount = 0, differentBytes = 0;
		for (UInt32 i = 0; i < length;)
		{
			if (actual[i] == expected[i]) { ++i; continue; }
			UInt32 start = i;
			do { ++i; ++differentBytes; } while (i < length && actual[i] != expected[i]);
			if (runCount < 8) { runs[runCount].offset = start; runs[runCount].length = i - start; }
			++runCount;
		}
		if (!runCount) return true;
		const UInt32 first = runs[0].offset;
		Log("refusing to patch: unexpected %.160s bytes; range=%08X length=%u; first=%08X offset=+%X expected=%02X actual=%02X; different=%u runs=%u",
			name, address, length, address + first, first, expected[first], actual[first], differentBytes, runCount);
		const UInt32 shown = runCount < 8 ? runCount : 8;
		for (UInt32 r = 0; r < shown; ++r)
		{
			const UInt32 start = runs[r].offset > 8 ? runs[r].offset - 8 : 0;
			const UInt32 count = length - start < 32 ? length - start : 32;
			char expectedText[100], actualText[100];
			FormatBytes(expected + start, count, expectedText, sizeof(expectedText));
			FormatBytes(actual + start, count, actualText, sizeof(actualText));
			Log("  difference %u/%u at %08X offset=+%X changedBytes=%u", r + 1, runCount,
				address + runs[r].offset, runs[r].offset, runs[r].length);
			Log("  expected @ %08X [%s]", address + start, expectedText);
			Log("  actual   @ %08X [%s]", address + start, actualText);
			LogDecodeBranchCandidate(address, actual, length, runs[r].offset);
		}
		if (runCount > shown) Log("  %u additional difference runs omitted", runCount - shown);
		return false;
	}
