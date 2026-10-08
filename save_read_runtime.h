// The sole production admission point is LoadGame's plugin-list CALL, before
// loading flags, Reset, incoming map construction, or game-state deserialization.
struct __declspec(align(8)) SaveReadCounters
{ volatile LONG64 accepted, unsupported, rejected, unexpected; };
static SaveReadCounters s_saveReadCounters = {};
static void SaveReadRuntimeEvent(SaveReadPreflightResult result)
{
    if (result != kSaveReadUnexpectedSeek && result != kSaveReadUnexpectedShortRead) return;
    LONG64 count = InterlockedIncrement64(&s_saveReadCounters.unexpected);
    if (count == 1)
        Log("SR1 snapshot observed an unsupported read/seek contract (event %u); native short-read semantics retained", (UInt32)result);
}
static UInt8 __fastcall SaveReadPreflightGate(UInt8* owner, void*, UInt8* stream)
{
    const SaveReadServices service = {
        (void**)kPerfBSFileVtableAddr, (void*)kArchiveReadCallbackDecodeAddr,
        PerfScratchAllocate, PerfScratchRelease, PerfStreamExtent,
        SaveReadRuntimeEvent, 256u * 1024u * 1024u
    };
    const SaveReadPreflightResult result = SaveReadBindSnapshot(owner, stream, service);
    if (result == kSaveReadAccepted)
        InterlockedIncrement64(&s_saveReadCounters.accepted);
    else if (result == kSaveReadUnsupportedVersion || result == kSaveReadUnsupportedStream ||
        result == kSaveReadUnsupportedCreatedObjects)
    {
        InterlockedIncrement64(&s_saveReadCounters.unsupported);
        if (s_perfCollectCounters) Log("SR1 native fallback: unsupported framing/stream category %u", (UInt32)result);
    }
    else
    {
        InterlockedIncrement64(&s_saveReadCounters.rejected);
        Log("Save load rejected before reset: incomplete or unsupported input acquisition (SR1 category %u)", (UInt32)result);
        return 0; // native 465A8A owns stream/reference-map/local-array cleanup
    }
    return ((UInt8 (__thiscall*)(UInt8*, UInt8*))kSaveReadNativePluginsAddr)(owner, stream);
}
