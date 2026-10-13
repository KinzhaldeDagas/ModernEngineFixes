#pragma once
#include <windows.h>
#include "obse_sdk.h"
namespace mef {
bool Query(const OBSEInterface* obse, PluginInfo* info);
bool Load(const OBSEInterface* obse);
void Attach(HINSTANCE instance);
void ThreadDetach();
void ProcessDetach();
}
