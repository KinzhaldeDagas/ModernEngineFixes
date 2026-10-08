// Included inside dllmain.cpp's private namespace. Engine addresses and signature
// declarations stay in dllmain.cpp so both verifiers see the production plan.
typedef unsigned __int64 PerfUInt64;
struct __declspec(align(8)) PerfCounters
{
    volatile LONG64 sortCalls, sortReused, sortHeapOpsAvoided;
    volatile LONG64 trimCalls, trimFast, trimRemoved, trimFallbacks;
    volatile LONG64 lightCalls, lightScores, lightPrefixSkipped, lightFallbacks;
    volatile LONG64 idCalls, idHits, idAppends, idFallbacks, idRebuilds;
    volatile LONG64 reserveCalls, reserveFast, reserveFallbacks;
    volatile LONG64 masterCalls, masterIndexed, masterComparisons;
};
static PerfCounters s_perfCounters = {};
static bool s_perfCollectCounters = false;
static void PerfCount(volatile LONG64* value, PerfUInt64 amount = 1)
{
    if (!s_perfCollectCounters) return;
    InterlockedExchangeAdd64(value, (LONG64)amount);
}
static void* __cdecl PerfScratchAllocate(UInt32 size)
{
    return size ? HeapAlloc(GetProcessHeap(), 0, size) : NULL;
}
static void __cdecl PerfScratchRelease(void* p)
{
    if (p) HeapFree(GetProcessHeap(), 0, p);
}
static UInt32 PerfHash(UInt32 key)
{
    key ^= key >> 16; key *= 0x7FEB352Du;
    key ^= key >> 15; key *= 0x846CA68Bu;
    return key ^ (key >> 16);
}
static bool PerfTableCapacity(UInt32 count, UInt32* capacity)
{
    // Private optimization storage only. Larger/unsupported operations retain
    // the original engine path; this is not a cap on engine data.
    if (count > 0x200000u) return false;
    UInt32 result = 128;
    while (result < count * 2) result *= 2;
    *capacity = result;
    return true;
}
struct PerfListNode { void* data; PerfListNode* next; };
static_assert(sizeof(PerfListNode) == 8, "BSSimpleList node ABI");

static bool __stdcall PerfReuseSortedNodes(PerfListNode* head, void** sorted, UInt32 count)
{
    PerfCount(&s_perfCounters.sortCalls);
    if (!head || (!sorted && count)) return false;
    if (!count) return !head->data && !head->next;
    PerfListNode* node = head;
    for (UInt32 i = 0; i < count; ++i)
    {
        if (!node || !node->data || !sorted[i]) return false;
        node = node->next;
    }
    if (node) return false;
    node = head;
    for (UInt32 i = 0; i < count; ++i, node = node->next) node->data = sorted[i];
    PerfCount(&s_perfCounters.sortReused);
    PerfCount(&s_perfCounters.sortHeapOpsAvoided, 2ull * (count - 1));
    return true;
}
static __declspec(naked) void PerfSortReusePatch()
{
    __asm {
        pushad
        push ebx
        push dword ptr [ebp-8]
        push esi
        call PerfReuseSortedNodes
        test al, al
        popad
        jz original
        push kPerfSortDoneSite
        ret
    original:
        cmp dword ptr [esi+4], 0
        jz emptyTail
        push kPerfSortRebuildNodeSite
        ret
    emptyTail:
        push kPerfSortRebuildHeadSite
        ret
    }
}

static bool PerfTryTrimCandidates(PerfListNode* head, UInt32 limit,
    FormHeapAllocFn scratch = PerfScratchAllocate,
    FormHeapFreeFn freeScratch = PerfScratchRelease,
    FormHeapFreeFn freeNode = (FormHeapFreeFn)kFormHeapFreeAddr)
{
    PerfCount(&s_perfCounters.trimCalls);
    if (!head) return false;
    UInt32 count = 0, nodes = 0;
    PerfListNode* fast = head;
    for (PerfListNode* node = head; node; node = node->next)
    {
        if (++nodes == 0) return false;
        if (node->data) ++count;
        fast = fast && fast->next ? fast->next->next : NULL;
        if (fast && fast == node->next) return false;
    }
    if (count <= limit) return true;
    if (nodes != count) return false;
    UInt32 capacity;
    if (!PerfTableCapacity(count, &capacity)) return false;
    UInt32* keys = (UInt32*)scratch(capacity * sizeof(UInt32));
    if (!keys) return false;
    std::memset(keys, 0, capacity * sizeof(UInt32));
    // Re-traverse the live chain after scratch allocation; never keep a stale
    // split point across a callback supplied by an alternate allocator.
    count = 0;
    PerfListNode* split = NULL;
    bool valid = true;
    for (PerfListNode* node = head; node; node = node->next)
    {
        if (!node->data || count >= capacity / 2) { valid = false; break; }
        UInt32 key = (UInt32)node->data, slot = PerfHash(key) & (capacity - 1);
        while (keys[slot] && keys[slot] != key) slot = (slot + 1) & (capacity - 1);
        if (keys[slot]) { valid = false; break; } // Duplicate/cycle: stock semantics.
        keys[slot] = key;
        if (++count == limit) split = node;
    }
    if (!valid) { freeScratch(keys); return false; }
    if (count <= limit) { freeScratch(keys); return true; }
    PerfListNode* detached;
    if (!limit)
    {
        detached = head->next;
        head->data = NULL; head->next = NULL;
    }
    else
    {
        detached = split->next;
        split->next = NULL;
    }
    // Detached nodes are private now. Never overwrite the surviving chain after
    // a free callback; payloads are borrowed actors and are never destroyed here.
    while (detached)
    {
        PerfListNode* next = detached->next;
        freeNode(detached); detached = next;
    }
    freeScratch(keys);
    PerfCount(&s_perfCounters.trimFast);
    PerfCount(&s_perfCounters.trimRemoved, count - limit);
    return true;
}
static bool __stdcall PerfTrimHead(PerfListNode* head)
{
    bool result = PerfTryTrimCandidates(head, *(volatile UInt32*)kPerfCandidateLimitAddr);
    if (!result) PerfCount(&s_perfCounters.trimFallbacks);
    return result;
}
static PerfListNode* __stdcall PerfTrimManager(UInt8* manager)
{
    PerfListNode* head = (PerfListNode*)(manager + 0x60);
    if (!PerfTrimHead(head))
        return ((PerfListNode* (__thiscall*)(UInt8*))kPerfNativeTrimAddr)(manager);
    return head;
}
static __declspec(naked) void PerfShadowTrimPatch()
{
    __asm { push ecx }
    __asm { call PerfTrimManager }
    __asm { ret }
}
static __declspec(naked) void PerfPeriodicTrimPatch()
{
    __asm {
        pushad
        push esi
        call PerfTrimHead
        test al, al
        popad
        jz original
        xor eax, eax
        push kPerfPeriodicTrimDoneSite
        ret
    original:
        mov eax, esi
        xor ecx, ecx
        test eax, eax
        push kPerfPeriodicTrimContinue
        ret
    }
}

