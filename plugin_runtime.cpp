#if !defined(_M_IX86)
#error Modern Engine Fixes requires an x86 build.
#endif
#pragma comment(lib, "user32.lib")

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdarg>
#include <cstdlib>
#include <cerrno>
#include <new>
#include <memory>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <float.h>
#include "patch_transaction.h"
#include "plugin_runtime.h"
#include "save_cursor.h"
#include "executable_identity.h"

class TESSaveLoadGame; // opaque; layout access is isolated in save_cursor.h

namespace
{
	typedef char (__thiscall* TESFileNextRecordFn)(UInt8* file);
	typedef void* (__cdecl* FormHeapAllocFn)(UInt32 size);
	typedef void (__cdecl* FormHeapFreeFn)(void* ptr);
	typedef char (__thiscall* TESFileGetChunkDataFn)(UInt8* file, char* dst, UInt32 maxSize);
	typedef void* (__thiscall* TESFormLoadCurrentSaveGameFn)(void* form, void* dst, UInt32 size);
	typedef void (__cdecl* PrintErrorFn)(const char* message);
	typedef int (__cdecl* ZlibStreamEndFn)(void* stream);

	static const UInt32 kPluginVersion = 1 * 10000 + 0 * 100 + 3;
	static const char kPluginVersionString[] = "1.0.3";

	#include "engine_addresses.inl"
	#include "runtime_state.inl"
	#include "engine_signatures.inl"
	#include "plugin_support.inl"
	#include "save_buffers.inl"
	#include "archive_fixes.inl"
	#include "compatibility.inl"
	#include "worldspace_parser.inl"
	#include "timing_fixes.inl"
	#include "plugin_settings.inl"
	#include "patch_plan.inl"

}

namespace {
static bool s_shutdownReported = false;
static void ReportShutdown()
{
    if (s_shutdownReported || s_log == INVALID_HANDLE_VALUE) return;
    s_shutdownReported = true;
    LogPerformanceCounters();
    LogExtendedPerformanceCounters();
    Log("Modern Engine Fixes shutting down");
}
static void OnOBSEMessage(OBSEMessagingInterface::Message* message)
{
    if (message && (message->type == OBSEMessagingInterface::kMessage_ExitGame ||
        message->type == OBSEMessagingInterface::kMessage_ExitGame_Console)) ReportShutdown();
}
static bool RegisterShutdownListener(const OBSEInterface* obse)
{
    // Optional v17 API. Never read later SDK fields on an older host, and never
    // make messaging availability a new requirement for installing fixes.
    if (obse->obseVersion < 17 || !obse->QueryInterface || !obse->GetPluginHandle) return false;
    auto* messaging = static_cast<OBSEMessagingInterface*>(obse->QueryInterface(kInterface_Messaging));
    if (!messaging || messaging->version < OBSEMessagingInterface::kVersion || !messaging->RegisterListener) return false;
    const PluginHandle handle = obse->GetPluginHandle(); // borrowed opaque ID, valid during Load
    return handle != kPluginHandle_Invalid && messaging->RegisterListener(handle, "OBSE", OnOBSEMessage);
}
void InitializePlugin()
{
    static bool initialized = false;
    if (initialized) return;
    initialized = true; // OBSE serializes Query/Load at startup.
    char counters[4] = {};
    s_perfCollectCounters = GetEnvironmentVariableA("MEF_PERF_COUNTERS", counters, sizeof(counters)) == 1 && counters[0] == '1';
    Log("Modern Engine Fixes v%s initializing", kPluginVersionString);
    Log("static loading screen wait uses %.0f%% of Oblivion's configured duration", (double)(kStaticLoadingScreenWaitScale * 100.0f));
}
}

namespace mef
{
	bool Query(const OBSEInterface* obse, PluginInfo* info)
	{
		if (info)
		{
			info->infoVersion = kPluginInfoVersion;
			info->name = "Modern Engine Fixes";
			info->version = kPluginVersion;
		}

		InitializePlugin();
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

	bool Load(const OBSEInterface* obse)
	{
		if (!obse || obse->isEditor || obse->oblivionVersion != kOblivionVersion_1_2_0_416)
			return false;

		InitializePlugin();
		if (s_patchAttempted)
			return s_patchLoadResult;
		try
		{
			if (!LoadPluginSettings()) return false;
		}
		catch (const std::bad_alloc&)
		{
			Log("configuration allocation failed");
			return false;
		}
		const bool loaded = InstallPatches();
		if (loaded) RegisterShutdownListener(obse);
		return loaded;
	}
}


namespace mef {
void Attach(HINSTANCE instance) { s_module = instance; }
void ThreadDetach() { ReleasePerformanceThreadResources(); }
void ProcessDetach()
{
    // Modern OBSE reports counters on its exit message outside the loader lock.
    // Preserve the old diagnostic fallback for older hosts or nonstandard exit.
    // No engine teardown, initialization or configuration work occurs here.
    ReportShutdown();
    if (s_log != INVALID_HANDLE_VALUE) { CloseHandle(s_log); s_log = INVALID_HANDLE_VALUE; }
}
}
