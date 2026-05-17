#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <float.h>

namespace
{
	typedef unsigned char UInt8;
	typedef unsigned int UInt32;
	typedef int SInt32;

	static const UInt32 kPluginVersion = 15;
	static const UInt32 kPluginInfoVersion = 2;
	static const UInt32 kOblivionVersion_1_2_0_416 = 0x010201A0;

	static const UInt32 kMainThreadHandlePatchSite = 0x00404A55;
	static const UInt32 kMainThreadHandleContinueSite = 0x00404A68;
	static const UInt32 kBinkOpenThreadHandleUseDecodeAddr = 0x00410229;
	static const UInt32 kBinkOpenResumeThreadUseDecodeAddr = 0x0041028E;

	static const UInt32 kRendererInitFailurePatchSite = 0x004983C0;
	static const UInt32 kRendererInitFailureContinueSite = 0x004983C5;
	static const UInt32 kRendererInitFailureReturnSite = 0x00498E7D;

	static const UInt32 kTexturePaletteUnderscorePatchSite = 0x004A26F7;
	static const UInt32 kTexturePaletteContinueSite = 0x004A26FE;
	static const UInt32 kTexturePaletteFallbackSite = 0x004A27A1;

	static const UInt32 kLoadingTextureWaitPatchSite = 0x004109EB;
	static const UInt32 kLoadingTextureWaitContinueSite = 0x00410A49;
	static const UInt32 kLoadingTextureWaitPumpAddr = 0x00410390;
	static const UInt32 kLoadingTextureWaitSecondsAddr = 0x00B030AC;
	static const float kStaticLoadingScreenWaitScale = 0.70f;

	static const UInt32 kLoadGameStartTickPatchSite = 0x00459A43;
	static const UInt32 kLoadGameStartTickElapsedSite = 0x00459A63;
	static const UInt32 kLoadGameStartTickGlobalAddr = 0x00B33B08;

	static const UInt32 kWorldspaceMessageDelayPatchSite = 0x005BDD35;
	static const UInt32 kWorldspaceMessageDelayContinueSite = 0x005BDD5E;
	static const UInt32 kWorldspaceMessageDelayStep1Addr = 0x005791A0;
	static const UInt32 kWorldspaceMessageDelayStep2Addr = 0x00579220;
	static const UInt32 kWorldspaceMessageDelayInputGlobalsAddr = 0x00B33398;
	static const UInt32 kWorldspaceMessageDelayInputUpdateAddr = 0x0040D4D0;

	static const UInt32 kGlobalAnimTimerLoadPatchSite = 0x004413DB;
	static const UInt32 kGlobalAnimTimerLoadContinueSite = 0x004413E0;
	static const UInt32 kSaveLoadLoadDataAddr = 0x004534D0;
	static const UInt32 kGlobalAnimTimerAddr = 0x00B33A30;
	static const UInt32 kGlobalAnimTimerUpdateDecodeAddr = 0x00442536;
	static const UInt32 kGlobalAnimTimerSceneUpdateDecodeAddr = 0x004425A8;
	static const float kGlobalAnimTimerPrecisionLimit = 131072.0f;

	static const UInt32 kActorAnimRestoreClockWriteSite = 0x00474BB2;
	static const UInt32 kActorAnimUpdatePreSamplePatchSite = 0x00476E86;
	static const UInt32 kActorAnimUpdateClockWriteSite = 0x00476F97;
	static const UInt32 kActorAnimUpdateClockPatchSite = 0x00476FA6;
	static const UInt32 kBSAnimGroupSequenceSampleUpdateAddr = 0x0049F4A0;
	static const UInt32 kBSAnimGroupSequenceSaveStateAddr = 0x0049F570;
	static const UInt32 kBSAnimGroupSequenceLoadStateAddr = 0x0049F5F0;
	static const UInt32 kNiControllerSequenceUpdateAddr = 0x006C5FC0;
	static const UInt32 kActorAnimClockOffset = 0x94;
	static const UInt32 kActorAnimSequenceListOffset = 0xA0;
	static const UInt32 kNiControllerSequenceOffsetOffset = 0x48;
	static const UInt32 kActorAnimSequenceCount = 5;
	static const float kActorAnimRebaseThreshold = 4096.0f;
	static const float kActorAnimRebaseStep = 2048.0f;
	static const float kSentinelMagnitude = 1.0e30f;

	static const UInt32 kActorGetAttackedPatchSite = 0x005E58C0;
	static const UInt32 kActorGetAttackedContinueSite = 0x005E58C5;
	static const UInt32 kActorAdjacentProcessGuardDecodeAddr = 0x005E58D0;
	static const UInt32 kActorIsTalkingPatchSite = 0x005E0E62;
	static const UInt32 kActorIsTalkingContinueSite = 0x005E0E67;
	static const UInt32 kActorIsTalkingReturnSite = 0x005E0E70;
	static const UInt32 kActorIsTalkingAdjacentGuardDecodeAddr = 0x005E0E80;

	static HMODULE s_module = NULL;
	static HANDLE s_log = INVALID_HANDLE_VALUE;
	static bool s_patchInstalled = false;
	static UInt32 s_actorAnimRebaseLogCount = 0;

	enum TexturePalettePatchState
	{
		kTexturePalettePatchMismatch = 0,
		kTexturePalettePatchNeedsInstall,
		kTexturePalettePatchAlreadyGuarded
	};

	enum GlobalAnimTimerLoadPatchState
	{
		kGlobalAnimTimerLoadPatchMismatch = 0,
		kGlobalAnimTimerLoadPatchNeedsInstall,
		kGlobalAnimTimerLoadPatchAlreadyGuarded
	};

	enum RendererInitFailurePatchState
	{
		kRendererInitFailurePatchMismatch = 0,
		kRendererInitFailurePatchNeedsInstall,
		kRendererInitFailurePatchAlreadyGuarded
	};

	enum ActorGetAttackedPatchState
	{
		kActorGetAttackedPatchMismatch = 0,
		kActorGetAttackedPatchNeedsInstall,
		kActorGetAttackedPatchAlreadyGuarded
	};

	enum ActorIsTalkingPatchState
	{
		kActorIsTalkingPatchMismatch = 0,
		kActorIsTalkingPatchNeedsInstall,
		kActorIsTalkingPatchAlreadyGuarded
	};

	static const UInt8 kMainThreadHandleExpectedBytes[] =
	{
		0xFF, 0x15, 0xC4, 0x80, 0xA2, 0x00,
		0x53,
		0x53,
		0x53,
		0x57,
		0x53,
		0x50,
		0x53,
		0xFF, 0x15, 0xC0, 0x80, 0xA2, 0x00
	};

	static const UInt8 kTexturePaletteExpectedBytes[] =
	{
		0x8B, 0xF0,             // mov esi, eax
		0x2B, 0xF3,             // sub esi, ebx
		0x83, 0xC4, 0x08        // add esp, 8
	};