struct PerfRefNode { PerfRefNode* next; PerfRefNode* prev; UInt8* light; };
struct PerfRefList { void* vtable; PerfRefNode* head; PerfRefNode* tail; UInt32 count; };
static_assert(sizeof(PerfRefList) == 16 && sizeof(PerfRefNode) == 12, "NiT reference-list layout");
typedef float (__thiscall* PerfLightScoreFn)(UInt8*, void*);
typedef void* (__thiscall* PerfRemovePositionFn)(PerfRefList*, UInt8**, PerfRefNode**);
typedef PerfRefNode* (__thiscall* PerfInsertBeforeFn)(PerfRefList*, PerfRefNode*, UInt8**);
typedef void (__cdecl* PerfReferenceFn)(UInt8*);
static void __cdecl PerfAddReference(UInt8* object) { InterlockedIncrement((volatile LONG*)(object + 4)); }
static void __cdecl PerfReleaseReference(UInt8* object)
{
    if (!InterlockedDecrement((volatile LONG*)(object + 4)))
        (*(void (__thiscall**)(UInt8*, UInt32))*(void**)object)(object, 1);
}
static bool PerfFiniteScore(float value)
{
    UInt32 bits; std::memcpy(&bits, &value, sizeof(bits));
    return (bits & 0x7F800000u) != 0x7F800000u;
}
static bool PerfScoreGreater(float left, float right)
{
    UInt8 result;
    __asm {
        fld right
        fld left
        fcompp
        fnstsw ax
        test ah, 41h
        setz result
    }
    return result != 0;
}
static __declspec(thread) PerfUInt64 s_perfLightEpoch = 0;
static bool PerfRankLights(UInt8* owner, void* camera,
    PerfLightScoreFn score = (PerfLightScoreFn)kPerfLightScoreAddr,
    PerfRemovePositionFn remove = (PerfRemovePositionFn)kPerfLightRemoveAddr,
    PerfInsertBeforeFn insert = (PerfInsertBeforeFn)kPerfLightInsertAddr,
    PerfReferenceFn addReference = PerfAddReference,
    PerfReferenceFn releaseReference = PerfReleaseReference)
{
    const PerfUInt64 epoch = ++s_perfLightEpoch; // Includes unsupported/nested calls.
    PerfCount(&s_perfCounters.lightCalls);
    if (!owner || !camera) return false;
    PerfRefList* list = (PerfRefList*)(owner + 0xE4);
    if (list->vtable != (void*)kPerfNativeListVtable) return false;
    // The native routine captures the original head before the special score.
    PerfRefNode* current = list->head;
    score(*(UInt8**)(owner + 0x118), camera);
    bool eligible = true, havePrevious = false;
    float previous = 0;
    PerfUInt64 processed = 0, skipped = 0, scoreCalls = 1;
    while (current)
    {
        PerfRefNode* nextOuter = current->next;
        float currentScore = score(current->light, camera);
        ++scoreCalls;
        eligible = eligible && epoch == s_perfLightEpoch && PerfFiniteScore(currentScore);
        if (eligible && havePrevious && PerfScoreGreater(currentScore, previous)) eligible = false;
        if (eligible)
        {
            skipped += processed;
        }
        else
        {
            // Exact native cold path, including its order-neutral predecessor
            // remove/reinsert. Never turn this performance fix into a true sort.
            UInt8* held = NULL;
            PerfRefNode* prefix = list->head;
            while (prefix != current)
            {
                PerfRefNode* position = prefix;
                prefix = prefix->next;
                if (held != position->light)
                {
                    if (held) releaseReference(held);
                    held = position->light;
                    if (held) addReference(held);
                }
                float predecessorScore = *(float*)(held + 0xD0);
                if (PerfScoreGreater(currentScore, predecessorScore))
                {
                    UInt8* removed = NULL;
                    remove(list, &removed, &position);
                    if (removed) releaseReference(removed);
                    insert(list, position, &held);
                    break;
                }
            }
            if (held) releaseReference(held);
        }
        previous = currentScore; havePrevious = true; ++processed;
        current = nextOuter;
    }
    PerfCount(&s_perfCounters.lightScores, scoreCalls);
    PerfCount(&s_perfCounters.lightPrefixSkipped, skipped);
    return true;
}
static __declspec(naked) void PerfNativeLightRank()
{
    __asm { push 0FFFFFFFFh }
    __asm { push kPerfLightHandler }
    __asm { push kPerfLightRankContinue }
    __asm { ret }
}
static void __stdcall PerfRankLightsEntry(UInt8* owner, void* camera)
{
    if (!PerfRankLights(owner, camera))
    {
        PerfCount(&s_perfCounters.lightFallbacks);
        ((void (__thiscall*)(UInt8*, void*))PerfNativeLightRank)(owner, camera);
    }
}
static __declspec(naked) void PerfLightRankPatch()
{
    __asm { push dword ptr [esp+4] }
    __asm { push ecx }
    __asm { call PerfRankLightsEntry }
    __asm { ret 4 }
}

