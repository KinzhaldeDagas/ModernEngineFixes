// Included after RawSaveBlobMap and its callback typedefs, inside the private
// namespace. The address/signature declarations belong in dllmain.cpp.
// PERF-10/11: bounded auxiliary indices; native lists, buckets and ownership
// remain authoritative. Native containers require their existing external
// serialization. These indices do not make an otherwise racing map safe.
typedef unsigned __int64 ActorMapUInt64;
struct __declspec(align(8)) ActorMapCounters
{
    volatile LONG64 actorCalls, actorIndexed, actorFallbacks;
    volatile LONG64 mapCalls, mapIndexed, mapBuildVisits, mapFallbacks;
};
static ActorMapCounters s_actorMapCounters = {};
static bool s_actorMapCollectCounters;
static void ActorMapCount(volatile LONG64* counter, ActorMapUInt64 amount = 1)
{
    if (s_actorMapCollectCounters) InterlockedExchangeAdd64(counter, (LONG64)amount);
}
static UInt32 ActorMapHash(UInt32 key)
{
    key ^= key >> 16; key *= 0x7FEB352Du;
    key ^= key >> 15; key *= 0x846CA68Bu;
    return key ^ (key >> 16);
}
static void* __cdecl ActorMapAllocate(UInt32 bytes)
{ return bytes ? HeapAlloc(GetProcessHeap(), 0, bytes) : NULL; }
static void __cdecl ActorMapRelease(void* p)
{ if (p) HeapFree(GetProcessHeap(), 0, p); }
static bool ActorMapCapacity(UInt32 count, UInt32* capacity)
{
    // At most 1 MiB per map, 512 KiB for an actor invocation. A larger native
    // population keeps working using its original searches.
    if (count > 65536) return false;
    UInt32 n = 128;
    while (n < count * 2) n *= 2;
    *capacity = n;
    return true;
}

struct ActorMapNode { void* data; ActorMapNode* next; };
typedef bool (__cdecl* ActorMapKnownPredicateFn)(void*);
typedef bool (__cdecl* ActorMapKnownDistanceFn)(void*, void*);
typedef UInt8 (__thiscall* ActorMapPredicateFn)(void*);
typedef double (__thiscall* ActorMapDistanceFn)(void*, void*, UInt32);
typedef void (__thiscall* ActorMapPushFrontFn)(ActorMapNode*, void*);
static __declspec(thread) ActorMapUInt64 s_actorRebuildEpoch;