	static const UInt8 kTexturePaletteGuardThunkExpectedBytes[] =
	{
		0x8B, 0xF0,                         // mov esi, eax
		0x2B, 0xF3,                         // sub esi, ebx
		0x83, 0xC4, 0x08,                   // add esp, 8
		0x85, 0xC0,                         // test eax, eax
		0x75, 0x06,                         // jnz +6
		0x68, 0xA1, 0x27, 0x4A, 0x00,       // push 004A27A1h
		0xC3,                               // ret
		0x68, 0xFE, 0x26, 0x4A, 0x00,       // push 004A26FEh
		0xC3                                // ret
	};

	static const UInt8 kLoadingTextureWaitExpectedBytes[] =
	{
		0x8B, 0x35, 0xD0, 0x80, 0xA2, 0x00,
		0xFF, 0xD6,
		0x85, 0xC0,
		0x89, 0x44, 0x24, 0x10,
		0xDB, 0x44, 0x24, 0x10,
		0x7D, 0x06,
		0xD8, 0x05, 0x78, 0xFC, 0xA2, 0x00,
		0xD9, 0x05, 0xAC, 0x30, 0xB0, 0x00,
		0xDC, 0x0D, 0x70, 0xFC, 0xA2, 0x00,
		0xD9, 0x7C, 0x24, 0x08,
		0x0F, 0xB7, 0x44, 0x24, 0x08,
		0xDE, 0xC1,
		0x0D, 0x00, 0x0C, 0x00, 0x00,
		0x89, 0x44, 0x24, 0x10,
		0xD9, 0x6C, 0x24, 0x10,
		0xDF, 0x7C, 0x24, 0x10,
		0x8B, 0x7C, 0x24, 0x10,
		0xD9, 0x6C, 0x24, 0x08,
		0xFF, 0xD6,
		0x3B, 0xF8,
		0x76, 0x0E,
		0x6A, 0x01,
		0xE8, 0x4E, 0xF9, 0xFF, 0xFF,
		0x83, 0xC4, 0x04,
		0x84, 0xC0,
		0x75, 0xEC
	};

	static const UInt8 kLoadGameStartTickExpectedBytes[] =
	{
		0x8B, 0x35, 0xD0, 0x80, 0xA2, 0x00,
		0xFF, 0xD6,
		0xA3, 0x08, 0x3B, 0xB3, 0x00,
		0xFF, 0xD6,
		0x8B, 0x0D, 0x08, 0x3B, 0xB3, 0x00,
		0x81, 0xC1, 0xB8, 0x0B, 0x00, 0x00,
		0x3B, 0xC1,
		0x5E,
		0x76, 0x2B
	};

	static const UInt8 kWorldspaceMessageDelayExpectedBytes[] =
	{
		0xFF, 0xD6,
		0x8D, 0xB8, 0xE8, 0x03, 0x00, 0x00,
		0xFF, 0xD6,
		0x3B, 0xC7,
		0x73, 0x1B,
		0xE8, 0x58, 0xB4, 0xFB, 0xFF,
		0xE8, 0xD3, 0xB4, 0xFB, 0xFF,
		0x8B, 0x0D, 0x98, 0x33, 0xB3, 0x00,
		0xE8, 0x78, 0xF7, 0xE4, 0xFF,
		0xFF, 0xD6,
		0x3B, 0xC7,
		0x72, 0xE5
	};

	static const UInt8 kGlobalAnimTimerLoadExpectedBytes[] =
	{
		0xE8, 0xF0, 0x20, 0x01, 0x00
	};

	static const UInt8 kGlobalAnimTimerUpdateExpectedBytes[] =
	{
		0xD9, 0x05, 0x30, 0x3A, 0xB3, 0x00,
		0x83, 0xC4, 0x08,
		0x85, 0xFF,
		0xD8, 0x44, 0x24, 0x14,
		0x51,
		0xD9, 0x1D, 0x30, 0x3A, 0xB3, 0x00,
		0xD9, 0x05, 0x30, 0x3A, 0xB3, 0x00,
		0xD9, 0x1C, 0x24,
		0x74
	};

	static const UInt8 kGlobalAnimTimerSceneUpdateExpectedBytes[] =
	{
		0xD9, 0x05, 0x30, 0x3A, 0xB3, 0x00,
		0x6A, 0x01,
		0x51,
		0x8B, 0x4E, 0x14,
		0xD9, 0x1C, 0x24,
		0xE8, 0xB4, 0x4D, 0x2C, 0x00
	};

	static const UInt8 kActorAnimClockPatchExpectedBytes[] =
	{
		0x8B, 0xCB,
		0x83, 0xE9, 0x05
	};

	static const UInt8 kActorAnimPreSamplePatchExpectedBytes[] =
	{
		0x33, 0xFF,
		0xEB, 0x06,
		0x8D, 0x9B, 0x00, 0x00, 0x00, 0x00
	};

	static const UInt8 kActorAnimRestoreClockWriteExpectedBytes[] =
	{
		0x66, 0x89, 0x6C, 0x5E, 0x3C,
		0x5F,
		0xD9, 0x9E, 0x94, 0x00, 0x00, 0x00
	};

	static const UInt8 kActorAnimClockWriteExpectedBytes[] =
	{
		0xD9, 0x86, 0x94, 0x00, 0x00, 0x00,
		0xD8, 0x45, 0x0C,
		0xD9, 0x9E, 0x94, 0x00, 0x00, 0x00
	};

	static const UInt8 kBSAnimGroupSequenceSampleUpdateExpectedBytes[] =
	{
		0x8B, 0x41, 0x44,
		0x83, 0xC0, 0xFF,
		0x83, 0xF8, 0x02,
		0x77, 0x1D,
		0xD9, 0x41, 0x48,
		0x6A, 0x01
	};

	static const UInt8 kBSAnimGroupSequenceSaveStateExpectedBytes[] =
	{
		0x56,
		0x8B, 0xF1,
		0xD9, 0x46, 0x48,
		0x8B, 0x0D, 0x00, 0x3B, 0xB3, 0x00,
		0xD8, 0x44, 0x24, 0x08
	};

	static const UInt8 kBSAnimGroupSequenceLoadStateExpectedBytes[] =
	{
		0x51,
		0x56,
		0x57,
		0x6A, 0x04,
		0x8D, 0x44, 0x24, 0x0C,
		0x8B, 0xF1,
		0x8B, 0x0D, 0x00, 0x3B, 0xB3
	};

	static const UInt8 kNiControllerSequenceUpdateExpectedBytes[] =
	{
		0x83, 0xEC, 0x0C,
		0x56,
		0x8B, 0xF1,
		0xD9, 0x46, 0x38,
		0xD9, 0x5C, 0x24, 0x08,
		0xD9, 0x46, 0x34
	};

