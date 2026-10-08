// Included in dllmain.cpp's private namespace, after performance_fixes.h.
// No persistent density/sortedness cache: each decision observes the native
// operation's current state, after native cleanup or parent detachment.
struct PerfPersistentNode
{
    PerfPersistentNode* next;
    PerfPersistentNode* previous;
    UInt8* payload;
};
struct PerfPersistentList
{
    void* allocatorVtable;
    PerfPersistentNode* head;
    PerfPersistentNode* tail;
    PerfPersistentNode* freeHead;
    UInt32 count;
};
struct PerfRefArray16
{
    void* vtable;
    void** data;
    UInt16 capacity, used, occupied, growth;
};
struct PerfRawArray32
{
    void* vtable;
    void** data;
    UInt32 capacity, used, occupied, growth;
};
static_assert(sizeof(PerfPersistentNode) == 12, "persistent node ABI");
static_assert(sizeof(PerfPersistentList) == 20, "persistent list ABI");
static_assert(sizeof(PerfRefArray16) == 16, "strong array ABI");
static_assert(sizeof(PerfRawArray32) == 24, "raw array ABI");
struct __declspec(align(8)) PerfArraySortCounters
{
    volatile LONG64 orderedChecks, orderedSkips, orderedNodes;
    volatile LONG64 dense16Skips, dense32Skips, holeProbesAvoided;
    volatile LONG64 transferReserves, transferSlots;
};
static PerfArraySortCounters s_perfArraySortCounters = {};

static bool PerfRenderPassKey(UInt8* pass, UInt32 comparator, UInt32* key)
{
    if (!pass) return false;
    if (comparator == kPerfOrderedSelectorCompareAddr)
        *key = *(UInt16*)(pass + 4);
    else
    {
        UInt8* geometry = *(UInt8**)pass;
        if (!geometry) return false;
        *key = *(UInt32*)(geometry + 0xB4);
    }
    return true;
}
static bool __stdcall PerfAlreadyOrderedPasses(PerfPersistentList* list, UInt32 comparator)
{
    // The two native comparators are pure unsigned-key readers returning a
    // SIGNED int. Unknown comparators must not be invoked by this probe.
    if (comparator != kPerfOrderedSelectorCompareAddr &&
        comparator != kPerfOrderedGeometryCompareAddr) return false;
    if (!list || list->freeHead || list->count < 32 || list->count > 0x200000u)
        return false;
    PerfCount(&s_perfArraySortCounters.orderedChecks);
    PerfPersistentNode* const head = list->head;
    PerfPersistentNode* const tail = list->tail;
    const UInt32 count = list->count;
    if (!head || !tail || head->previous || tail->next) return false;
    PerfPersistentNode* previous = NULL;
    PerfPersistentNode* node = head;
    UInt32 previousKey = 0;
    for (UInt32 i = 0; i < count; ++i)
    {
        UInt32 key;
        if (!node || node->previous != previous ||
            !PerfRenderPassKey(node->payload, comparator, &key)) return false;
        if (i && previousKey > key) return false;
        previousKey = key;
        previous = node;
        node = node->next;
    }
    // Bounded shape proof also rejects cycles, stale count/tail, and any
    // observable ownership change. As in native sort, the caller must own a
    // stable list/key view; this adds no synchronization or cross-call state.
    if (node || previous != tail || list->head != head || list->tail != tail ||
        list->count != count || list->freeHead) return false;
    PerfCount(&s_perfArraySortCounters.orderedSkips);
    PerfCount(&s_perfArraySortCounters.orderedNodes, count);
    return true;
}
static __declspec(naked) void PerfOrderedPassSortPatch()
{
    __asm {
        // Native cleanup 7A9C30 has already executed exactly once. Native
        // frame is sub ESP,10h / push ESI; comparator is [ESP+18h].
        pushad
        mov eax, [esp+38h]
        push eax
        push esi
        call PerfAlreadyOrderedPasses
        test al, al
        popad
        jnz finished
        xor eax, eax
        cmp [esi+4], eax
        jz finished
        push kPerfOrderedSortContinue
        ret
    finished:
        xor eax, eax
        push kPerfOrderedSortDone
        ret
    }
}