static bool __cdecl ActorMapKnownPredicate(void* actor)
{
    return actor && (*(UInt32**)actor)[0x190 / 4] == kPerfActorTruePredicateAddr;
}
static bool __cdecl ActorMapKnownDistance(void* actor, void* player)
{
    if (!actor || !player) return true; // GetDistance returns its sentinel.
    // With the native third argument zero, either target flag returns before
    // reading cells, position or process. Do not inspect otherwise-unused state.
    if (*(UInt32*)((UInt8*)player + 8) & 0x820) return true;
    // Parentless references use a child-cell virtual. The swimming-position
    // branch invokes additional gameplay code. Retain the native search for
    // either case; normal resident, non-swimming actors have no such callback.
    if (!*(void**)((UInt8*)actor + 0x40) || !*(void**)((UInt8*)player + 0x40)) return false;
    if ((*(UInt32**)player)[0x174 / 4] != kPerfActorPositionAddr) return false;
    void* process = *(void**)((UInt8*)player + 0x58);
    if (!process) return true;
    UInt32 getter = (*(UInt32**)process)[0x2C0 / 4];
    if (getter == kPerfActorZeroMovementAddr) return true;
    return getter == kPerfActorMovementAddr &&
        !(*(UInt16*)((UInt8*)process + 0x1FC) & 0x800);
}
static bool ActorMapRadiusGreater(float radius, float distance)
{
    UInt8 greater;
    __asm {
        fld distance
        fld radius
        fcompp
        fnstsw ax
        test ah, 41h
        setz greater
    }
    return greater != 0;
}
static bool ActorMapContains(ActorMapNode* head, void* value)
{
    for (ActorMapNode* p = head; p; p = p->next)
        if (p->data == value) return true;
    return false;
}
// The complete native traversal is retained, including its carried reference
// when a virtual predicate returns false. Unknown callbacks turn the index off
// BEFORE invocation; fallback never restarts work or repeats side effects.
static void PerfRebuildActors(ActorMapNode* destination, ActorMapNode* source,
    void** player, volatile float* radius,
    ActorMapKnownPredicateFn knownPredicate = ActorMapKnownPredicate,
    ActorMapKnownDistanceFn knownDistance = ActorMapKnownDistance,
    ActorMapDistanceFn distance = (ActorMapDistanceFn)kPerfActorDistanceAddr,
    ActorMapPushFrontFn push = (ActorMapPushFrontFn)kPerfActorPushFrontAddr,
    FormHeapFreeFn releaseNode = (FormHeapFreeFn)kFormHeapFreeAddr,
    FormHeapAllocFn scratch = ActorMapAllocate,
    FormHeapFreeFn releaseScratch = ActorMapRelease)
{
    const ActorMapUInt64 epoch = ++s_actorRebuildEpoch;
    ActorMapCount(&s_actorMapCounters.actorCalls);
    UInt32* keys = NULL;
    UInt32 capacity = 0, used = 0;
    bool enabled = true;
    __try
    {
        while (destination->next)
        {
            ActorMapNode* next = destination->next->next;
            releaseNode(destination->next);
            destination->next = next;
        }
        destination->data = NULL;
        void* carried = NULL;
        for (ActorMapNode* node = source; node; node = node->next)
        {
            if (!node->next && !node->data) break;
            if (!knownPredicate(node->data)) enabled = false;
            if (((ActorMapPredicateFn)(*(void***)node->data)[0x190 / 4])(node->data))
                carried = node->data;
            if (!carried) continue;
            void* currentPlayer = *player;
            if (!knownDistance(carried, currentPlayer)) enabled = false;
            const float separation = (float)distance(carried, currentPlayer, 0);
            if (s_actorRebuildEpoch != epoch) enabled = false;
            if (!ActorMapRadiusGreater(*radius, separation)) continue;

            if (enabled && (!capacity || used + 1 > capacity / 2))
            {
                UInt32 nextCapacity = 0;
                if (!ActorMapCapacity(used + 1, &nextCapacity)) enabled = false;
                UInt32* replacement = enabled ? (UInt32*)scratch(nextCapacity * 4) : NULL;
                if (!replacement) enabled = false;
                if (replacement)
                {
                    std::memset(replacement, 0, nextCapacity * 4);
                    for (UInt32 i = 0; i < capacity; ++i)
                    {
                        UInt32 key = keys[i];
                        if (!key) continue;
                        UInt32 slot = ActorMapHash(key) & (nextCapacity - 1);
                        while (replacement[slot]) slot = (slot + 1) & (nextCapacity - 1);
                        replacement[slot] = key;
                    }
                    releaseScratch(keys); keys = replacement; capacity = nextCapacity;
                    if (s_actorRebuildEpoch != epoch) enabled = false;
                }
            }
            UInt32 slot = 0;
            bool contains;
            if (enabled)
            {
                ActorMapCount(&s_actorMapCounters.actorIndexed);
                slot = ActorMapHash((UInt32)carried) & (capacity - 1);
                while (keys[slot] && keys[slot] != (UInt32)carried)
                    slot = (slot + 1) & (capacity - 1);
                contains = keys[slot] != 0;
            }
            else
            {
                ActorMapCount(&s_actorMapCounters.actorFallbacks);
                contains = ActorMapContains(destination, carried);
            }
            if (!contains)
            {
                // Keep native PushFront allocation and its fatal required-node
                // OOM contract. Auxiliary OOM above only changes membership work.
                push(destination, carried);
                if (s_actorRebuildEpoch != epoch) enabled = false;
                if (enabled) { keys[slot] = (UInt32)carried; ++used; }
            }
        }
    }
    __finally { releaseScratch(keys); }
}
static void __stdcall PerfActorRebuildDispatch(UInt8* manager)
{
    if (manager != (UInt8*)kPerfActorManagerAddr)
    {
        ++s_actorRebuildEpoch;
        ((void (__thiscall*)(UInt8*))kPerfActorRebuildNativeAddr)(manager);
        return;
    }
    PerfRebuildActors((ActorMapNode*)(manager + 0x60), (ActorMapNode*)(manager + 0x68),
        (void**)kPerfActorPlayerAddr, (volatile float*)kPerfActorRadiusAddr);
}
static __declspec(naked) void PerfActorRebuildPatch()
{
    __asm { push ecx }
    __asm { call PerfActorRebuildDispatch }
    __asm { ret }
}