	static const UInt8 kBinkOpenThreadHandleUseExpectedBytes[] =
	{
		0x74, 0x27,
		0x8B, 0x78, 0x10,
		0xFF, 0x15, 0x8C, 0x80, 0xA2, 0x00,
		0x3B, 0xC7,
		0x74, 0x1A,
		0x8B, 0x15, 0x98, 0x33, 0xB3, 0x00,
		0x8B, 0x42, 0x14,
		0x50,
		0x88, 0x5C, 0x24, 0x17,
		0xFF, 0x15, 0xF4, 0x80, 0xA2
	};

	static const UInt8 kBinkOpenResumeThreadUseExpectedBytes[] =
	{
		0x74, 0x0F,
		0xA1, 0x98, 0x33, 0xB3, 0x00,
		0x8B, 0x48, 0x14,
		0x51,
		0xFF, 0x15, 0xF0, 0x80, 0xA2
	};

	static const UInt8 kRendererInitFailureExpectedBytes[] =
	{
		0x83, 0xCD, 0xFF,
		0x33, 0xDB
	};

	static const UInt8 kActorGetAttackedExpectedBytes[] =
	{
		0x8B, 0x49, 0x58,
		0x8B, 0x01
	};

	static const UInt8 kActorAdjacentProcessGuardExpectedBytes[] =
	{
		0x8B, 0x49, 0x58,
		0x85, 0xC9,
		0x74, 0x0A,
		0x8B, 0x01,
		0x8B, 0x80, 0x9C, 0x03, 0x00, 0x00,
		0xFF, 0xE0
	};

	static const UInt8 kActorIsTalkingExpectedBytes[] =
	{
		0x8B, 0x48, 0x58,
		0x8B, 0x11
	};

	static const UInt8 kActorIsTalkingAdjacentGuardExpectedBytes[] =
	{
		0x83, 0x79, 0x58, 0x00,
		0x74, 0x0D,
		0x8B, 0x49, 0x58,
		0x8B, 0x01,
		0x8B, 0x90, 0x88, 0x03, 0x00, 0x00,
		0xFF, 0xE2,
		0x32, 0xC0,
		0xC3
	};

	struct OBSEInterface
	{
		UInt32 obseVersion;
		UInt32 oblivionVersion;
		UInt32 editorVersion;
		UInt32 isEditor;
		void* RegisterCommand;
		void* SetOpcodeBase;
		void* QueryInterface;
		void* GetPluginHandle;
	};

	struct PluginInfo
	{
		UInt32 infoVersion;
		const char* name;
		UInt32 version;
	};

