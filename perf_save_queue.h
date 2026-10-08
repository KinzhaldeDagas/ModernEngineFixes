// Included in dllmain.cpp's private namespace after performance_fixes.h.
// PERF-20: only LoadGame's stack-owned queue append CALL at 4666F0 is patched.
// The manager-owned queue and the folded HUD append caller retain native code.
struct PerfSaveQueueArray
{
    void* vtable;
    UInt32* data;
    UInt32 capacity, used, occupied, growth;
};
static_assert(sizeof(PerfSaveQueueArray) == 24, "post-load queue ABI");
typedef UInt32 (__thiscall* PerfSaveQueueAppendFn)(PerfSaveQueueArray*, const UInt32*);
struct __declspec(align(8)) PerfSaveQueueCounters
{
    volatile LONG64 calls, reserves, copiedSlots, rejected;
};
static PerfSaveQueueCounters s_perfSaveQueueCounters = {};

// This is an optional optimization bound, not a limit on native queue size.
// Above 4 MiB of pointer storage, the original required-growth path remains.
static const UInt32 kPerfSaveQueueMaxReserveSlots = (4u * 1024u * 1024u) / 4u;
static bool PerfSaveQueueCapacity(UInt32 capacity, UInt32* result)
{
    if (capacity < 50 || capacity >= kPerfSaveQueueMaxReserveSlots ||
        capacity > 0x3FFFFFFFu - 50) return false;
    const UInt32 required = capacity + 50;
    const PerfUInt64 geometric = (PerfUInt64)capacity + (capacity + 1u) / 2u;
    PerfUInt64 rounded = ((geometric + 49u) / 50u) * 50u;
    const UInt32 bound = (kPerfSaveQueueMaxReserveSlots / 50u) * 50u;
    if (rounded > bound) rounded = bound;
    if (rounded <= required || rounded > 0x3FFFFFFFu) return false;
    *result = (UInt32)rounded;
    return true;
}

struct PerfSaveQueueReserveFrame
{
    PerfSaveQueueArray* array;
    PerfSaveQueueReserveFrame* previous;
};
static __declspec(thread) PerfSaveQueueReserveFrame* s_perfSaveQueueReserveFrame;
static bool PerfReserveSaveQueue(PerfSaveQueueArray* array,
    FormHeapAllocFn allocate = (FormHeapAllocFn)kFormHeapAllocAddr,
    FormHeapFreeFn release = (FormHeapFreeFn)kFormHeapFreeAddr)
{
    UInt32 capacity;
    if (!array || array->vtable != (void*)kPerfSaveQueueVtableAddr ||
        !array->data || array->used != array->capacity ||
        array->occupied > array->used || array->growth != 50 ||
        !PerfSaveQueueCapacity(array->capacity, &capacity)) return false;
    // Heap pressure callbacks may reenter. A nested load has its own stack
    // queue and can reserve independently; the same queue takes native growth.
    for (PerfSaveQueueReserveFrame* frame = s_perfSaveQueueReserveFrame;
        frame; frame = frame->previous)
        if (frame->array == array) return false;
    PerfSaveQueueReserveFrame frame = { array, s_perfSaveQueueReserveFrame };
    s_perfSaveQueueReserveFrame = &frame;
    UInt32* pending = NULL;
    __try
    {
        const PerfSaveQueueArray before = *array;
        pending = (UInt32*)allocate(capacity * sizeof(UInt32));
        if (!pending) return false; // native fatal allocation/exception behavior remains
        // Recheck BEFORE reading old slots: an allocation callback can replace and
        // free their storage. No engine calls occur between this check and publish.
        if (std::memcmp(array, &before, sizeof(before))) return false;
        std::memcpy(pending, before.data, before.used * sizeof(UInt32));
        std::memset(pending + before.used, 0, (capacity - before.used) * sizeof(UInt32));
        array->data = pending;
        array->capacity = capacity;
        pending = NULL; // the queue now owns the published allocation
        // Publish first, matching the native ownership at its old-storage free.
        // Reentry from release observes a complete queue and may append normally.
        release(before.data);
        PerfCount(&s_perfSaveQueueCounters.reserves);
        PerfCount(&s_perfSaveQueueCounters.copiedSlots, before.used);
        return true;
    }
    __finally
    {
        // State rejection or an exception before publication owns only pending.
        // Unwind the recursion guard even if the engine release itself escapes.
        __try { if (pending) release(pending); }
        __finally { s_perfSaveQueueReserveFrame = frame.previous; }
    }
}

static UInt32 PerfAppendSaveQueue(PerfSaveQueueArray* array, const UInt32* value,
    PerfSaveQueueAppendFn append = (PerfSaveQueueAppendFn)kPerfSaveQueueAppendAddr,
    FormHeapAllocFn allocate = (FormHeapAllocFn)kFormHeapAllocAddr,
    FormHeapFreeFn release = (FormHeapFreeFn)kFormHeapFreeAddr)
{
    PerfCount(&s_perfSaveQueueCounters.calls);
    if (!PerfReserveSaveQueue(array, allocate, release))
        PerfCount(&s_perfSaveQueueCounters.rejected);
    // Re-read current state inside the original helper after every callback.
    // Native SetSlot advances used even for null records; it owns that policy.
    // The audited caller passes a separate stack local, never an old data slot.
    return append(array, value);
}
static UInt32 __fastcall PerfSaveQueueAppendPatch(PerfSaveQueueArray* array,
    void*, const UInt32* value)
{
    return PerfAppendSaveQueue(array, value);
}