struct SaveMapIndexEntry { UInt32 key; RawSaveBlobMapNode* node; };
struct SaveMapIndex
{
    RawSaveBlobMap* map;
    RawSaveBlobMapNode** buckets;
    UInt32 bucketCount, count, capacity;
    ActorMapUInt64 epoch;
    SaveMapIndexEntry* entries;
    FormHeapFreeFn release;
};
static __declspec(align(8)) volatile LONG64 s_saveMapEpoch;
static __declspec(thread) SaveMapIndex s_saveMapIndex[4];
static __declspec(thread) UInt32 s_saveMapReplaceSlot;
static __declspec(thread) UInt32 s_saveMapActive;
static ActorMapUInt64 SaveMapEpoch()
{ return (ActorMapUInt64)InterlockedCompareExchange64(&s_saveMapEpoch, 0, 0); }
static void (__cdecl* s_perfSaveMapMutationObserver)(RawSaveBlobMap*);
static void __cdecl PerfSaveMapInvalidate(RawSaveBlobMap* map)
{
    if (s_perfSaveMapMutationObserver) s_perfSaveMapMutationObserver(map);
    if (map && map->vtable == (void**)kPerfSaveMapVtableAddr)
        InterlockedIncrement64(&s_saveMapEpoch);
}
static void __cdecl PerfSaveMapInvalidateAll()
{
    if (s_perfSaveMapMutationObserver) s_perfSaveMapMutationObserver(NULL);
    InterlockedIncrement64(&s_saveMapEpoch);
}
static void PerfReleaseThreadSaveMapIndices()
{
    for (UInt32 i = 0; i < 4; ++i)
    {
        SaveMapIndex& index = s_saveMapIndex[i];
        SaveMapIndexEntry* entries = index.entries;
        FormHeapFreeFn release = index.release;
        // Detach first: thread teardown releases private OS-heap storage only.
        // A test/injected release may reenter without seeing an owned table.
        std::memset(&index, 0, sizeof(index));
        if (entries) release(entries);
    }
    s_saveMapReplaceSlot = 0;
}
static void PerfReleaseSaveMapIndices()
{
    PerfSaveMapInvalidateAll();
    PerfReleaseThreadSaveMapIndices();
}
static SaveMapIndexEntry* SaveMapSlot(SaveMapIndex& index, UInt32 key)
{
    UInt32 slot = ActorMapHash(key) & (index.capacity - 1);
    while (index.entries[slot].node && index.entries[slot].key != key)
        slot = (slot + 1) & (index.capacity - 1);
    return index.entries + slot;
}
static SaveMapIndex* SaveMapPrepare(RawSaveBlobMap* map,
    FormHeapAllocFn scratch, FormHeapFreeFn releaseScratch)
{
    UInt32 capacity;
    if (map->count == 0xFFFFFFFFu || map->bucketCount > 65536 ||
        !ActorMapCapacity(map->count + 1, &capacity)) return NULL;
    SaveMapIndex* index = NULL;
    for (UInt32 i = 0; i < 4; ++i)
        if (s_saveMapIndex[i].map == map) { index = &s_saveMapIndex[i]; break; }
    if (!index) index = &s_saveMapIndex[s_saveMapReplaceSlot++ & 3];
    ActorMapUInt64 epoch = SaveMapEpoch();
    if (index->map == map && index->epoch == epoch && index->buckets == map->buckets &&
        index->bucketCount == map->bucketCount && index->count == map->count &&
        index->capacity >= capacity) return index;

    // Detach private memory before auxiliary allocation/reentry. The index
    // stores borrowed nodes and is never traversed during invalidation.
    SaveMapIndexEntry* oldEntries = index->entries;
    FormHeapFreeFn oldRelease = index->release;
    std::memset(index, 0, sizeof(*index));
    if (oldEntries) oldRelease(oldEntries);
    SaveMapIndexEntry* entries = (SaveMapIndexEntry*)scratch(capacity * sizeof(SaveMapIndexEntry));
    if (!entries) return NULL;
    std::memset(entries, 0, capacity * sizeof(SaveMapIndexEntry));
    // The allocator may have reentered or modified the live map. Re-evaluate
    // its size and epoch before borrowing nodes; never publish an old snapshot.
    epoch = SaveMapEpoch();
    if (map->count >= capacity / 2 || !map->bucketCount || !map->buckets)
    { releaseScratch(entries); return NULL; }
    SaveMapIndex proposed = { map, map->buckets, map->bucketCount, map->count,
        capacity, epoch, entries, releaseScratch };
    UInt32 visited = 0;
    bool valid = true;
    for (UInt32 bucket = 0; valid && bucket < map->bucketCount; ++bucket)
    {
        for (RawSaveBlobMapNode* node = map->buckets[bucket]; node; node = node->next)
        {
            if (visited++ >= map->count || node->key % map->bucketCount != bucket)
            { valid = false; break; }
            SaveMapIndexEntry* slot = SaveMapSlot(proposed, node->key);
            // Preserve the native first node on malformed duplicate-key chains.
            if (!slot->node) { slot->key = node->key; slot->node = node; }
        }
    }
    if (!valid || visited != map->count || SaveMapEpoch() != epoch)
    { releaseScratch(entries); return NULL; }
    ActorMapCount(&s_actorMapCounters.mapBuildVisits, visited);
    *index = proposed;
    return index;
}
static bool SaveMapSupported(RawSaveBlobMap* map)
{
    return map && map->vtable == (void**)kPerfSaveMapVtableAddr && map->bucketCount && map->buckets &&
        map->vtable[1] == (void*)kPerfSaveMapHashAddr && map->vtable[2] == (void*)kPerfSaveMapEqualAddr &&
        map->vtable[3] == (void*)kPerfSaveMapSetValueAddr && map->vtable[4] == (void*)kPerfSaveMapClearValueAddr &&
        map->vtable[5] == (void*)kPerfSaveMapAllocateAddr;
}
// Returns whether the optimized helper handled the request; result carries the
// existing helper's success/ownership result. Unsupported/OOM/reentry falls
// through to the existing checked helper before any native side effect.
static bool PerfTryStoreSaveMap(RawSaveBlobMap* map, UInt32 key,
    bool zeroKeyWhenExisting, void* blob, bool* result,
    FormHeapAllocFn scratch = ActorMapAllocate,
    FormHeapFreeFn releaseScratch = ActorMapRelease,
    bool (*supported)(RawSaveBlobMap*) = SaveMapSupported)
{
    ActorMapCount(&s_actorMapCounters.mapCalls);
    if (s_saveMapActive)
    {
        // The nested operation will use the original helper, which does not
        // enter a generic SetAt detour. Invalidate the outer snapshot first.
        PerfSaveMapInvalidateAll();
        ActorMapCount(&s_actorMapCounters.mapFallbacks);
        return false;
    }
    if (!supported(map))
    {
        PerfSaveMapInvalidate(map);
        ActorMapCount(&s_actorMapCounters.mapFallbacks);
        return false;
    }
    bool handled = false;
    ++s_saveMapActive;
    __try
    {
        SaveMapIndex* index = SaveMapPrepare(map, scratch, releaseScratch);
        if (index)
        {
            const ActorMapUInt64 epoch = index->epoch;
            SaveMapIndexEntry* slot = SaveMapSlot(*index, key);
            if (slot->node && zeroKeyWhenExisting) { key = 0; slot = SaveMapSlot(*index, key); }
            RawSaveBlobMapNode* entry = slot->node;
            if (entry)
            {
                ((SaveBlobMapClearFn)map->vtable[4])(map, entry);
                ((SaveBlobMapSetFn)map->vtable[3])(map, entry, key, blob);
                *result = true;
            }
            else
            {
                const UInt32 bucket = key % map->bucketCount;
                entry = ((SaveBlobMapAllocateFn)map->vtable[5])(map);
                *result = entry != NULL;
                if (entry)
                {
                    ((SaveBlobMapSetFn)map->vtable[3])(map, entry, key, blob);
                    entry->next = map->buckets[bucket];
                    map->buckets[bucket] = entry;
                    ++map->count;
                    // Native allocation can reenter. Preserve the existing
                    // helper's prepend behavior but never publish stale nodes.
                    if (SaveMapEpoch() == epoch && index->map == map &&
                        index->buckets == map->buckets && index->bucketCount == map->bucketCount &&
                        index->count + 1 == map->count)
                    { slot->key = key; slot->node = entry; index->count = map->count; }
                    else index->epoch = ~SaveMapEpoch();
                }
            }
            handled = true;
            ActorMapCount(&s_actorMapCounters.mapIndexed);
        }
    }
    __finally { --s_saveMapActive; }
    if (!handled) ActorMapCount(&s_actorMapCounters.mapFallbacks);
    return handled;
}

