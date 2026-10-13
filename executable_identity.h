#pragma once
#include <windows.h>
#include <cstdint>

namespace mef {
// OBSE supplies the runtime version; each group supplies its exact byte gates.
// This structural gate additionally rejects a different executable/mapping.
// Do not pin a timestamp/checksum: supported 1.2.0.416 distributions and LAA
// headers can differ. This is not a cryptographic executable allowlist.
inline bool IsSupportedEngineImage(const void* image, std::uintptr_t requiredMapping)
{
    if (!image || reinterpret_cast<std::uintptr_t>(image) != requiredMapping) return false;
    IMAGE_DOS_HEADER dos = {};
    SIZE_T copied = 0;
    if (!ReadProcessMemory(GetCurrentProcess(), image, &dos, sizeof(dos), &copied) ||
        copied != sizeof(dos) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
        dos.e_lfanew < static_cast<LONG>(sizeof(dos)) || dos.e_lfanew > 0x100000) return false;
    const auto base = reinterpret_cast<std::uintptr_t>(image);
    if (base + static_cast<std::uintptr_t>(dos.e_lfanew) < base) return false;
    IMAGE_NT_HEADERS32 nt = {};
    if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(base + dos.e_lfanew),
        &nt, sizeof(nt), &copied) || copied != sizeof(nt)) return false;
    return nt.Signature == IMAGE_NT_SIGNATURE &&
        nt.FileHeader.Machine == IMAGE_FILE_MACHINE_I386 &&
        !(nt.FileHeader.Characteristics & IMAGE_FILE_DLL) &&
        nt.FileHeader.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32) &&
        nt.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
        nt.OptionalHeader.ImageBase == 0x00400000 &&
        nt.OptionalHeader.SizeOfImage >= 0x007AF000 &&
        nt.OptionalHeader.SizeOfImage <= 0x10000000 &&
        nt.OptionalHeader.AddressOfEntryPoint < nt.OptionalHeader.SizeOfImage &&
        nt.OptionalHeader.SizeOfHeaders >= static_cast<DWORD>(dos.e_lfanew) + sizeof(nt) &&
        nt.OptionalHeader.SizeOfHeaders <= nt.OptionalHeader.SizeOfImage;
}
}