struct PerfIDArray
{
    void* vtable;
    UInt32* data;
    UInt32 capacity, used, nonzero, growth;
};
static_assert(sizeof(PerfIDArray) == 24, "NiTLargeArray UInt32 layout");
struct PerfIndexEntry { UInt32 key, indexPlusOne; };
struct PerfIDIndex
{
    UInt8* owner;
    PerfIDArray* array;
    UInt32* data;
    UInt32 used;
    PerfUInt64 generation;
    PerfIndexEntry* entries;
    FormHeapFreeFn freeEntries;
    UInt32 capacity;
    bool valid;
};
static INIT_ONCE s_perfIndexOnce = INIT_ONCE_STATIC_INIT;
static CRITICAL_SECTION s_perfIndexLock;
static PerfIDIndex s_perfIDIndex[2] = {};
static PerfUInt64 s_perfIndexGeneration = 1;
static UInt32 s_perfIndexSuspended = 0;
static __declspec(thread) UInt32 s_perfLookupDepth = 0;
static BOOL CALLBACK PerfInitializeIndex(PINIT_ONCE, PVOID, PVOID*)
{
    return InitializeCriticalSectionAndSpinCount(&s_perfIndexLock, 1000);
}
static bool PerfLockIndex()
{
    if (!InitOnceExecuteOnce(&s_perfIndexOnce, PerfInitializeIndex, NULL, NULL)) return false;
    EnterCriticalSection(&s_perfIndexLock);
    return true;
}
static void PerfUnlockIndex() { LeaveCriticalSection(&s_perfIndexLock); }
static void PerfInvalidateIndexLocked()
{
    ++s_perfIndexGeneration;
    s_perfIDIndex[0].valid = s_perfIDIndex[1].valid = false;
}
static bool PerfBeginArrayMutation()
{
    if (!PerfLockIndex()) return false;
    ++s_perfIndexSuspended;
    PerfInvalidateIndexLocked();
    PerfUnlockIndex();
    return true;
}
static void PerfEndArrayMutation(bool begun)
{
    if (!begun) return;
    EnterCriticalSection(&s_perfIndexLock);
    PerfInvalidateIndexLocked();
    --s_perfIndexSuspended;
    PerfUnlockIndex();
}
static bool PerfValidArray(const PerfIDArray* array)
{
    return array && array->vtable == (void*)kPerfIDArrayVtableAddr &&
        array->used <= array->capacity && array->nonzero <= array->used &&
        array->capacity <= 0xFFFFFFFFu / sizeof(UInt32) &&
        (!array->capacity || array->data);
}
static void PerfIndexInsert(PerfIDIndex& index, UInt32 key, UInt32 position)
{
    UInt32 slot = PerfHash(key) & (index.capacity - 1);
    while (index.entries[slot].indexPlusOne && index.entries[slot].key != key)
        slot = (slot + 1) & (index.capacity - 1);
    if (!index.entries[slot].indexPlusOne)
    {
        index.entries[slot].key = key;
        index.entries[slot].indexPlusOne = position + 1;
    }
}
static bool PerfIndexFind(PerfIDIndex& index, UInt32 key, UInt32* result)
{
    UInt32 slot = PerfHash(key) & (index.capacity - 1);
    while (index.entries[slot].indexPlusOne)
    {
        if (index.entries[slot].key == key)
        {
            *result = index.entries[slot].indexPlusOne - 1;
            return true;
        }
        slot = (slot + 1) & (index.capacity - 1);
    }
    return false;
}
static __declspec(naked) void PerfNativeFormID()
{
    __asm { push ebx }
    __asm { mov ebx, [esp+8] }
    __asm { push kPerfFormIDContinue }
    __asm { ret }
}
static __declspec(naked) void PerfNativeWorldID()
{
    __asm { push ebx }
    __asm { mov ebx, [esp+8] }
    __asm { push kPerfWorldIDContinue }
    __asm { ret }
}
typedef UInt32 (__thiscall* PerfNativeLookupFn)(UInt8*, UInt32);
static UInt32 PerfColdLookup(UInt8* owner, UInt32 key, PerfNativeLookupFn original)
{
    PerfCount(&s_perfCounters.idFallbacks);
    bool begun = PerfBeginArrayMutation();
    UInt32 result;
    __try { result = original(owner, key); }
    __finally { PerfEndArrayMutation(begun); }
    return result;
}
// PERF-19: capacity is not serialized by SaveLoad_SaveIDArrays. Keep the
// native growth quantum, but amortize high-water appends with at most 256 KiB
// of optional storage above the native required increment. This bounds slack,
// not total array size or temporary old-plus-new allocation during replacement.
static const UInt32 kPerfIDAppendExtraSlots = 65536;
static bool PerfIDAppendCapacities(UInt32 used, UInt32 growth,
    UInt32* required, UInt32* preferred)
{
    const UInt32 limit = 0xFFFFFFFFu / sizeof(UInt32);
    if (!growth || used > limit || growth > limit - used) return false;
    *required = used + growth;
    *preferred = *required;
    UInt32 extraLimit = limit - *required;
    if (extraLimit > kPerfIDAppendExtraSlots) extraLimit = kPerfIDAppendExtraSlots;
    UInt32 ceiling = *required + extraLimit;
    PerfUInt64 target = (PerfUInt64)used + (used + 1u) / 2u;
    if (target <= *required) return true;
    if (target > ceiling) target = ceiling;
    PerfUInt64 rounded = ((target + growth - 1u) / growth) * growth;
    if (rounded > ceiling) rounded = (target / growth) * growth;
    if (rounded > *required) *preferred = (UInt32)rounded;
    return true;
}
static UInt32 PerfLookupIDCore(UInt8* owner, UInt32 key, bool world,
    PerfNativeLookupFn original,
    FormHeapAllocFn allocate = (FormHeapAllocFn)kFormHeapAllocAddr,
    FormHeapFreeFn release = (FormHeapFreeFn)kFormHeapFreeAddr,
    FormHeapAllocFn scratch = PerfScratchAllocate,
    FormHeapFreeFn freeScratch = PerfScratchRelease)
{
    PerfCount(&s_perfCounters.idCalls);
    if (!world && key >= 0xFF000000u) return key;
    if (!owner || s_perfLookupDepth > 1) return PerfColdLookup(owner, key, original);
    const UInt32 offset = world ? 0x78 : 0x74;
    for (unsigned attempt = 0; attempt < 5; ++attempt)
    {
        if (!PerfLockIndex()) break;
        PerfIDArray* array = *(PerfIDArray**)(owner + offset);
        if (s_perfIndexSuspended || !PerfValidArray(array)) { PerfUnlockIndex(); break; }
        PerfIDIndex& index = s_perfIDIndex[world ? 1 : 0];
        bool indexed = array->used >= 64;
        UInt32 tableCapacity = 0;
        if (indexed && !PerfTableCapacity(array->used + 1, &tableCapacity))
        { PerfUnlockIndex(); break; }
        if (indexed && index.capacity < tableCapacity)
        {
            PerfUnlockIndex();
            PerfIndexEntry* replacement = (PerfIndexEntry*)scratch(tableCapacity * sizeof(PerfIndexEntry));
            if (!replacement) break;
            if (!PerfLockIndex()) { freeScratch(replacement); break; }
            PerfIndexEntry* discarded = replacement;
            FormHeapFreeFn discard = freeScratch;
            if (index.capacity < tableCapacity)
            {
                discarded = index.entries;
                discard = index.freeEntries;
                index.entries = replacement; index.capacity = tableCapacity; index.valid = false;
                index.freeEntries = freeScratch;
            }
            PerfUnlockIndex();
            if (discarded) discard(discarded);
            continue; // Revalidate owner/array/generation after allocation.
        }
        if (indexed && (!index.valid || index.owner != owner || index.array != array ||
            index.data != array->data || index.used != array->used || index.generation != s_perfIndexGeneration))
        {
            index.valid = false;
            std::memset(index.entries, 0, index.capacity * sizeof(PerfIndexEntry));
            for (UInt32 i = 0; i < array->used; ++i) PerfIndexInsert(index, array->data[i], i);
            index.owner = owner; index.array = array; index.data = array->data;
            index.used = array->used; index.generation = s_perfIndexGeneration; index.valid = true;
            PerfCount(&s_perfCounters.idRebuilds);
        }
        UInt32 position = 0;
        bool found = false;
        if (indexed) found = PerfIndexFind(index, key, &position);
        else
            for (; position < array->used; ++position)
                if (array->data[position] == key) { found = true; break; }
        if (found)
        {
            if (position >= array->used || array->data[position] != key)
            { index.valid = false; PerfUnlockIndex(); continue; }
            PerfUnlockIndex(); PerfCount(&s_perfCounters.idHits); return position;
        }
        if (array->used < array->capacity)
        {
            position = array->used;
            array->data[position] = key;
            if (key) ++array->nonzero;
            ++array->used;
            if (indexed) { PerfIndexInsert(index, key, position); index.used = array->used; }
            else index.valid = false;
            PerfUnlockIndex(); PerfCount(&s_perfCounters.idAppends); return position;
        }
        PerfIDArray before = *array;
        PerfUInt64 generation = s_perfIndexGeneration;
        UInt32 required = 0, capacity = 0;
        if (!PerfIDAppendCapacities(before.used, before.growth, &required, &capacity))
        { PerfUnlockIndex(); break; }
        PerfUnlockIndex();
        // Never hold the private lock while calling the recoverable engine heap.
        UInt32* replacement = (UInt32*)allocate(capacity * 4);
        if (!replacement && capacity != required)
        {
            // A failed optional allocation may still invoke heap callbacks.
            // Revalidate before retrying the original required increment, and
            // revalidate again below after that allocation returns.
            if (!PerfLockIndex()) break;
            bool unchanged = !s_perfIndexSuspended && generation == s_perfIndexGeneration &&
                *(PerfIDArray**)(owner + offset) == array &&
                std::memcmp(&before, array, sizeof(before)) == 0;
            PerfUnlockIndex();
            if (!unchanged) continue;
            capacity = required;
            replacement = (UInt32*)allocate(capacity * 4);
        }
        if (!replacement) break;
        if (!PerfLockIndex()) { release(replacement); break; }
        if (s_perfIndexSuspended || generation != s_perfIndexGeneration ||
            *(PerfIDArray**)(owner + offset) != array ||
            std::memcmp(&before, array, sizeof(before)) != 0)
        { PerfUnlockIndex(); release(replacement); continue; }
        if (before.used) std::memcpy(replacement, before.data, before.used * 4);
        std::memset(replacement + before.used, 0, (capacity - before.used) * 4);
        array->data = replacement; array->capacity = capacity;
        if (index.valid && index.array == array) index.data = replacement;
        PerfUnlockIndex();
        if (before.data) release(before.data);
        // Reentrant free/append/reset is observed before choosing the new index.
    }
    // No invented index and no partially published allocation on refusal. The
    // engine retains its original required-allocation/fatal failure contract.
    return PerfColdLookup(owner, key, original);
}
static UInt32 __stdcall PerfLookupID(UInt8* owner, UInt32 key, UInt32 world)
{
    UInt32 result;
    ++s_perfLookupDepth;
    __try
    {
        result = PerfLookupIDCore(owner, key, world != 0,
            (PerfNativeLookupFn)(world ? PerfNativeWorldID : PerfNativeFormID));
    }
    __finally { --s_perfLookupDepth; }
    return result;
}
static __declspec(naked) void PerfFormIDPatch()
{
    __asm { push 0 }
    __asm { push dword ptr [esp+8] }
    __asm { push ecx }
    __asm { call PerfLookupID }
    __asm { ret 4 }
}
static __declspec(naked) void PerfWorldIDPatch()
{
    __asm { push 1 }
    __asm { push dword ptr [esp+8] }
    __asm { push ecx }
    __asm { call PerfLookupID }
    __asm { ret 4 }
}