// Invalidate before any native mutation/callback, then replay only complete
// original instructions. These entries preserve flags and all registers.
static __declspec(naked) void PerfSaveMapSetAtPatch()
{
    __asm { pushfd }
    __asm { pushad }
    __asm { push ecx }
    __asm { call PerfSaveMapInvalidate }
    __asm { add esp, 4 }
    __asm { popad }
    __asm { popfd }
    __asm { push ebx }
    __asm { mov ebx, [esp+8] }
    __asm { push kPerfSaveMapSetAtContinue }
    __asm { ret }
}
static __declspec(naked) void PerfSaveMapRemovePatch()
{
    __asm { pushfd }
    __asm { pushad }
    __asm { push ecx }
    __asm { call PerfSaveMapInvalidate }
    __asm { add esp, 4 }
    __asm { popad }
    __asm { popfd }
    __asm { push ebx }
    __asm { push ebp }
    __asm { mov ebp, [esp+0Ch] }
    __asm { push kPerfSaveMapRemoveContinue }
    __asm { ret }
}
static __declspec(naked) void PerfSaveMapClearPatch()
{
    __asm { pushfd }
    __asm { pushad }
    __asm { push ecx }
    __asm { call PerfSaveMapInvalidate }
    __asm { add esp, 4 }
    __asm { popad }
    __asm { popfd }
    __asm { push ebx }
    __asm { push esi }
    __asm { mov esi, ecx }
    __asm { xor ebx, ebx }
    __asm { push kPerfSaveMapClearContinue }
    __asm { ret }
}
static __declspec(naked) void PerfSaveMapCtorPatch()
{
    __asm { pushfd }
    __asm { pushad }
    __asm { call PerfSaveMapInvalidateAll }
    __asm { popad }
    __asm { popfd }
    __asm { push esi }
    __asm { mov esi, ecx }
    __asm { xor ecx, ecx }
    __asm { push kPerfSaveMapCtorContinue }
    __asm { ret }
}
static __declspec(naked) void PerfSaveMapDtorPatch()
{
    __asm { pushfd }
    __asm { pushad }
    __asm { call PerfSaveMapInvalidateAll }
    __asm { popad }
    __asm { popfd }
    __asm { push -1 }
    __asm { push kPerfSaveMapDtorHandlerAddr }
    __asm { push kPerfSaveMapDtorContinue }
    __asm { ret }
}
static void (__cdecl* s_perfActorMapPressureObserver)();
static void __cdecl PerfActorMapPressureInvalidate()
{
    ++s_actorRebuildEpoch;
    PerfSaveMapInvalidateAll();
    if (s_perfActorMapPressureObserver) s_perfActorMapPressureObserver();
}
static __declspec(naked) void PerfActorMapPressurePatch()
{
    __asm { pushfd }
    __asm { pushad }
    __asm { call PerfActorMapPressureInvalidate }
    __asm { popad }
    __asm { popfd }
    // MASM's [C++ const] names the constant's storage, not its value as an
    // absolute address. Materialize the address before replaying the load.
    __asm { mov eax, kPerfActorMainAddr }
    __asm { mov eax, [eax] }
    __asm { push kPerfActorMapPressureContinue }
    __asm { ret }
}