	static bool GetModuleSiblingPath(const char* extension, char* path, UInt32 pathSize)
	{
		if (!path || !pathSize)
			return false;

		path[0] = 0;
		if (!GetModuleFileNameA(s_module, path, pathSize))
			return false;

		char* dot = std::strrchr(path, '.');
		if (dot)
			strcpy_s(dot, pathSize - (dot - path), extension);
		else
			strncat_s(path, pathSize, extension, _TRUNCATE);

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

	static bool WriteRelCall(UInt32 source, UInt32 destination)
	{
		DWORD oldProtect = 0;
		if (!VirtualProtect((void*)source, 5, PAGE_EXECUTE_READWRITE, &oldProtect))
			return false;

		UInt8* code = (UInt8*)source;
		code[0] = 0xE8;
		*(SInt32*)(code + 1) = (SInt32)(destination - source - 5);

		DWORD ignored = 0;
		VirtualProtect((void*)source, 5, oldProtect, &ignored);
		FlushInstructionCache(GetCurrentProcess(), (void*)source, 5);
		return true;
	}

	static bool WriteNop(UInt32 address, UInt32 length)
	{
		if (!length)
			return true;

		DWORD oldProtect = 0;
		if (!VirtualProtect((void*)address, length, PAGE_EXECUTE_READWRITE, &oldProtect))
			return false;

		UInt8* code = (UInt8*)address;
		for (UInt32 i = 0; i < length; i++)
			code[i] = 0x90;

		DWORD ignored = 0;
		VirtualProtect((void*)address, length, oldProtect, &ignored);
		FlushInstructionCache(GetCurrentProcess(), (void*)address, length);
		return true;
	}

	static bool WriteRelJump(UInt32 source, UInt32 destination, UInt32 length)
	{
		if (length < 5)
			return false;

		DWORD oldProtect = 0;
		if (!VirtualProtect((void*)source, length, PAGE_EXECUTE_READWRITE, &oldProtect))
			return false;

		UInt8* code = (UInt8*)source;
		code[0] = 0xE9;
		*(SInt32*)(code + 1) = (SInt32)(destination - source - 5);

		for (UInt32 i = 5; i < length; i++)
			code[i] = 0x90;

		DWORD ignored = 0;
		VirtualProtect((void*)source, length, oldProtect, &ignored);
		FlushInstructionCache(GetCurrentProcess(), (void*)source, length);
		return true;
	}

	static bool ValidateDecodeBytes(const char* name, UInt32 address, const UInt8* expected, UInt32 length)
	{
		if (BytesMatch(address, expected, length))
			return true;

		char expectedText[512];
		char actualText[512];
		FormatBytes(expected, length, expectedText, sizeof(expectedText));
		FormatBytes((const UInt8*)address, length, actualText, sizeof(actualText));

		Log("refusing to patch: unexpected %s bytes at %08X; expected [%s], actual [%s]",
			name,
			address,
			expectedText,
			actualText);
		return false;
	}

	static TexturePalettePatchState GetTexturePalettePatchState()
	{
		if (BytesMatch(kTexturePaletteUnderscorePatchSite,
				kTexturePaletteExpectedBytes,
				sizeof(kTexturePaletteExpectedBytes)))
		{
			return kTexturePalettePatchNeedsInstall;
		}

		const UInt8* site = (const UInt8*)kTexturePaletteUnderscorePatchSite;
		if (site[0] == 0xE9)
		{
			UInt32 target = kTexturePaletteUnderscorePatchSite + 5 + *(const SInt32*)(site + 1);
			if (CanReadMemory(target, sizeof(kTexturePaletteGuardThunkExpectedBytes)) &&
				BytesMatch(target,
					kTexturePaletteGuardThunkExpectedBytes,
					sizeof(kTexturePaletteGuardThunkExpectedBytes)))
			{
				Log("BSTexturePalette strrchr null-result guard already present at %08X via jump target %08X",
					kTexturePaletteUnderscorePatchSite,
					target);
				return kTexturePalettePatchAlreadyGuarded;
			}
		}

		char expectedText[128];
		char actualText[128];
		FormatBytes(kTexturePaletteExpectedBytes,
			sizeof(kTexturePaletteExpectedBytes),
			expectedText,
			sizeof(expectedText));
		FormatBytes(site,
			sizeof(kTexturePaletteExpectedBytes),
			actualText,
			sizeof(actualText));

		Log("refusing to patch: unexpected BSTexturePalette strrchr guard bytes at %08X; expected [%s], actual [%s]",
			kTexturePaletteUnderscorePatchSite,
			expectedText,
			actualText);
		return kTexturePalettePatchMismatch;
	}

	static bool IsEngineBugFixesGlobalAnimTimerResetHelper(UInt32 helper)
	{
		if (!CanReadMemory(helper, 0xB1))
			return false;

		const UInt8* code = (const UInt8*)helper;
		UInt32 globalTimerPointer = 0;

		for (UInt32 i = 0; i + 7 <= 0x50; i++)
		{
			if (code[i] == 0xA1 && code[i + 5] == 0xD9 && code[i + 6] == 0x00)
			{
				UInt32 pointer = *(const UInt32*)(code + i + 1);
				if (CanReadMemory(pointer, 4) && *(const UInt32*)pointer == kGlobalAnimTimerAddr)
				{
					globalTimerPointer = pointer;
					break;
				}
			}
		}

		if (!globalTimerPointer)
			return false;

		for (UInt32 i = 0; i + 20 <= 0xB1; i++)
		{
			if (code[i] == 0x81 &&
				code[i + 1] == 0xFE &&
				*(const UInt32*)(code + i + 2) == 0x48000000 &&
				code[i + 6] == 0x72 &&
				code[i + 8] == 0xD9 &&
				code[i + 9] == 0xEE &&
				code[i + 10] == 0x8B &&
				code[i + 11] == 0x0D &&
				*(const UInt32*)(code + i + 12) == globalTimerPointer)
			{
				for (UInt32 j = i + 16; j + 1 < 0xB1 && j < i + 36; j++)
				{
					if (code[j] == 0xD9 && code[j + 1] == 0x19)
						return true;
				}
			}
		}

		return false;
	}

	static GlobalAnimTimerLoadPatchState GetGlobalAnimTimerLoadPatchState()
	{
		if (BytesMatch(kGlobalAnimTimerLoadPatchSite,
				kGlobalAnimTimerLoadExpectedBytes,
				sizeof(kGlobalAnimTimerLoadExpectedBytes)))
		{
			return kGlobalAnimTimerLoadPatchNeedsInstall;
		}

		const UInt8* site = (const UInt8*)kGlobalAnimTimerLoadPatchSite;
		if (site[0] == 0xE9)
		{
			UInt32 target = kGlobalAnimTimerLoadPatchSite + 5 + *(const SInt32*)(site + 1);
			if (CanReadMemory(target, 17))
			{
				const UInt8* hook = (const UInt8*)target;
				if (hook[0] == 0xFF &&
					hook[1] == 0x15 &&
					hook[6] == 0xE8 &&
					hook[11] == 0xFF &&
					hook[12] == 0x25)
				{
					UInt32 loadPointer = *(const UInt32*)(hook + 2);
					UInt32 resumePointer = *(const UInt32*)(hook + 13);
					UInt32 resetHelper = target + 11 + *(const SInt32*)(hook + 7);

					if (CanReadMemory(loadPointer, 4) &&
						CanReadMemory(resumePointer, 4) &&
						*(const UInt32*)loadPointer == kSaveLoadLoadDataAddr &&
						*(const UInt32*)resumePointer == kGlobalAnimTimerLoadContinueSite &&
						IsEngineBugFixesGlobalAnimTimerResetHelper(resetHelper))
					{
						Log("global animation timer load precision guard already present at %08X via jump target %08X",
							kGlobalAnimTimerLoadPatchSite,
							target);
						return kGlobalAnimTimerLoadPatchAlreadyGuarded;
					}
				}
			}
		}

		char expectedText[128];
		char actualText[128];
		FormatBytes(kGlobalAnimTimerLoadExpectedBytes,
			sizeof(kGlobalAnimTimerLoadExpectedBytes),
			expectedText,
			sizeof(expectedText));
		FormatBytes(site,
			sizeof(kGlobalAnimTimerLoadExpectedBytes),
			actualText,
			sizeof(actualText));

		Log("refusing to patch: unexpected global animation timer load bytes at %08X; expected [%s], actual [%s]",
			kGlobalAnimTimerLoadPatchSite,
			expectedText,
			actualText);
		return kGlobalAnimTimerLoadPatchMismatch;
	}

	static bool InlineTransferTargets(UInt32 base, const UInt8* code, UInt32 length, UInt32 target)
	{
		for (UInt32 i = 0; i < length; i++)
		{
			if (i + 6 <= length && code[i] == 0xFF && code[i + 1] == 0x25)
			{
				UInt32 pointer = *(const UInt32*)(code + i + 2);
				if (CanReadMemory(pointer, 4) && *(const UInt32*)pointer == target)
					return true;
			}

			if (i + 6 <= length && code[i] == 0x68 && code[i + 5] == 0xC3 &&
				*(const UInt32*)(code + i + 1) == target)
			{
				return true;
			}

			if (i + 5 <= length && code[i] == 0xE9)
			{
				UInt32 destination = base + i + 5 + *(const SInt32*)(code + i + 1);
				if (destination == target)
					return true;
			}
		}

		return false;
	}

	static bool IsExistingRendererInitFailureHook(UInt32 hook)
	{
		static const UInt32 kScanLength = 32;

		if (!CanReadMemory(hook, kScanLength))
			return false;

		const UInt8* code = (const UInt8*)hook;
		if (code[0] != 0x84 || code[1] != 0xC0 || code[2] != 0x75)
			return false;

		bool hasOriginalResumeBytes = false;
		for (UInt32 i = 0; i + sizeof(kRendererInitFailureExpectedBytes) <= kScanLength; i++)
		{
			if (BytesMatch(hook + i,
					kRendererInitFailureExpectedBytes,
					sizeof(kRendererInitFailureExpectedBytes)))
			{
				hasOriginalResumeBytes = true;
				break;
			}
		}

		return hasOriginalResumeBytes &&
			InlineTransferTargets(hook, code, kScanLength, kRendererInitFailureReturnSite) &&
			InlineTransferTargets(hook, code, kScanLength, kRendererInitFailureContinueSite);
	}

	static RendererInitFailurePatchState GetRendererInitFailurePatchState()
	{
		if (BytesMatch(kRendererInitFailurePatchSite,
				kRendererInitFailureExpectedBytes,
				sizeof(kRendererInitFailureExpectedBytes)))
		{
			return kRendererInitFailurePatchNeedsInstall;
		}

		const UInt8* site = (const UInt8*)kRendererInitFailurePatchSite;
		if (site[0] == 0xE9)
		{
			UInt32 target = kRendererInitFailurePatchSite + 5 + *(const SInt32*)(site + 1);
			if (IsExistingRendererInitFailureHook(target))
			{
				Log("renderer initialization failure guard already present at %08X via jump target %08X",
					kRendererInitFailurePatchSite,
					target);
				return kRendererInitFailurePatchAlreadyGuarded;
			}
		}

		char expectedText[128];
		char actualText[128];
		FormatBytes(kRendererInitFailureExpectedBytes,
			sizeof(kRendererInitFailureExpectedBytes),
			expectedText,
			sizeof(expectedText));
		FormatBytes(site,
			sizeof(kRendererInitFailureExpectedBytes),
			actualText,
			sizeof(actualText));

		Log("refusing to patch: unexpected renderer initialization failure guard bytes at %08X; expected [%s], actual [%s]",
			kRendererInitFailurePatchSite,
			expectedText,
			actualText);
		return kRendererInitFailurePatchMismatch;
	}

	static bool IsExistingActorGetAttackedProcessGuard(UInt32 hook)
	{
		static const UInt32 kScanLength = 96;

		if (!CanReadMemory(hook, kScanLength))
			return false;

		const UInt8* code = (const UInt8*)hook;
		bool loadsProcessAndChecksNull =
			code[0] == 0x8B &&
			code[1] == 0x41 &&
			code[2] == 0x58 &&
			code[3] == 0x85 &&
			code[4] == 0xC0 &&
			code[5] == 0x74;

		if (!loadsProcessAndChecksNull)
			return false;

		bool hasOriginalCallSetup = false;
		for (UInt32 i = 0; i + 10 <= kScanLength; i++)
		{
			if (code[i] == 0x8B &&
				code[i + 1] == 0xC8 &&
				code[i + 2] == 0x8B &&
				code[i + 3] == 0x00 &&
				code[i + 4] == 0x8B &&
				code[i + 5] == 0x90 &&
				*(const UInt32*)(code + i + 6) == 0x00000398)
			{
				hasOriginalCallSetup = true;
				break;
			}
		}

		bool returnsFalseOnNull = false;
		for (UInt32 i = 0; i + 3 <= kScanLength; i++)
		{
			if ((code[i] == 0x33 && code[i + 1] == 0xC0 && code[i + 2] == 0xC3) ||
				(code[i] == 0x32 && code[i + 1] == 0xC0 && code[i + 2] == 0xC3))
			{
				returnsFalseOnNull = true;
				break;
			}
		}

		return hasOriginalCallSetup && returnsFalseOnNull;
	}

	static ActorGetAttackedPatchState GetActorGetAttackedPatchState()
	{
		if (BytesMatch(kActorGetAttackedPatchSite,
				kActorGetAttackedExpectedBytes,
				sizeof(kActorGetAttackedExpectedBytes)))
		{
			return kActorGetAttackedPatchNeedsInstall;
		}

		const UInt8* site = (const UInt8*)kActorGetAttackedPatchSite;
		if (site[0] == 0xE9)
		{
			UInt32 target = kActorGetAttackedPatchSite + 5 + *(const SInt32*)(site + 1);
			if (IsExistingActorGetAttackedProcessGuard(target))
			{
				Log("Actor::GetAttacked process null guard already present at %08X via jump target %08X",
					kActorGetAttackedPatchSite,
					target);
				return kActorGetAttackedPatchAlreadyGuarded;
			}
		}

		char expectedText[128];
		char actualText[128];
		FormatBytes(kActorGetAttackedExpectedBytes,
			sizeof(kActorGetAttackedExpectedBytes),
			expectedText,
			sizeof(expectedText));
		FormatBytes(site,
			sizeof(kActorGetAttackedExpectedBytes),
			actualText,
			sizeof(actualText));

		Log("refusing to patch: unexpected Actor::GetAttacked process guard bytes at %08X; expected [%s], actual [%s]",
			kActorGetAttackedPatchSite,
			expectedText,
			actualText);
		return kActorGetAttackedPatchMismatch;
	}

	static bool IsExistingActorIsTalkingProcessGuard(UInt32 hook)
	{
		static const UInt32 kScanLength = 96;

		if (!CanReadMemory(hook, kScanLength))
			return false;

		const UInt8* code = (const UInt8*)hook;
		bool loadsProcessAndChecksNull =
			code[0] == 0x8B &&
			code[1] == 0x48 &&
			code[2] == 0x58 &&
			code[3] == 0x85 &&
			code[4] == 0xC9 &&
			code[5] == 0x74;

		if (!loadsProcessAndChecksNull)
			return false;

		bool hasOriginalCallSetup = false;
		for (UInt32 i = 0; i + 2 <= kScanLength; i++)
		{
			if (code[i] == 0x8B && code[i + 1] == 0x11)
			{
				hasOriginalCallSetup = true;
				break;
			}
		}

		bool returnsFalseOnNull = false;
		for (UInt32 i = 0; i + 2 <= kScanLength; i++)
		{
			if ((code[i] == 0x33 && code[i + 1] == 0xC0) ||
				(code[i] == 0x32 && code[i + 1] == 0xC0))
			{
				returnsFalseOnNull = true;
				break;
			}
		}

		return hasOriginalCallSetup &&
			returnsFalseOnNull &&
			InlineTransferTargets(hook, code, kScanLength, kActorIsTalkingContinueSite) &&
			InlineTransferTargets(hook, code, kScanLength, kActorIsTalkingReturnSite);
	}

	static ActorIsTalkingPatchState GetActorIsTalkingPatchState()
	{
		if (BytesMatch(kActorIsTalkingPatchSite,
				kActorIsTalkingExpectedBytes,
				sizeof(kActorIsTalkingExpectedBytes)))
		{
			return kActorIsTalkingPatchNeedsInstall;
		}

		const UInt8* site = (const UInt8*)kActorIsTalkingPatchSite;
		if (site[0] == 0xE9)
		{
			UInt32 target = kActorIsTalkingPatchSite + 5 + *(const SInt32*)(site + 1);
			if (IsExistingActorIsTalkingProcessGuard(target))
			{
				Log("Actor::IsTalking process null guard already present at %08X via jump target %08X",
					kActorIsTalkingPatchSite,
					target);
				return kActorIsTalkingPatchAlreadyGuarded;
			}
		}

		char expectedText[128];
		char actualText[128];
		FormatBytes(kActorIsTalkingExpectedBytes,
			sizeof(kActorIsTalkingExpectedBytes),
			expectedText,
			sizeof(expectedText));
		FormatBytes(site,
			sizeof(kActorIsTalkingExpectedBytes),
			actualText,
			sizeof(actualText));

		Log("refusing to patch: unexpected Actor::IsTalking process guard bytes at %08X; expected [%s], actual [%s]",
			kActorIsTalkingPatchSite,
			expectedText,
			actualText);
		return kActorIsTalkingPatchMismatch;
	}

	static __declspec(naked) void RendererInitFailurePatch()
	{
		__asm
		{
			test	al, al
			jnz		rendererModeOk

			push	00498E7Dh
			ret

		rendererModeOk:
			or		ebp, 0FFFFFFFFh
			xor		ebx, ebx
			push	004983C5h
			ret
		}
	}

	static __declspec(naked) void ActorGetAttackedProcessGuardPatch()
	{
		__asm
		{
			mov		ecx, [ecx + 58h]
			test	ecx, ecx
			jz		noProcess

			mov		eax, [ecx]
			push	005E58C5h
			ret

		noProcess:
			xor		eax, eax
			ret
		}
	}

	static __declspec(naked) void ActorIsTalkingProcessGuardPatch()
	{
		__asm
		{
			mov		ecx, [eax + 58h]
			test	ecx, ecx
			jz		noProcess

			mov		edx, [ecx]
			push	005E0E67h
			ret

		noProcess:
			xor		al, al
			push	005E0E70h
			ret
		}
	}

	static __declspec(naked) void TexturePaletteUnderscorePatch()
	{
		__asm
		{
			mov		esi, eax
			sub		esi, ebx
			add		esp, 8

			test	eax, eax
			jnz		notNull

			push	004A27A1h
			ret

		notNull:
			push	004A26FEh
			ret
		}
	}

	static __declspec(naked) void LoadGameStartTickPatch()
	{
		__asm
		{
			mov		esi, ds:0A280D0h
			call	esi
			mov		ds:0B33B08h, eax
			mov		ecx, eax
			call	esi
			sub		eax, ecx
			cmp		eax, 0BB8h
			pop		esi
			ja		elapsed
			ret

		elapsed:
			push	00459A63h
			ret
		}
	}

	static void __cdecl DuplicateMainThreadHandle(HANDLE* targetHandle)
	{
		if (!targetHandle)
			return;

		HANDLE duplicated = NULL;
		HANDLE currentProcess = GetCurrentProcess();
		HANDLE currentThread = GetCurrentThread();
		if (DuplicateHandle(currentProcess,
				currentThread,
				currentProcess,
				&duplicated,
				0,
				FALSE,
				DUPLICATE_SAME_ACCESS))
		{
			*targetHandle = duplicated;
		}
		else
		{
			*targetHandle = NULL;
			Log("failed to duplicate main thread handle: %lu", GetLastError());
		}
	}

	static __declspec(naked) void MainThreadHandleDuplicatePatch()
	{
		__asm
		{
			push	edi
			call	DuplicateMainThreadHandle
			add		esp, 4
			push	00404A68h
			ret
		}
	}

	typedef char (__cdecl* PumpWaitInputFn)(char pumpInput);
	typedef void (__cdecl* NoArgFn)();
	typedef void (__thiscall* InputGlobalUpdateFn)(void* inputGlobals);
	typedef void (__thiscall* SaveLoadLoadDataFn)(void* saveLoad, void* dst, UInt32 size);

	static void __cdecl LoadingTextureWaitPatch()
	{
		float seconds = *(float*)kLoadingTextureWaitSecondsAddr * kStaticLoadingScreenWaitScale;

		double milliseconds = (double)seconds * 1000.0;
		if (!(milliseconds > 0.0) || milliseconds >= 2147483647.0)
			return;

		SInt32 duration = (SInt32)milliseconds;
		DWORD start = GetTickCount();
		PumpWaitInputFn pump = (PumpWaitInputFn)kLoadingTextureWaitPumpAddr;

		while ((SInt32)(GetTickCount() - start) < duration)
		{
			if (!pump(1))
				break;
		}
	}

	static void __cdecl WorldspaceMessageDelayPatch()
	{
		NoArgFn step1 = (NoArgFn)kWorldspaceMessageDelayStep1Addr;
		NoArgFn step2 = (NoArgFn)kWorldspaceMessageDelayStep2Addr;
		InputGlobalUpdateFn updateInputGlobals = (InputGlobalUpdateFn)kWorldspaceMessageDelayInputUpdateAddr;
		DWORD start = GetTickCount();

		while ((SInt32)(GetTickCount() - start) < 1000)
		{
			step1();
			step2();
			updateInputGlobals(*(void**)kWorldspaceMessageDelayInputGlobalsAddr);
		}
	}

	static void __cdecl LoadGlobalAnimTimerAndClamp(void* saveLoad, void* dst, UInt32 size)
	{
		((SaveLoadLoadDataFn)kSaveLoadLoadDataAddr)(saveLoad, dst, size);

		if (dst != (void*)kGlobalAnimTimerAddr || size != sizeof(float))
			return;

		float& timer = *(float*)kGlobalAnimTimerAddr;
		if (timer >= kGlobalAnimTimerPrecisionLimit)
		{
			Log("reset loaded global animation timer from %.3f to 0.000", timer);
			timer = 0.0f;
		}
	}

	static __declspec(naked) void GlobalAnimTimerLoadPatch()
	{
		__asm
		{
			mov		eax, [esp + 8]
			mov		edx, [esp + 4]
			push	eax
			push	edx
			push	ecx
			call	LoadGlobalAnimTimerAndClamp
			add		esp, 0Ch
			ret		8
		}
	}

	static bool IsRebasableTime(float value)
	{
		return _finite(value) && value > -kSentinelMagnitude && value < kSentinelMagnitude;
	}

	static float& ActorAnimClock(void* animData)
	{
		return *(float*)((UInt8*)animData + kActorAnimClockOffset);
	}

	static void* ActorAnimSequence(void* animData, UInt32 index)
	{
		return *(void**)((UInt8*)animData + kActorAnimSequenceListOffset + index * sizeof(void*));
	}

	static float& SequenceOffset(void* sequence)
	{
		return *(float*)((UInt8*)sequence + kNiControllerSequenceOffsetOffset);
	}

	static void __cdecl RebaseActorAnimData(void* animData)
	{
		if (!animData)
			return;

		float& clock = ActorAnimClock(animData);
		if (!IsRebasableTime(clock) || clock < kActorAnimRebaseThreshold)
			return;

		UInt32 chunks = (UInt32)(clock / kActorAnimRebaseStep);
		if (!chunks)
			return;

		float shift = (float)chunks * kActorAnimRebaseStep;
		if (shift <= 0.0f || shift > clock)
			return;

		for (UInt32 i = 0; i < kActorAnimSequenceCount; i++)
		{
			void* sequence = ActorAnimSequence(animData, i);
			if (!sequence)
				continue;

			float& offset = SequenceOffset(sequence);
			if (IsRebasableTime(offset))
				offset += shift;
		}

		float oldClock = clock;
		clock -= shift;

		if (s_actorAnimRebaseLogCount < 32)
		{
			Log("rebased ActorAnimData %08X clock from %.3f to %.3f, shifted active sequence offsets by %.3f",
				(UInt32)animData,
				oldClock,
				clock,
				shift);
			s_actorAnimRebaseLogCount++;
		}
	}

	static __declspec(naked) void ActorAnimClockPatch()
	{
		__asm
		{
			pushad
			push	esi
			call	RebaseActorAnimData
			add		esp, 4
			popad

			mov		ecx, ebx
			sub		ecx, 5
			ret
		}
	}

	static __declspec(naked) void ActorAnimPreSamplePatch()
	{
		__asm
		{
			pushad
			push	esi
			call	RebaseActorAnimData
			add		esp, 4
			popad

			xor		edi, edi
			ret
		}
	}

	static bool ValidateDecode()
	{
		return ValidateDecodeBytes("OSGlobals main thread handle duplicate",
				kMainThreadHandlePatchSite,
				kMainThreadHandleExpectedBytes,
				sizeof(kMainThreadHandleExpectedBytes)) &&
			ValidateDecodeBytes("BinkOpen main thread suspend use",
				kBinkOpenThreadHandleUseDecodeAddr,
				kBinkOpenThreadHandleUseExpectedBytes,
				sizeof(kBinkOpenThreadHandleUseExpectedBytes)) &&
			ValidateDecodeBytes("BinkOpen main thread resume use",
				kBinkOpenResumeThreadUseDecodeAddr,
				kBinkOpenResumeThreadUseExpectedBytes,
				sizeof(kBinkOpenResumeThreadUseExpectedBytes)) &&
			ValidateDecodeBytes("loading texture wait",
				kLoadingTextureWaitPatchSite,
				kLoadingTextureWaitExpectedBytes,
				sizeof(kLoadingTextureWaitExpectedBytes)) &&
			ValidateDecodeBytes("load-game start tick",
				kLoadGameStartTickPatchSite,
				kLoadGameStartTickExpectedBytes,
				sizeof(kLoadGameStartTickExpectedBytes)) &&
			ValidateDecodeBytes("worldspace message delay",
				kWorldspaceMessageDelayPatchSite,
				kWorldspaceMessageDelayExpectedBytes,
				sizeof(kWorldspaceMessageDelayExpectedBytes)) &&
			ValidateDecodeBytes("global animation timer update",
				kGlobalAnimTimerUpdateDecodeAddr,
				kGlobalAnimTimerUpdateExpectedBytes,
				sizeof(kGlobalAnimTimerUpdateExpectedBytes)) &&
			ValidateDecodeBytes("global animation timer scene update",
				kGlobalAnimTimerSceneUpdateDecodeAddr,
				kGlobalAnimTimerSceneUpdateExpectedBytes,
				sizeof(kGlobalAnimTimerSceneUpdateExpectedBytes)) &&
			ValidateDecodeBytes("ActorAnimData restore clock write",
				kActorAnimRestoreClockWriteSite,
				kActorAnimRestoreClockWriteExpectedBytes,
				sizeof(kActorAnimRestoreClockWriteExpectedBytes)) &&
			ValidateDecodeBytes("ActorAnimData pre-sample loop",
				kActorAnimUpdatePreSamplePatchSite,
				kActorAnimPreSamplePatchExpectedBytes,
				sizeof(kActorAnimPreSamplePatchExpectedBytes)) &&
			ValidateDecodeBytes("ActorAnimData clock write",
				kActorAnimUpdateClockWriteSite,
				kActorAnimClockWriteExpectedBytes,
				sizeof(kActorAnimClockWriteExpectedBytes)) &&
			ValidateDecodeBytes("ActorAnimData post-clock loop",
				kActorAnimUpdateClockPatchSite,
				kActorAnimClockPatchExpectedBytes,
				sizeof(kActorAnimClockPatchExpectedBytes)) &&
			ValidateDecodeBytes("BSAnimGroupSequence sample update",
				kBSAnimGroupSequenceSampleUpdateAddr,
				kBSAnimGroupSequenceSampleUpdateExpectedBytes,
				sizeof(kBSAnimGroupSequenceSampleUpdateExpectedBytes)) &&
			ValidateDecodeBytes("BSAnimGroupSequence save state",
				kBSAnimGroupSequenceSaveStateAddr,
				kBSAnimGroupSequenceSaveStateExpectedBytes,
				sizeof(kBSAnimGroupSequenceSaveStateExpectedBytes)) &&
			ValidateDecodeBytes("BSAnimGroupSequence load state",
				kBSAnimGroupSequenceLoadStateAddr,
				kBSAnimGroupSequenceLoadStateExpectedBytes,
				sizeof(kBSAnimGroupSequenceLoadStateExpectedBytes)) &&
			ValidateDecodeBytes("NiControllerSequence update",
				kNiControllerSequenceUpdateAddr,
				kNiControllerSequenceUpdateExpectedBytes,
				sizeof(kNiControllerSequenceUpdateExpectedBytes)) &&
			ValidateDecodeBytes("Actor adjacent process guard",
				kActorAdjacentProcessGuardDecodeAddr,
				kActorAdjacentProcessGuardExpectedBytes,
				sizeof(kActorAdjacentProcessGuardExpectedBytes)) &&
			ValidateDecodeBytes("Actor::IsTalking adjacent process guard",
				kActorIsTalkingAdjacentGuardDecodeAddr,
				kActorIsTalkingAdjacentGuardExpectedBytes,
				sizeof(kActorIsTalkingAdjacentGuardExpectedBytes));
	}

	static bool InstallPatches()
	{
		if (s_patchInstalled)
			return true;

		TexturePalettePatchState texturePaletteState = GetTexturePalettePatchState();
		if (texturePaletteState == kTexturePalettePatchMismatch)
			return false;

		GlobalAnimTimerLoadPatchState globalAnimTimerLoadState = GetGlobalAnimTimerLoadPatchState();
		if (globalAnimTimerLoadState == kGlobalAnimTimerLoadPatchMismatch)
			return false;

		RendererInitFailurePatchState rendererInitFailureState = GetRendererInitFailurePatchState();
		if (rendererInitFailureState == kRendererInitFailurePatchMismatch)
			return false;

		ActorGetAttackedPatchState actorGetAttackedState = GetActorGetAttackedPatchState();
		if (actorGetAttackedState == kActorGetAttackedPatchMismatch)
			return false;

		ActorIsTalkingPatchState actorIsTalkingState = GetActorIsTalkingPatchState();
		if (actorIsTalkingState == kActorIsTalkingPatchMismatch)
			return false;

		if (!ValidateDecode())
			return false;

		if (!WriteRelJump(kMainThreadHandlePatchSite, (UInt32)&MainThreadHandleDuplicatePatch, sizeof(kMainThreadHandleExpectedBytes)))
		{
			Log("failed to write OSGlobals main thread handle patch at %08X", kMainThreadHandlePatchSite);
			return false;
		}

		if (texturePaletteState == kTexturePalettePatchNeedsInstall &&
			!WriteRelJump(kTexturePaletteUnderscorePatchSite, (UInt32)&TexturePaletteUnderscorePatch, sizeof(kTexturePaletteExpectedBytes)))
		{
			Log("failed to write relative jump at %08X", kTexturePaletteUnderscorePatchSite);
			return false;
		}

		if (!WriteRelCall(kLoadingTextureWaitPatchSite, (UInt32)&LoadingTextureWaitPatch) ||
			!WriteRelJump(kLoadingTextureWaitPatchSite + 5, kLoadingTextureWaitContinueSite, 5) ||
			!WriteNop(kLoadingTextureWaitPatchSite + 10, sizeof(kLoadingTextureWaitExpectedBytes) - 10))
		{
			Log("failed to write loading texture wait patch at %08X", kLoadingTextureWaitPatchSite);
			return false;
		}

		if (!WriteRelJump(kLoadGameStartTickPatchSite, (UInt32)&LoadGameStartTickPatch, sizeof(kLoadGameStartTickExpectedBytes)))
		{
			Log("failed to write load-game start tick patch at %08X", kLoadGameStartTickPatchSite);
			return false;
		}

		if (!WriteRelCall(kWorldspaceMessageDelayPatchSite, (UInt32)&WorldspaceMessageDelayPatch) ||
			!WriteRelJump(kWorldspaceMessageDelayPatchSite + 5, kWorldspaceMessageDelayContinueSite, 5) ||
			!WriteNop(kWorldspaceMessageDelayPatchSite + 10, sizeof(kWorldspaceMessageDelayExpectedBytes) - 10))
		{
			Log("failed to write worldspace message delay patch at %08X", kWorldspaceMessageDelayPatchSite);
			return false;
		}

		if (globalAnimTimerLoadState == kGlobalAnimTimerLoadPatchNeedsInstall &&
			!WriteRelCall(kGlobalAnimTimerLoadPatchSite, (UInt32)&GlobalAnimTimerLoadPatch))
		{
			Log("failed to write global animation timer load patch at %08X", kGlobalAnimTimerLoadPatchSite);
			return false;
		}

		if (rendererInitFailureState == kRendererInitFailurePatchNeedsInstall &&
			!WriteRelJump(kRendererInitFailurePatchSite,
				(UInt32)&RendererInitFailurePatch,
				sizeof(kRendererInitFailureExpectedBytes)))
		{
			Log("failed to write renderer initialization failure guard at %08X", kRendererInitFailurePatchSite);
			return false;
		}

		if (actorGetAttackedState == kActorGetAttackedPatchNeedsInstall &&
			!WriteRelJump(kActorGetAttackedPatchSite,
				(UInt32)&ActorGetAttackedProcessGuardPatch,
				sizeof(kActorGetAttackedExpectedBytes)))
		{
			Log("failed to write Actor::GetAttacked process null guard at %08X", kActorGetAttackedPatchSite);
			return false;
		}

		if (actorIsTalkingState == kActorIsTalkingPatchNeedsInstall &&
			!WriteRelJump(kActorIsTalkingPatchSite,
				(UInt32)&ActorIsTalkingProcessGuardPatch,
				sizeof(kActorIsTalkingExpectedBytes)))
		{
			Log("failed to write Actor::IsTalking process null guard at %08X", kActorIsTalkingPatchSite);
			return false;
		}

		if (!WriteRelCall(kActorAnimUpdatePreSamplePatchSite, (UInt32)&ActorAnimPreSamplePatch) ||
			!WriteNop(kActorAnimUpdatePreSamplePatchSite + 5, sizeof(kActorAnimPreSamplePatchExpectedBytes) - 5) ||
			!WriteRelCall(kActorAnimUpdateClockPatchSite, (UInt32)&ActorAnimClockPatch))
		{
			Log("failed to write ActorAnimData clock rebase patches at %08X/%08X",
				kActorAnimUpdatePreSamplePatchSite,
				kActorAnimUpdateClockPatchSite);
			return false;
		}

		s_patchInstalled = true;
		Log("installed OSGlobals main thread handle duplicate fix at %08X", kMainThreadHandlePatchSite);
		if (texturePaletteState == kTexturePalettePatchNeedsInstall)
			Log("installed BSTexturePalette strrchr null-result guard at %08X", kTexturePaletteUnderscorePatchSite);
		else
			Log("accepted existing BSTexturePalette strrchr null-result guard at %08X", kTexturePaletteUnderscorePatchSite);
		Log("installed loading texture GetTickCount wrap-safe wait at %08X", kLoadingTextureWaitPatchSite);
		Log("installed load-game start GetTickCount wrap-safe guard at %08X", kLoadGameStartTickPatchSite);
		Log("installed worldspace message GetTickCount wrap-safe delay at %08X", kWorldspaceMessageDelayPatchSite);
		if (globalAnimTimerLoadState == kGlobalAnimTimerLoadPatchNeedsInstall)
			Log("installed global animation timer load precision guard at %08X", kGlobalAnimTimerLoadPatchSite);
		else
			Log("accepted existing global animation timer load precision guard at %08X", kGlobalAnimTimerLoadPatchSite);
		if (rendererInitFailureState == kRendererInitFailurePatchNeedsInstall)
			Log("installed renderer initialization failure guard at %08X", kRendererInitFailurePatchSite);
		else
			Log("accepted existing renderer initialization failure guard at %08X", kRendererInitFailurePatchSite);
		if (actorGetAttackedState == kActorGetAttackedPatchNeedsInstall)
			Log("installed Actor::GetAttacked process null guard at %08X", kActorGetAttackedPatchSite);
		else
			Log("accepted existing Actor::GetAttacked process null guard at %08X", kActorGetAttackedPatchSite);
		if (actorIsTalkingState == kActorIsTalkingPatchNeedsInstall)
			Log("installed Actor::IsTalking process null guard at %08X", kActorIsTalkingPatchSite);
		else
			Log("accepted existing Actor::IsTalking process null guard at %08X", kActorIsTalkingPatchSite);
		Log("installed ActorAnimData clock rebase hooks at %08X and %08X",
			kActorAnimUpdatePreSamplePatchSite,
			kActorAnimUpdateClockPatchSite);
		return true;
	}
}

extern "C"
{
	__declspec(dllexport) bool OBSEPlugin_Query(const OBSEInterface* obse, PluginInfo* info)
	{
		if (info)
		{
			info->infoVersion = kPluginInfoVersion;
			info->name = "Modern Engine Fixes";
			info->version = kPluginVersion;
		}

		Log("Modern Engine Fixes query");

		if (!obse)
		{
			Log("query failed: null OBSE interface");
			return false;
		}

		if (obse->isEditor)
		{
			Log("query failed: editor is not supported");
			return false;
		}

		if (obse->oblivionVersion != kOblivionVersion_1_2_0_416)
		{
			Log("query failed: Oblivion version %08X, expected %08X",
				obse->oblivionVersion,
				kOblivionVersion_1_2_0_416);
			return false;
		}

		return true;
	}

	__declspec(dllexport) bool OBSEPlugin_Load(const OBSEInterface* obse)
	{
		if (!obse || obse->isEditor || obse->oblivionVersion != kOblivionVersion_1_2_0_416)
			return false;

		return InstallPatches();
	}
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		s_module = instance;
		DisableThreadLibraryCalls(instance);
		OpenLog();
		Log("Modern Engine Fixes %u initializing", kPluginVersion);
		Log("static loading screen wait uses %.0f%% of Oblivion's configured duration",
			(double)(kStaticLoadingScreenWaitScale * 100.0f));
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		if (s_log != INVALID_HANDLE_VALUE)
		{
			Log("Modern Engine Fixes shutting down");
			CloseHandle(s_log);
			s_log = INVALID_HANDLE_VALUE;
		}
	}

	return TRUE;
}