static bool __stdcall PerfDenseRefArray(PerfRefArray16* array)
{
    // Scope to the audited folded NiAVObject strong-array layout. The check
    // occurs inside AddFirstEmpty, after AddObject's old-parent callbacks.
    if (!array || array->vtable != (void*)kPerfRefArrayVtable ||
        !array->used || !array->data || array->occupied != array->used ||
        array->used > array->capacity || array->used == 0xFFFFu) return false;
    if (array->used == array->capacity && (!array->growth ||
        (UInt32)array->used + array->growth > 0xFFFFu)) return false;
    PerfCount(&s_perfArraySortCounters.dense16Skips);
    PerfCount(&s_perfArraySortCounters.holeProbesAvoided, array->used);
    return true;
}
static bool __stdcall PerfDenseRawArray(PerfRawArray32* array)
{
    // The observed raw caller is the active-file list, not the save-ID arrays
    // intercepted by PERF-3/4. Keep other folded layouts on their native path.
    if (!array || array != (PerfRawArray32*)kPerfActiveFormArrayAddr ||
        array->vtable != (void*)kPerfRawArrayVtable || !array->used ||
        !array->data || array->occupied != array->used ||
        array->used > array->capacity || array->capacity > 0x3FFFFFFEu)
        return false;
    if (array->used == array->capacity && (!array->growth ||
        array->growth > 0x3FFFFFFEu - array->used)) return false;
    PerfCount(&s_perfArraySortCounters.dense32Skips);
    PerfCount(&s_perfArraySortCounters.holeProbesAvoided, array->used);
    return true;
}
static __declspec(naked) void PerfDenseRefArrayPatch()
{
    __asm {
        pushad
        push esi
        call PerfDenseRefArray
        test al, al
        popad
        jz original
        movzx edi, word ptr [esi+0Ah]
        xor eax, eax
        push kPerfDenseRefAppend
        ret
    original:
        movzx edi, word ptr [esi+0Ah]
        xor eax, eax
        push kPerfDenseRefContinue
        ret
    }
}
static __declspec(naked) void PerfDenseRawArrayPatch()
{
    __asm {
        pushad
        push esi
        call PerfDenseRawArray
        test al, al
        popad
        jz original
        mov edi, [esi+0Ch]
        xor eax, eax
        push kPerfDenseRawAppend
        ret
    original:
        mov edi, [esi+0Ch]
        xor eax, eax
        push kPerfDenseRawContinue
        ret
    }
}

static bool PerfReserveFreshTransfer(PerfRefArray16* destination,
    const PerfRefArray16* source,
    FormHeapAllocFn allocate = (FormHeapAllocFn)kFormHeapAllocAddr,
    FormHeapFreeFn release = (FormHeapFreeFn)kFormHeapFreeAddr)
{
    if (!destination || !source || destination == source ||
        destination->vtable != (void*)kPerfRefArrayVtable ||
        source->vtable != (void*)kPerfRefArrayVtable ||
        destination->data || destination->capacity || destination->used ||
        destination->occupied || destination->growth != 1 ||
        source->used < 32 || source->used > source->capacity ||
        source->occupied > source->used || !source->data) return false;
    const PerfRefArray16 before = *destination;
    const PerfRefArray16 sourceBefore = *source;
    const UInt32 count = sourceBefore.used;
    // Native allocations store capacity in a 4-byte cookie before zeroed
    // pointer slots. Zero construction has no callbacks or reference work.
    // Never call Resize16 for optional reserve: it publishes before success.
    UInt32* const block = (UInt32*)allocate(4 + count * 4);
    if (!block) return false; // allocator aborts/exceptions remain native
    block[0] = count;
    std::memset(block + 1, 0, count * 4);
    // Allocation can reenter. Do not overwrite a changed or nested-reserved
    // destination, and use the native loop on a changed source estimate.
    if (std::memcmp(destination, &before, sizeof(before)) ||
        std::memcmp(source, &sourceBefore, sizeof(sourceBefore)))
    {
        release(block);
        return false;
    }
    destination->data = (void**)(block + 1);
    destination->capacity = (UInt16)count;
    PerfCount(&s_perfArraySortCounters.transferReserves);
    PerfCount(&s_perfArraySortCounters.transferSlots, count);
    return true;
}
static void __stdcall PerfReserveFreshTransferNodes(UInt8* destination, UInt8* source)
{
    PerfReserveFreshTransfer((PerfRefArray16*)(destination + 0xAC),
        (const PerfRefArray16*)(source + 0xAC));
}
static __declspec(naked) void PerfFreshTransferReservePatch()
{
    __asm {
        pushfd
        pushad
        push ebx
        push ebp
        call PerfReserveFreshTransferNodes
        popad
        popfd
        // Replay the source extent read AFTER optional allocator callbacks.
        movzx eax, word ptr [ebx+0B6h]
        push kPerfFreshTransferContinue
        ret
    }
}
