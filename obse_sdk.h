#pragma once
#include <cstddef>
#include <type_traits>
#include <map>
#include <fstream>
#include "third_party/xobse/common/ITypes.h"
#include "third_party/xobse/obse/obse/PluginAPI.h"
static_assert(sizeof(void*) == 4 && sizeof(UInt32) == 4, "xOBSE x86 ABI");
static_assert(sizeof(OBSEInterface) == 0x30, "pinned xOBSE interface size");
static_assert(offsetof(OBSEInterface, RegisterCommand) == 0x10, "base callback ABI");
static_assert(offsetof(OBSEInterface, GetPluginHandle) == 0x1C, "v15 callback ABI");
static_assert(sizeof(PluginInfo) == 12 && offsetof(PluginInfo, version) == 8, "plugin metadata ABI");
static_assert(sizeof(OBSEMessagingInterface::Message) == 16 && offsetof(OBSEMessagingInterface, RegisterListener) == 4, "messaging ABI");