static __declspec(naked) void PerfNativeOwnerCtor()
{
    __asm { push 0FFFFFFFFh }
    __asm { push kPerfOwnerCtorHandler }
    __asm { push kPerfOwnerCtorContinue }
    __asm { ret }
}
static __declspec(naked) void PerfNativeOwnerDestroy()
{
    __asm { push ebx }
    __asm { push esi }
    __asm { mov esi, ecx }
    __asm { mov ecx, [esi] }
    __asm { push kPerfOwnerDestroyContinue }
    __asm { ret }
}
static __declspec(naked) void PerfNativeOwnerReset()
{
    __asm { push 0FFFFFFFFh }
    __asm { push kPerfOwnerResetHandler }
    __asm { push kPerfOwnerResetContinue }
    __asm { ret }
}
static __declspec(naked) void PerfNativeIDLoad()
{
    __asm { sub esp, 118h }
    __asm { push kPerfIDLoadContinue }
    __asm { ret }
}
static __declspec(naked) void PerfNativeArraySet()
{
    __asm { mov eax, [esp+4] }
    __asm { cmp eax, [ecx+0Ch] }
    __asm { push kPerfArraySetContinue }
    __asm { ret }
}
static __declspec(naked) void PerfNativeArrayResize()
{
    __asm { mov eax, [esp+4] }
    __asm { push esi }
    __asm { mov esi, ecx }
    __asm { push kPerfArrayResizeContinue }
    __asm { ret }
}
typedef UInt32 (__thiscall* PerfOwnerCallFn)(UInt8*);
static UInt32 __stdcall PerfOwnerMutation(UInt8* owner, PerfOwnerCallFn original)
{
    bool begun = PerfBeginArrayMutation();
    UInt32 result;
    __try { result = original(owner); }
    __finally { PerfEndArrayMutation(begun); }
    return result;
}
static UInt32 __stdcall PerfLoadMutation(UInt8* owner, void* stream)
{
    bool begun = PerfBeginArrayMutation();
    UInt32 result;
    __try { result = ((UInt32 (__thiscall*)(UInt8*, void*))PerfNativeIDLoad)(owner, stream); }
    __finally { PerfEndArrayMutation(begun); }
    return result;
}
static UInt32 __stdcall PerfSetMutation(PerfIDArray* array, UInt32 at, UInt32* value)
{
    bool begun = (!array || array->vtable == (void*)kPerfIDArrayVtableAddr) && PerfBeginArrayMutation();
    UInt32 result;
    __try { result = ((UInt32 (__thiscall*)(PerfIDArray*, UInt32, UInt32*))PerfNativeArraySet)(array, at, value); }
    __finally { PerfEndArrayMutation(begun); }
    return result;
}
static void __stdcall PerfResizeMutation(PerfIDArray* array, UInt32 capacity)
{
    // Resize32 is shared by pointer arrays of other template types as well.
    // Only native UInt32 instances can back our supported ID arrays.
    bool begun = (!array || array->vtable == (void*)kPerfIDArrayVtableAddr) && PerfBeginArrayMutation();
    __try { ((void (__thiscall*)(PerfIDArray*, UInt32))PerfNativeArrayResize)(array, capacity); }
    __finally { PerfEndArrayMutation(begun); }
}
static __declspec(naked) void PerfOwnerCtorPatch()
{
    __asm { push offset PerfNativeOwnerCtor }
    __asm { push ecx }
    __asm { call PerfOwnerMutation }
    __asm { ret }
}
static __declspec(naked) void PerfOwnerDestroyPatch()
{
    __asm { push offset PerfNativeOwnerDestroy }
    __asm { push ecx }
    __asm { call PerfOwnerMutation }
    __asm { ret }
}
static __declspec(naked) void PerfOwnerResetPatch()
{
    __asm { push offset PerfNativeOwnerReset }
    __asm { push ecx }
    __asm { call PerfOwnerMutation }
    __asm { ret }
}
static __declspec(naked) void PerfIDLoadPatch()
{
    __asm { push dword ptr [esp+4] }
    __asm { push ecx }
    __asm { call PerfLoadMutation }
    __asm { ret 4 }
}
static __declspec(naked) void PerfArraySetPatch()
{
    __asm { push dword ptr [esp+8] }
    __asm { push dword ptr [esp+8] }
    __asm { push ecx }
    __asm { call PerfSetMutation }
    __asm { ret 8 }
}
static __declspec(naked) void PerfArrayResizePatch()
{
    __asm { push dword ptr [esp+4] }
    __asm { push ecx }
    __asm { call PerfResizeMutation }
    __asm { ret 4 }
}

