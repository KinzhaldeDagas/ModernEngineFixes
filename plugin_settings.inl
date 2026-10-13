// MEF-only settings. External fix plugins own their configuration and patches.
static const char kDefaultPluginIni[] =
    "; Modern Engine Fixes: independent fixes only. Restart after changes.\r\n"
    "[Diagnostics]\r\n"
    "bDisableRenderOptimizations=0\r\n"
    "bTraceSaveReads=0\r\n";
static bool ReadPluginSettings(const char* path)
{
    if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)
    {
        HANDLE file = CreateFileA(path, GENERIC_WRITE | DELETE, FILE_SHARE_READ, nullptr,
            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE)
        {
            DWORD written = 0;
            if (!WriteFile(file, kDefaultPluginIni, sizeof(kDefaultPluginIni)-1, &written, nullptr) ||
                written != sizeof(kDefaultPluginIni)-1)
            {
                FILE_DISPOSITION_INFO remove = {TRUE};
                SetFileInformationByHandle(file, FileDispositionInfo, &remove, sizeof(remove));
                Log("could not write default MEF configuration; using defaults");
            }
            CloseHandle(file);
        }
    }
    s_disableRenderOptimizations = GetPrivateProfileIntA("Diagnostics", "bDisableRenderOptimizations", 0, path) != 0;
    s_traceSaveReads = GetPrivateProfileIntA("Diagnostics", "bTraceSaveReads", 0, path) != 0;
    return true;
}
static bool LoadPluginSettings()
{
    char path[MAX_PATH];
    if (!GetModuleSiblingPath(".ini", path, sizeof(path))) return false;
    return ReadPluginSettings(path);
}
