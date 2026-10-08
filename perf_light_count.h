// LC-1. The retail RenderPass ABI has an eight-bit light count. Keep that ABI
// and bound this ordinary Lighting30 pass to one reserved +254 extra slots.
struct PerfLC1Pass
{
    void* geometry;
    UInt16 selector;
    UInt8 byte6, byte7, count, byte9, byteA, byteB;
    void** lights;
};
static_assert(sizeof(PerfLC1Pass) == 0x10 && offsetof(PerfLC1Pass, lights) == 0xC,
    "Lighting30 RenderPass ABI");
typedef UInt8* (__thiscall* PerfLC1IteratorFn)(UInt8*);
struct __declspec(align(8)) PerfLC1Counters
{
    volatile LONG64 calls, bounded, omitted, failures;
};
static PerfLC1Counters s_perfLC1Counters = {};
static bool PerfLC1Build(PerfLC1Pass* pass, UInt8* property, void* reserved,
    PerfLC1IteratorFn first = (PerfLC1IteratorFn)kPerfLC1FirstAddr,
    PerfLC1IteratorFn next = (PerfLC1IteratorFn)kPerfLC1NextAddr,
    FormHeapAllocFn allocate = (FormHeapAllocFn)kFormHeapAllocAddr,
    FormHeapFreeFn release = (FormHeapFreeFn)kFormHeapFreeAddr)
{
    PerfCount(&s_perfLC1Counters.calls);
    if (!pass || !property || pass->lights || pass->count) return false;
    // Both complete iterator traversals are intentional: they preserve the
    // native filter calls, reference temporaries and final property cursor.
    PerfUInt64 wanted = 1;
    for (UInt8* light = first(property); light; light = next(property))
        if (!light[0xF4] && wanted != ~0ull) ++wanted;
    UInt32 capacity = wanted > 255 ? 255 : (UInt32)wanted;
    void** prepared = NULL;
    bool committed = false;
    PerfUInt64 omitted = 0;
    __try
    {
        prepared = (void**)allocate(capacity * sizeof(void*));
        if (prepared)
        {
            UInt8* light = first(property);
            prepared[0] = reserved;
            UInt32 used = 1;
            for (; light; light = next(property))
            {
                if (!light[0xF4])
                {
                    if (used < capacity) prepared[used++] = light;
                    else ++omitted;
                }
            }
            // Count may shrink or grow between scans. Never let the second
            // scan exceed the actual allocation or publish an unfilled slot.
            pass->lights = prepared;
            pass->count = (UInt8)used;
            prepared = NULL;
            committed = true;
        }
    }
    __finally { if (prepared) release(prepared); }
    if (wanted > 255 || omitted) PerfCount(&s_perfLC1Counters.bounded);
    PerfCount(&s_perfLC1Counters.omitted, omitted);
    if (!committed) PerfCount(&s_perfLC1Counters.failures);
    return committed;
}
static bool __stdcall PerfLC1BuildNative(PerfLC1Pass* pass, UInt8* property, void* reserved)
{
    bool committed = false;
    __try { committed = PerfLC1Build(pass, property, reserved); }
    __finally
    {
        // The caller's constructor finished with count=0/array=null and SEH
        // state=-1. This pass has not entered any list; on failure it is ours
        // to release, including exceptions while iterating or allocating.
        if (!committed && pass) ((FormHeapFreeFn)kFormHeapFreeAddr)(pass);
    }
    return committed;
}
static __declspec(naked) void PerfLC1BuildPatch()
{
    __asm {
        pushad
        push edi
        push ebp
        push esi
        call PerfLC1BuildNative
        test al,al
        popad
        jz failed
        push kPerfLC1ContinueSite
        ret
    failed:
        // The original failure-free path would clear the dirty flag, select
        // a shader and publish the pass. Return the current cleared list and
        // retain retryability instead. Cache key differs from this request.
        mov eax,kPerfLC1RendererModeAddr
        movzx eax,word ptr [eax]
        shl eax,8
        or eax,[esp+64h]
        not eax
        mov [ebp+24h],eax
        mov dword ptr [esp+24h],0
        mov esi,ebp
        push kPerfLC1FailureSite
        ret
    }
}