struct PerfPrefixContext
{
    UInt32 frame;
    UInt8* owner;
    UInt8* stream;
    PerfIDArray* array;
    UInt32 total, start, end, which;
    bool valid;
};
static __declspec(thread) PerfPrefixContext s_perfPrefixes[16] = {};
static __declspec(thread) UInt32 s_perfPrefixVictim = 0;
static UInt32 PerfStreamPosition(const UInt8* stream)
{
    UInt32 alternate = *(const UInt32*)(stream + 0x30);
    return alternate == 0xFFFFFFFFu ? *(const UInt32*)(stream + 0x148) : alternate;
}
// An admitted SR-1 snapshot keeps PERF-4's count-aware reservation using its
// immutable extent. Unknown custom streams still use the native fallback.
static bool (*s_perfSnapshotReadStream)(const UInt8*);
static bool (*s_perfSnapshotExtent)(UInt8*, UInt32*);
static bool PerfNativeReadStream(const UInt8* stream)
{
    if (s_perfSnapshotReadStream && s_perfSnapshotReadStream(stream)) return true;
    return stream && *(void**)stream == (void*)kPerfBSFileVtableAddr &&
        *(void**)(stream + 4) == (void*)kArchiveReadCallbackDecodeAddr &&
        *(UInt32*)(stream + 0x20) == 0 && stream[0x24] && *(void**)(stream + 0x1C);
}
static bool PerfStreamExtent(UInt8* stream, UInt32* end)
{
    if (s_perfSnapshotExtent && s_perfSnapshotExtent(stream, end)) return true;
    // Use Oblivion's CRT descriptor layout, not this DLL's different CRT FILE
    // layout. GetFileSizeEx does not seek or disturb either input buffer.
    int descriptor = ((int (__cdecl*)(void*))kPerfFilenoAddr)(*(void**)(stream + 0x1C));
    if (descriptor < 0 || (UInt32)descriptor >= *(volatile UInt32*)kPerfCRTHandleCountAddr) return false;
    UInt8* page = ((UInt8**)kPerfCRTIOInfoAddr)[(UInt32)descriptor >> 5];
    if (!page) return false;
    UInt8* info = page + 0x28 * ((UInt32)descriptor & 31);
    if (!(info[4] & 1)) return false;
    HANDLE handle = *(HANDLE*)info;
    LARGE_INTEGER size;
    if (!handle || handle == INVALID_HANDLE_VALUE) return false;
    DWORD lastError = GetLastError();
    BOOL sized = GetFileSizeEx(handle, &size);
    SetLastError(lastError);
    if (!sized || size.QuadPart < 0 || (PerfUInt64)size.QuadPart > 0xFFFFFFFFull) return false;
    *end = size.LowPart;
    return true;
}
static void __stdcall PerfRecordIDPrefix(UInt32 frame, UInt8* stream, UInt8* owner,
    UInt32 count, UInt32 bytesRead, UInt32 which)
{
    if (!which)
        for (unsigned i = 0; i < 16; ++i)
            if (s_perfPrefixes[i].frame == frame) s_perfPrefixes[i].valid = false;
    PerfPrefixContext* record = NULL;
    for (unsigned i = 0; i < 16; ++i)
        if (s_perfPrefixes[i].frame == frame && s_perfPrefixes[i].which == which)
            record = &s_perfPrefixes[i];
    if (!record) record = &s_perfPrefixes[(s_perfPrefixVictim++) & 15];
    *record = {};
    record->frame = frame; record->owner = owner; record->stream = stream;
    record->which = which; record->total = count;
    if (bytesRead != 4 || !owner || !PerfNativeReadStream(stream)) return;
    PerfIDArray* array = *(PerfIDArray**)(owner + (which ? 0x78 : 0x74));
    if (!PerfValidArray(array) || count <= array->capacity || count > 0xFFFFFFFFu / 4) return;
    UInt32 position = PerfStreamPosition(stream);
    UInt32 size;
    if (!PerfStreamExtent(stream, &size) || PerfStreamPosition(stream) != position || position > size ||
        count > (size - position) / 4) return;
    record->array = array;
    record->start = position; record->end = size; record->valid = true;
}
static bool PerfTryBulkReserve(PerfIDArray* array, UInt32 request, UInt32 total,
    UInt32 currentIndex, const PerfPrefixContext* context,
    FormHeapAllocFn allocate = (FormHeapAllocFn)kFormHeapAllocAddr,
    FormHeapFreeFn release = (FormHeapFreeFn)kFormHeapFreeAddr)
{
    if (!context) return false;
    PerfPrefixContext savedContext = *context;
    context = &savedContext;
    if (!context->valid || context->array != array || context->total != total ||
        !PerfNativeReadStream(context->stream) || !PerfValidArray(array) ||
        *(PerfIDArray**)(context->owner + (context->which ? 0x78 : 0x74)) != array ||
        currentIndex != array->used || currentIndex >= total || !array->growth ||
        total <= array->capacity || total > 0xFFFFFFFFu / 4) return false;
    PerfUInt64 expected = (PerfUInt64)context->start + 4ull * (currentIndex + 1ull);
    if (expected > context->end || PerfStreamPosition(context->stream) != expected ||
        (PerfUInt64)total * 4 > (PerfUInt64)context->end - context->start) return false;
    PerfUInt64 originalRequest = (PerfUInt64)currentIndex + array->growth;
    if (originalRequest != request || originalRequest > 0xFFFFFFFFu / 4) return false;
    PerfUInt64 blocks = ((PerfUInt64)total - array->capacity + array->growth - 1) / array->growth;
    PerfUInt64 desired = array->capacity + blocks * array->growth;
    if (desired < request || desired > 0xFFFFFFFFu / 4) return false;
    if (!PerfLockIndex()) return false;
    PerfIDArray before = *array;
    PerfUInt64 generation = s_perfIndexGeneration;
    PerfUnlockIndex();
    UInt32* replacement = (UInt32*)allocate((UInt32)desired * 4);
    if (!replacement) return false;
    if (!PerfLockIndex()) { release(replacement); return false; }
    if (generation != s_perfIndexGeneration || std::memcmp(array, &before, sizeof(before)) != 0 ||
        *(PerfIDArray**)(context->owner + (context->which ? 0x78 : 0x74)) != array ||
        PerfStreamPosition(context->stream) != expected)
    { PerfUnlockIndex(); release(replacement); return false; }
    if (before.used) std::memcpy(replacement, before.data, before.used * 4);
    std::memset(replacement + before.used, 0, ((UInt32)desired - before.used) * 4);
    array->data = replacement; array->capacity = (UInt32)desired;
    PerfInvalidateIndexLocked();
    PerfUnlockIndex();
    if (before.data) release(before.data);
    return true;
}
static void __stdcall PerfBulkReserve(PerfIDArray* array, UInt32 request,
    UInt32 total, UInt32 index, UInt32 frame, UInt32 which)
{
    PerfCount(&s_perfCounters.reserveCalls);
    const PerfPrefixContext* context = NULL;
    for (unsigned i = 0; i < 16; ++i)
        if (s_perfPrefixes[i].frame == frame && s_perfPrefixes[i].which == which)
            context = &s_perfPrefixes[i];
    if (PerfTryBulkReserve(array, request, total, index, context))
        PerfCount(&s_perfCounters.reserveFast);
    else
    {
        PerfCount(&s_perfCounters.reserveFallbacks);
        // Refusal never pretends a void resize succeeded. Use the original
        // requested resize, including its existing engine OOM/exception contract.
        PerfResizeMutation(array, request);
    }
}
static __declspec(naked) void PerfNumericPrefixPatch()
{
    __asm {
        pushad
        mov edx, [esp+50h]
        lea ecx, [esp+34h]
        push 0
        push eax
        push edx
        push ebp
        push ebx
        push ecx
        call PerfRecordIDPrefix
        popad
        add esp, 14h
        cmp [esp+1Ch], edi
        push kPerfNumericPrefixContinue
        ret
    }
}
static __declspec(naked) void PerfWorldPrefixPatch()
{
    __asm {
        pushad
        mov edx, [esp+4Ch]
        lea ecx, [esp+34h]
        push 1
        push eax
        push edx
        push ebp
        push ebx
        push ecx
        call PerfRecordIDPrefix
        popad
        xor edi, edi
        add esp, 14h
        push kPerfWorldPrefixContinue
        ret
    }
}
static __declspec(naked) void PerfNumericReservePatch()
{
    __asm {
        pushad
        mov edx, [esp+44h]
        lea eax, [esp+28h]
        push 0
        push eax
        push edi
        push edx
        push dword ptr [esp+34h]
        push ecx
        call PerfBulkReserve
        popad
        ret 4
    }
}
static __declspec(naked) void PerfWorldReservePatch()
{
    __asm {
        pushad
        mov edx, [esp+40h]
        lea eax, [esp+28h]
        push 1
        push eax
        push edi
        push edx
        push dword ptr [esp+34h]
        push ecx
        call PerfBulkReserve
        popad
        ret 4
    }
}

