// Modern Engine Fixes: x86 entry points. Build in an x86 VS developer prompt:
// cl /nologo /LD /O2 /MT /EHsc /std:c++20 /Ithird_party/xobse/obse dllmain.cpp plugin_runtime.cpp /link /OUT:"Modern Engine Fixes.dll"
#include "plugin_runtime.h"
#pragma comment(linker, "/EXPORT:OBSEPlugin_Query=_OBSEPlugin_Query")
#pragma comment(linker, "/EXPORT:OBSEPlugin_Load=_OBSEPlugin_Load")
extern "C" bool OBSEPlugin_Query(const OBSEInterface* obse, PluginInfo* info)
{ return mef::Query(obse, info); }
extern "C" bool OBSEPlugin_Load(const OBSEInterface* obse)
{ return mef::Load(obse); }
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    // /MT requires thread notifications. Keep only module bookkeeping and
    // existing per-thread resource disposal here; no initialization/configuration I/O.
    if (reason == DLL_PROCESS_ATTACH) mef::Attach(instance);
    else if (reason == DLL_THREAD_DETACH) mef::ThreadDetach();
    else if (reason == DLL_PROCESS_DETACH) mef::ProcessDetach();
    return TRUE;
}
