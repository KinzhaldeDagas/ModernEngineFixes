#pragma once
struct PatchInstallServices
{
    mef::PatchTransaction::ProtectFn protect;
    mef::PatchTransaction::FlushFn flush;
    BOOL (WINAPI* retain)(DWORD, LPCSTR, HMODULE*);
    BOOL (WINAPI* release)(HMODULE);
    bool (*validateImage)();
};
static bool ValidateProcessImage()
{ return mef::IsSupportedEngineImage(GetModuleHandleA(nullptr), 0x00400000); }
static const PatchInstallServices kDefaultPatchInstallServices =
    { VirtualProtect, FlushInstructionCache, GetModuleHandleExA, FreeLibrary, ValidateProcessImage };
// OBSE startup only. Never retry individual skipped features during gameplay.
static const char* PatchStateName(PatchGroupState state)
{
    switch (state)
    {
    case PatchGroupState::Pending: return "Pending";
    case PatchGroupState::Disabled: return "Disabled";
    case PatchGroupState::Installed: return "Installed";
    case PatchGroupState::AlreadyInstalledByMEF: return "AlreadyInstalledByMEF";
    case PatchGroupState::CoveredByVerifiedEquivalent: return "CoveredByVerifiedEquivalent";
    case PatchGroupState::DeferredToExternalPolicy: return "DeferredToExternalPolicy";
    case PatchGroupState::SkippedConflict: return "SkippedConflict";
    case PatchGroupState::SkippedDependency: return "SkippedDependency";
    case PatchGroupState::FailedRolledBack: return "FailedRolledBack";
    case PatchGroupState::FatalRetained: return "FatalRetained";
    }
    return "Invalid";
}
static bool PatchGroupEnabled(PatchGroupId group)
{
    if (group == kGroupRenderOptimizations && s_disableRenderOptimizations) return false;
    for (const auto& window : s_patchWriteWindows)
        if (window.group == group) return true;
    return false;
}
static bool PatchGroupActive(PatchGroupState state)
{
    return state == PatchGroupState::Installed || state == PatchGroupState::AlreadyInstalledByMEF ||
        state == PatchGroupState::CoveredByVerifiedEquivalent;
}
static bool CheckPatchRegistry()
{
    for (unsigned i = 0; i < kPatchGroupCount; ++i)
        if (s_patchGroups[i].prerequisite >= (int)i) return false; // Topological order.
    for (unsigned i = 0; i < _countof(s_patchWriteWindows); ++i)
    {
        const auto& left = s_patchWriteWindows[i];
        if (!left.length || left.length > mef::PatchTransaction::MaxBytes ||
            left.address + left.length < left.address) return false;
        for (unsigned j = 0; j < i; ++j)
        {
            const auto& right = s_patchWriteWindows[j];
            if (left.address < right.address + right.length && right.address < left.address + left.length)
            {
                Log("invalid patch registry: %s/%s overlap at %08X/%08X",
                    s_patchGroups[left.group].name, s_patchGroups[right.group].name, left.address, right.address);
                return false;
            }
        }
    }
    return true;
}
static bool CaptureSelectedWriteContracts()
{
    for (unsigned i = 0; i < _countof(s_patchWriteWindows); ++i)
    {
        const auto& window = s_patchWriteWindows[i];
        if (!PatchGroupSelected(window.group)) continue;
        if (!CanReadMemory(window.address, window.length))
        {
            Log("unreadable owned write at %08X length=%u", window.address, window.length);
            return false;
        }
        std::memcpy(s_patchOriginalWrites[i], (void*)window.address, window.length);
        s_patchOriginalCaptured[i] = true;
    }
    return true;
}
static bool CommitSelectedPatchGroup(const PatchInstallServices& services)
{
    PatchGroupResult& result = s_patchResults[s_activePatchGroup];
    ScopedPatchTransaction scope(services.protect, services.flush);
    if (!StageSelectedPatchGroup())
    {
        result.state = PatchGroupState::FailedRolledBack; // Staging has not written memory.
        Log("group %s staging failed: %s", s_patchGroups[s_activePatchGroup].name, scope.transaction.Error());
        return true;
    }
    if (!scope.transaction.WriteCount())
    {
        // Texture thunk proves equivalent coverage. An OFST bypass proves only
        // external policy ownership; it does not provide MEF bounds tracking.
        const bool equivalent =
            (s_activePatchGroup == kGroupTexturePaletteGuard && texturePaletteState == kTexturePalettePatchAlreadyGuarded);
        const bool externalPolicy = s_activePatchGroup == kGroupWorldspaceOffsets &&
            worldspaceOFSTPatchState == kTESWorldSpaceOFSTPatchExternallySkipped;
        result.state = equivalent ? PatchGroupState::CoveredByVerifiedEquivalent :
            externalPolicy ? PatchGroupState::DeferredToExternalPolicy : PatchGroupState::Disabled;
        return true;
    }
    if (!s_patchResidentModule && !services.retain(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCSTR)&CommitSelectedPatchGroup, &s_patchResidentModule))
    {
        Log("initialization failed: cannot retain MEF before first commit");
        result.state = PatchGroupState::FailedRolledBack;
        return false;
    }
    if (!scope.transaction.Commit())
    {
        Log("group %s transaction failed: %s", s_patchGroups[s_activePatchGroup].name, scope.transaction.Error());
        if (scope.transaction.RollbackIncomplete())
        {
            result.state = PatchGroupState::FatalRetained;
            s_patchInstallBlocked = true;
            Log("fatal retained state: bytes, cache or page protection uncertain; stopping all further writes");
            return false;
        }
        result.state = PatchGroupState::FailedRolledBack;
        return true;
    }
    result.state = PatchGroupState::Installed;
    result.writes = scope.transaction.WriteCount();
    return true;
}
static bool InstallPatches(const PatchInstallServices& services = kDefaultPatchInstallServices)
{
    if (s_patchAttempted) return s_patchLoadResult;
    s_patchAttempted = true;
    if (!services.validateImage || !services.validateImage())
    { Log("initialization failed: unsupported executable mapping/PE identity"); return false; }
    if (!CheckPatchRegistry()) { Log("initialization failed: invalid patch ownership registry"); return false; }
    InitializePerformanceRuntime();
    // Preflight against one unmodified-by-MEF startup image. Other groups can
    // deliberately replace native callees described by these ABI contracts.
    // Owned writes are captured here and checked again at staging and commit.
    for (int group = 0; group < kPatchGroupCount; ++group)
    {
        s_activePatchGroup = group;
        auto& result = s_patchResults[group];
        if (!PatchGroupEnabled((PatchGroupId)group)) { result.state = PatchGroupState::Disabled; continue; }
        Log("validating group %s", s_patchGroups[group].name);
        if (!ValidateSelectedPatchGroup() || !CaptureSelectedWriteContracts())
            result.state = PatchGroupState::SkippedConflict;
    }
    bool normal = true;
    for (int group = 0; group < kPatchGroupCount; ++group)
    {
        s_activePatchGroup = group;
        auto& result = s_patchResults[group];
        if (result.state != PatchGroupState::Pending) continue;
        const int dependency = s_patchGroups[group].prerequisite;
        if (!normal || (dependency >= 0 && !PatchGroupActive(s_patchResults[dependency].state)))
        {
            result.state = PatchGroupState::SkippedDependency;
            Log("group %s skipped: %s", s_patchGroups[group].name,
                normal ? s_patchGroups[dependency].name : "installation halted after infrastructure failure");
            continue;
        }
        normal = CommitSelectedPatchGroup(services);
    }
    s_activePatchGroup = -1;
    unsigned installed = 0, covered = 0, disabled = 0, skipped = 0, failed = 0, writes = 0;
    for (unsigned group = 0; group < kPatchGroupCount; ++group)
    {
        const auto& result = s_patchResults[group];
        Log("group %s: %s; committed writes=%u", s_patchGroups[group].name, PatchStateName(result.state), result.writes);
        writes += result.writes;
        switch (result.state)
        {
        case PatchGroupState::Installed: case PatchGroupState::AlreadyInstalledByMEF: ++installed; break;
        case PatchGroupState::CoveredByVerifiedEquivalent: ++covered; break;
        case PatchGroupState::Disabled: ++disabled; break;
        case PatchGroupState::DeferredToExternalPolicy: case PatchGroupState::SkippedConflict: case PatchGroupState::SkippedDependency: ++skipped; break;
        default: ++failed; break;
        }
    }
    if (!writes && !s_patchInstallBlocked && s_patchResidentModule)
    { services.release(s_patchResidentModule); s_patchResidentModule = NULL; }
    Log("MEF installation summary: installed=%u equivalent=%u disabled=%u skipped=%u failed=%u committed writes=%u; load=%s",
        installed, covered, disabled, skipped, failed, writes, normal ? "success" : "failed (see diagnostics)");
    if (s_patchInstallBlocked) Log("active coverage uncertain after incomplete rollback; MEF remains resident");
    else if (!installed) Log("zero MEF patch groups active; verified external coverage=%u", covered);
    s_patchLoadResult = normal;
    return normal;
}