typedef int (__cdecl* PerfStringCompareFn)(const char*, const char*);
typedef bool (__cdecl* PerfDefaultLocaleFn)();
typedef void (__cdecl* PerfMasterErrorFn)(const char*, ...);
static bool __cdecl PerfDefaultLocale()
{
    return *(volatile UInt32*)kPerfCRTLocaleFlagAddr == 0;
}
static bool PerfHashFilename(const char* text, UInt32* hash)
{
    if (!text) return false;
    UInt32 value = 2166136261u;
    for (UInt32 i = 0; i < 260; ++i)
    {
        UInt8 c = (UInt8)text[i];
        if (!c) { *hash = value; return true; }
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        value = (value ^ c) * 16777619u;
    }
    return false; // Longer names use the original comparator, not truncation.
}
struct PerfMasterIndexEntry { const char* name; UInt8* file; UInt32 hash; };
static UInt8* PerfFindMasterLinear(PerfListNode* candidates, const char* name,
    PerfStringCompareFn compare, PerfUInt64* comparisons)
{
    PerfListNode* fast = candidates;
    for (PerfListNode* node = candidates; node && node->data; node = node->next)
    {
        ++*comparisons;
        if (!compare(name, (char*)node->data + 0x1C)) return (UInt8*)node->data;
        fast = fast && fast->next ? fast->next->next : NULL;
        if (fast && fast == node->next) return NULL;
    }
    return NULL;
}
static bool PerfBuildMasterArray(UInt8* file, PerfListNode* candidates, bool reportMissing,
    FormHeapAllocFn allocate = (FormHeapAllocFn)kFormHeapAllocAddr,
    FormHeapFreeFn release = (FormHeapFreeFn)kFormHeapFreeAddr,
    FormHeapAllocFn scratch = PerfScratchAllocate,
    FormHeapFreeFn freeScratch = PerfScratchRelease,
    PerfStringCompareFn compare = (PerfStringCompareFn)kPerfMasterCompareAddr,
    PerfDefaultLocaleFn defaultLocale = PerfDefaultLocale,
    PerfMasterErrorFn error = (PerfMasterErrorFn)kPerfMasterErrorAddr)
{
    PerfCount(&s_perfCounters.masterCalls);
    if (!file) return false;
    UInt32 count = *(UInt32*)(file + 0x3F0);
    void* old = *(void**)(file + 0x3F4);
    *(void**)(file + 0x3F4) = NULL;
    if (old) release(old);
    if (!count) return true;
    if (count > 0xFFFFFFFFu / 4) return false;
    UInt8** output = (UInt8**)allocate(count * 4);
    if (!output) return false;
    std::memset(output, 0, count * 4);
    *(UInt8***)(file + 0x3F4) = output;
    if (*(UInt32*)(file + 0x3F0) != count) return false;

    PerfMasterIndexEntry* table = NULL;
    UInt32 capacity = 0;
    PerfUInt64 comparisons = 0;
    bool indexed = false;
    // One-off/tiny master lists do not amortize building a candidate index.
    if (count >= 8 && defaultLocale())
    {
        UInt32 candidatesCount = 0;
        PerfListNode* fast = candidates;
        bool valid = true;
        for (PerfListNode* node = candidates; node && node->data; node = node->next)
        {
            if (++candidatesCount > 0x200000u) { valid = false; break; }
            fast = fast && fast->next ? fast->next->next : NULL;
            if (fast && fast == node->next) { valid = false; break; }
        }
        if (valid && candidatesCount && PerfTableCapacity(candidatesCount, &capacity))
        {
            table = (PerfMasterIndexEntry*)scratch(capacity * sizeof(PerfMasterIndexEntry));
            if (table)
            {
                std::memset(table, 0, capacity * sizeof(PerfMasterIndexEntry));
                UInt32 traversed = 0;
                indexed = true;
                // Rebuild from the current list after auxiliary allocation.
                for (PerfListNode* node = candidates; node && node->data; node = node->next)
                {
                    const char* name = (char*)node->data + 0x1C;
                    UInt32 hash;
                    if (++traversed > candidatesCount || !PerfHashFilename(name, &hash))
                    { indexed = false; break; }
                    UInt32 slot = hash & (capacity - 1);
                    while (table[slot].name)
                    {
                        if (table[slot].hash == hash)
                        {
                            ++comparisons;
                            if (!compare(name, table[slot].name)) break;
                        }
                        slot = (slot + 1) & (capacity - 1);
                    }
                    if (!table[slot].name)
                    { table[slot].name = name; table[slot].file = (UInt8*)node->data; table[slot].hash = hash; }
                }
                indexed = indexed && defaultLocale();
            }
        }
    }
    if (indexed) PerfCount(&s_perfCounters.masterIndexed);
    bool success = true;
    PerfListNode* master = (PerfListNode*)(file + 0x3E0);
    for (UInt32 i = 0; i < count; ++i)
    {
        if (!master || !master->data) { success = false; break; }
        const char* name = (const char*)master->data;
        UInt8* found = NULL;
        UInt32 hash;
        bool useIndex = indexed && defaultLocale() && PerfHashFilename(name, &hash);
        if (useIndex)
        {
            UInt32 slot = hash & (capacity - 1);
            while (table[slot].name)
            {
                if (table[slot].hash == hash)
                {
                    ++comparisons;
                    if (!compare(name, table[slot].name)) { found = table[slot].file; break; }
                }
                slot = (slot + 1) & (capacity - 1);
            }
            // The CRT's locale-changed flag can switch during a call. Do not
            // return a hash miss/hit from a no-longer-compatible normalization.
            if (!defaultLocale()) { indexed = false; useIndex = false; }
        }
        if (!useIndex) found = PerfFindMasterLinear(candidates, name, compare, &comparisons);
        output[i] = found;
        if (!found)
        {
            success = false;
            if (reportMissing)
            {
                indexed = false; // Diagnostics may pump callbacks or alter lists.
                error("Missing Masterfile: %s", name);
            }
        }
        master = master->next;
    }
    if (master && master->data) success = false; // Declared count/list mismatch.
    if (table) freeScratch(table);
    PerfCount(&s_perfCounters.masterComparisons, comparisons);
    return success;
}
static bool __stdcall PerfMasterEntry(UInt8* file, PerfListNode* candidates, UInt32 reportMissing)
{
    return PerfBuildMasterArray(file, candidates, reportMissing != 0);
}
static __declspec(naked) void PerfMasterPatch()
{
    __asm { push dword ptr [esp+8] }
    __asm { push dword ptr [esp+8] }
    __asm { push ecx }
    __asm { call PerfMasterEntry }
    __asm { ret 8 }
}
static void LogPerformanceCounters()
{
    if (!s_perfCollectCounters) return;
    Log("performance: sort calls=%I64d reused=%I64d avoided node heap ops=%I64d",
        s_perfCounters.sortCalls, s_perfCounters.sortReused, s_perfCounters.sortHeapOpsAvoided);
    Log("performance: actor trim calls=%I64d suffix trims=%I64d removed=%I64d fallbacks=%I64d",
        s_perfCounters.trimCalls, s_perfCounters.trimFast, s_perfCounters.trimRemoved, s_perfCounters.trimFallbacks);
    Log("performance: light calls=%I64d scores=%I64d skipped prefix positions=%I64d unsupported=%I64d",
        s_perfCounters.lightCalls, s_perfCounters.lightScores, s_perfCounters.lightPrefixSkipped, s_perfCounters.lightFallbacks);
    Log("performance: ID calls=%I64d hits=%I64d appends=%I64d fallbacks=%I64d index rebuilds=%I64d",
        s_perfCounters.idCalls, s_perfCounters.idHits, s_perfCounters.idAppends, s_perfCounters.idFallbacks, s_perfCounters.idRebuilds);
    Log("performance: bulk reserve calls=%I64d fast=%I64d fallback=%I64d; master calls=%I64d indexed=%I64d comparisons=%I64d",
        s_perfCounters.reserveCalls, s_perfCounters.reserveFast, s_perfCounters.reserveFallbacks,
        s_perfCounters.masterCalls, s_perfCounters.masterIndexed, s_perfCounters.masterComparisons);
}
