// PERF-18: a negative-query accelerator only. Native matching scans retain
// all callbacks, duplicate records, unlink order and payload ownership.
typedef unsigned __int64 PerfExteriorUInt64;
struct PerfExteriorRecord { UInt32 referenceID; SInt32 x, y; };
struct PerfExteriorNode { PerfExteriorRecord* data; PerfExteriorNode* next; };
struct PerfExteriorSlot { SInt32 x, y; UInt32 occupied; };
typedef void* (__cdecl* PerfExteriorAllocFn)(UInt32);
typedef void (__cdecl* PerfExteriorFreeFn)(void*);
typedef DWORD (__cdecl* PerfExteriorThreadFn)();
struct PerfExteriorCache
{
    UInt8* owner;
    void* map;
    PerfExteriorNode* list;
    PerfExteriorUInt64 epoch;
    PerfExteriorSlot* entries;
    PerfExteriorFreeFn release;
    UInt32 capacity;
    bool valid;
};
struct __declspec(align(8)) PerfExteriorCounters
{ volatile LONG64 calls, builds, buildVisits, negativeHits, reuses, fallbacks; };
static PerfExteriorCounters s_perfExteriorCounters = {};
static bool s_perfExteriorCollectCounters;
static __declspec(align(8)) volatile LONG64 s_perfExteriorEpoch = 1;
static volatile LONG s_perfExteriorWriters;
static __declspec(thread) PerfExteriorCache s_perfExteriorCaches[2];
static __declspec(thread) UInt32 s_perfExteriorDepth;
static __declspec(thread) UInt32 s_perfExteriorConsumerDepth;
static __declspec(thread) UInt32 s_perfExteriorNextSlot;
static PerfExteriorThreadFn s_perfExteriorThreadProvider;
static const UInt32 kPerfExteriorMaximumNodes = 131072;
static const UInt32 kPerfExteriorMinimumNodes = 32;
static void PerfExteriorCount(volatile LONG64* counter, UInt32 amount = 1)
{ if (s_perfExteriorCollectCounters) InterlockedExchangeAdd64(counter, amount); }
static PerfExteriorUInt64 PerfExteriorEpoch()
{ return (PerfExteriorUInt64)InterlockedCompareExchange64(&s_perfExteriorEpoch, 0, 0); }
static void __cdecl PerfExteriorInvalidate()
{ InterlockedIncrement64(&s_perfExteriorEpoch); }
static void PerfExteriorBeginWrite()
{ InterlockedIncrement(&s_perfExteriorWriters); PerfExteriorInvalidate(); }
static void PerfExteriorEndWrite()
{ PerfExteriorInvalidate(); InterlockedDecrement(&s_perfExteriorWriters); }
static void PerfExteriorSetThreadProvider(PerfExteriorThreadFn provider)
{ s_perfExteriorThreadProvider = provider; PerfExteriorInvalidate(); }
static bool PerfExteriorMainThread()
{ return s_perfExteriorThreadProvider && s_perfExteriorThreadProvider() == GetCurrentThreadId(); }
static void* __cdecl PerfExteriorAllocate(UInt32 bytes)
{ return HeapAlloc(GetProcessHeap(), 0, bytes); }
static void __cdecl PerfExteriorFree(void* p)
{ if (p) HeapFree(GetProcessHeap(), 0, p); }
static void PerfExteriorDrop(PerfExteriorCache* cache)
{
    void* entries = cache->entries;
    PerfExteriorFreeFn release = cache->release;
    *cache = {};
    if (entries) release(entries);
}
static void PerfExteriorReleaseThreadCaches()
{
    // Other threads never borrow these private tables. Worker detachment must
    // not invalidate the main thread's otherwise unchanged coordinate index.
    for (UInt32 i = 0; i < 2; ++i) PerfExteriorDrop(&s_perfExteriorCaches[i]);
}
static UInt32 PerfExteriorHash(SInt32 x, SInt32 y)
{
    UInt32 h = (UInt32)x * 0x9E3779B1u + (UInt32)y * 0x85EBCA77u;
    h ^= h >> 16; h *= 0x7FEB352Du; return h ^ (h >> 15);
}
static PerfExteriorSlot* PerfExteriorFind(PerfExteriorCache* cache, SInt32 x, SInt32 y)
{
    UInt32 at = PerfExteriorHash(x, y) & (cache->capacity - 1);
    while (cache->entries[at].occupied &&
        (cache->entries[at].x != x || cache->entries[at].y != y))
        at = (at + 1) & (cache->capacity - 1);
    return &cache->entries[at];
}
static bool PerfExteriorKnownMap(void* map)
{ return map && *(void**)map == (void*)kPerfExteriorMapVtableAddr; }
static bool PerfExteriorRefuse()
{
    // Positive and unsupported queries enter a native matching loop that can
    // mutate the list and call arbitrary restoration code. Invalidate first.
    PerfExteriorInvalidate(); PerfExteriorCount(&s_perfExteriorCounters.fallbacks);
    return false;
}
static bool PerfExteriorQueryCore(UInt8* owner, PerfExteriorNode* list, SInt32 x, SInt32 y,
    PerfExteriorAllocFn allocate, PerfExteriorFreeFn release)
{
    if (!owner || !list || s_perfExteriorConsumerDepth > 1 || !PerfExteriorMainThread() ||
        InterlockedCompareExchange(&s_perfExteriorWriters, 0, 0)) return PerfExteriorRefuse();
    void* map = *(void**)(owner + 0xC);
    if (!PerfExteriorKnownMap(map)) return PerfExteriorRefuse();
    PerfExteriorUInt64 epoch = PerfExteriorEpoch();
    PerfExteriorCache* cache = NULL;
    for (UInt32 i = 0; i < 2; ++i)
    {
        PerfExteriorCache* candidate = &s_perfExteriorCaches[i];
        if (candidate->valid && candidate->epoch == epoch && candidate->owner == owner &&
            candidate->map == map && candidate->list == list)
        { cache = candidate; PerfExteriorCount(&s_perfExteriorCounters.reuses); break; }
    }
    if (!cache)
    {
        UInt32 count = 0;
        for (PerfExteriorNode* p = list; p; p = p->next)
        {
            if (++count > kPerfExteriorMaximumNodes) return PerfExteriorRefuse();
            // Seeing a match already proves native work is needed. Avoid all
            // scratch allocation and further traversal for this case.
            if (p->data && p->data->x == x && p->data->y == y) return PerfExteriorRefuse();
        }
        if (count < kPerfExteriorMinimumNodes) return PerfExteriorRefuse();
        UInt32 capacity = 64;
        while (capacity < count * 2) capacity *= 2;
        cache = &s_perfExteriorCaches[s_perfExteriorNextSlot++ & 1];
        PerfExteriorDrop(cache);
        // Release/allocate are injectable for validation. Reject any callback
        // mutation before dereferencing list nodes again or publishing a cache.
        if (PerfExteriorEpoch() != epoch) return PerfExteriorRefuse();
        auto entries = (PerfExteriorSlot*)allocate(capacity * sizeof(PerfExteriorSlot));
        if (!entries) return PerfExteriorRefuse();
        if (PerfExteriorEpoch() != epoch || !PerfExteriorMainThread() ||
            InterlockedCompareExchange(&s_perfExteriorWriters, 0, 0) ||
            *(void**)(owner + 0xC) != map || !PerfExteriorKnownMap(map))
        { release(entries); return PerfExteriorRefuse(); }
        std::memset(entries, 0, capacity * sizeof(PerfExteriorSlot));
        cache->owner = owner; cache->map = map; cache->list = list; cache->epoch = epoch;
        cache->entries = entries; cache->release = release; cache->capacity = capacity;
        UInt32 scanned = 0;
        for (PerfExteriorNode* p = list; p; p = p->next)
        {
            if (++scanned > count) { PerfExteriorDrop(cache); return PerfExteriorRefuse(); }
            if (!p->data) continue;
            PerfExteriorSlot* slot = PerfExteriorFind(cache, p->data->x, p->data->y);
            slot->x = p->data->x; slot->y = p->data->y; slot->occupied = 1;
        }
        PerfExteriorCount(&s_perfExteriorCounters.buildVisits, count + scanned);
        if (scanned != count || PerfExteriorEpoch() != epoch)
        { PerfExteriorDrop(cache); return PerfExteriorRefuse(); }
        cache->valid = true; PerfExteriorCount(&s_perfExteriorCounters.builds);
    }
    if (PerfExteriorFind(cache, x, y)->occupied) return PerfExteriorRefuse();
    PerfExteriorCount(&s_perfExteriorCounters.negativeHits); return true;
}
static bool PerfExteriorQueryNoMatch(UInt8* owner, PerfExteriorNode* list, SInt32 x, SInt32 y,
    PerfExteriorAllocFn allocate = PerfExteriorAllocate, PerfExteriorFreeFn release = PerfExteriorFree)
{
    PerfExteriorCount(&s_perfExteriorCounters.calls);
    if (s_perfExteriorDepth) return PerfExteriorRefuse();
    bool result;
    ++s_perfExteriorDepth;
    __try { result = PerfExteriorQueryCore(owner, list, x, y, allocate, release); }
    __finally { --s_perfExteriorDepth; }
    return result;
}
static UInt32 __stdcall PerfExteriorQueryEntry(UInt8* owner, PerfExteriorNode* list, SInt32 x, SInt32 y)
{ return PerfExteriorQueryNoMatch(owner, list, x, y) ? 1u : 0u; }
static __declspec(naked) void PerfExteriorQueryPatch()
{
    __asm {
        pushfd
        pushad
        mov ebp, esp
        sub esp, 528
        and esp, -16
        fxsave [esp]
        push dword ptr [ebp+40h]
        push dword ptr [ebp+3Ch]
        push edi
        push ebx
        call PerfExteriorQueryEntry
        mov [ebp+1Ch], eax
        fxrstor [esp]
        mov esp, ebp
        popad
        popfd
        test eax, eax
        jnz no_match
        mov eax, edi
        test eax, eax
        jz no_match
        push kPerfExteriorQueryContinue
        ret
    no_match:
        mov eax, edi
        test eax, eax
        push kPerfExteriorQueryEmpty
        ret
    }
}
static __declspec(naked) void PerfExteriorNativeAdd()
{
    __asm { push ebx }
    __asm { mov ebx, [esp+8] }
    __asm { push kPerfExteriorAddContinue }
    __asm { ret }
}
static __declspec(naked) void PerfExteriorNativeCtor()
{
    __asm { push esi }
    __asm { mov esi, ecx }
    __asm { xor ecx, ecx }
    __asm { push kPerfExteriorCtorContinue }
    __asm { ret }
}
static __declspec(naked) void PerfExteriorNativeDtor()
{
    __asm { push -1 }
    __asm { push kPerfExteriorDtorHandler }
    __asm { push kPerfExteriorDtorContinue }
    __asm { ret }
}
static void __fastcall PerfExteriorAddPatch(void* map, void*, UInt32 worldID, UInt32 referenceID,
    float x, float y, float z)
{
    PerfExteriorBeginWrite();
    __try { ((void (__thiscall*)(void*, UInt32, UInt32, float, float, float))PerfExteriorNativeAdd)(map, worldID, referenceID, x, y, z); }
    __finally { PerfExteriorEndWrite(); }
}
static void* __fastcall PerfExteriorCtorPatch(void* map, void*)
{
    void* result;
    PerfExteriorBeginWrite();
    __try { result = ((void* (__thiscall*)(void*))PerfExteriorNativeCtor)(map); }
    __finally { PerfExteriorEndWrite(); }
    return result;
}
static void __fastcall PerfExteriorDtorPatch(void* map, void*)
{
    PerfExteriorBeginWrite();
    __try { ((void (__thiscall*)(void*))PerfExteriorNativeDtor)(map); }
    __finally { PerfExteriorEndWrite(); }
}
static __declspec(naked) void PerfExteriorNativeConsumer()
{
    __asm { mov eax, kPerfActorMainAddr }
    __asm { mov eax, [eax] }
    __asm { push kPerfExteriorConsumerContinue }
    __asm { ret }
}
static UInt8 __fastcall PerfExteriorConsumerPatch(UInt8* owner, void*, void* cell)
{
    UInt8 result;
    ++s_perfExteriorConsumerDepth;
    __try { result = ((UInt8 (__thiscall*)(UInt8*, void*))PerfExteriorNativeConsumer)(owner, cell); }
    __finally
    {
        --s_perfExteriorConsumerDepth;
        if (AbnormalTermination()) PerfExteriorInvalidate();
    }
    return result;
}
