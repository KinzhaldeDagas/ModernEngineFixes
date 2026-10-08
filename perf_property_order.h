// PERF-14. Included inside the private namespace after performance_fixes.h.
// The caller bridge scopes state to one native invocation. All native outer
// scores, float stores, temporary references, moves and invalidation remain.
static __declspec(align(8)) volatile LONG64 s_perfPropertyEpoch;
static void PerfPropertyInvalidate() { InterlockedIncrement64(&s_perfPropertyEpoch); }
static PerfUInt64 PerfPropertyReadEpoch()
{ return (PerfUInt64)InterlockedCompareExchange64(&s_perfPropertyEpoch, 0, 0); }
struct PerfPropertyOrderContext
{
    PerfPropertyOrderContext* previous;
    UInt8* property;
    const float* bound;
    UInt32 boundBits[4];
    PerfRefList snapshot;
    PerfRefNode* expected;
    PerfRefNode* previousNode;
    PerfUInt64 epoch;
    UInt32 processed;
    float maximum;
    UInt16 controlWord;
    bool enabled;
};
static __declspec(thread) PerfPropertyOrderContext* s_perfPropertyOrderContext;
struct __declspec(align(8)) PerfPropertyOrderCounters
{
    volatile LONG64 calls, scoped, nodes, predecessorScoresAvoided, disabled;
};
static PerfPropertyOrderCounters s_perfPropertyOrderCounters = {};

static UInt16 PerfPropertyControlWord()
{
    UInt16 result;
    __asm { fnstcw result }
    return result;
}
static bool PerfPropertySourceSupported(UInt8* wrapper)
{
    if (!wrapper) return false;
    UInt8* source = *(UInt8**)(wrapper + 0x100);
    if (!source) return false;
    // The getter is nonvirtual and adds a temporary reference to the strongly
    // held +100 source. Positive ordinary ownership keeps its paired releases
    // from entering a destructor in this supported, stable native scene view.
    LONG references = *(volatile LONG*)(source + 4);
    if (references <= 0 || references > 0x7FFFFFFDL) return false;
    float denominator = *(float*)(source + 0xF8);
    return PerfFiniteScore(denominator) && denominator != 0.0f &&
        PerfFiniteScore(*(float*)(source + 0x88)) &&
        PerfFiniteScore(*(float*)(source + 0x8C)) &&
        PerfFiniteScore(*(float*)(source + 0x90));
}
static bool PerfBeginPropertyOrder(PerfPropertyOrderContext* context)
{
    if (!context->property || !context->bound) return false;
    for (UInt32 i = 0; i < 4; ++i)
        if (!PerfFiniteScore(context->bound[i])) return false;
    auto list = (PerfRefList*)(context->property + 0x6C);
    context->epoch = PerfPropertyReadEpoch();
    context->snapshot = *list;
    if (list->count < 16 || list->count > 0x200000u || !list->head || !list->tail)
        return false;
    PerfRefNode* previous = NULL;
    PerfRefNode* node = list->head;
    for (UInt32 i = 0; i < context->snapshot.count; ++i)
    {
        if (!node || node->prev != previous || !PerfPropertySourceSupported(node->light)) return false;
        previous = node; node = node->next;
    }
    if (node || previous != context->snapshot.tail ||
        std::memcmp(list, &context->snapshot, sizeof(*list)) ||
        context->epoch != PerfPropertyReadEpoch()) return false;
    std::memcpy(context->boundBits, context->bound, sizeof(context->boundBits));
    context->expected = list->head;
    context->controlWord = PerfPropertyControlWord();
    return true;
}
static bool __stdcall PerfPropertyCanSkipPrefix(UInt8* nativeFrame, PerfRefNode* current)
{
    auto context = s_perfPropertyOrderContext;
    if (!context || !context->enabled) return false;
    auto list = (PerfRefList*)(context->property + 0x6C);
    const float score = *(float*)(nativeFrame + 0x34); // native rounded FSTP
    bool stable = *(UInt8**)(nativeFrame + 0x14) == context->property &&
        *(const float**)(nativeFrame + 0x58) == context->bound &&
        context->epoch == PerfPropertyReadEpoch() &&
        context->controlWord == PerfPropertyControlWord() &&
        !std::memcmp(context->boundBits, context->bound, sizeof(context->boundBits)) &&
        !std::memcmp(list, &context->snapshot, sizeof(*list)) &&
        current && current == context->expected && current->prev == context->previousNode &&
        current->next == *(PerfRefNode**)(nativeFrame + 0x18) &&
        context->processed < context->snapshot.count &&
        PerfPropertySourceSupported(current->light) && PerfFiniteScore(score);
    // A one-way rejection is intentional: after any native insertion, the
    // previously visited original node is not necessarily the prefix maximum.
    // Finish that call natively instead of using an invalid cached maximum.
    if (!stable || (context->processed && PerfScoreGreater(context->maximum, score)))
    {
        context->enabled = false;
        PerfCount(&s_perfPropertyOrderCounters.disabled);
        return false;
    }
    PerfCount(&s_perfPropertyOrderCounters.nodes);
    PerfCount(&s_perfPropertyOrderCounters.predecessorScoresAvoided, context->processed);
    context->maximum = score;
    ++context->processed;
    context->previousNode = current;
    context->expected = current->next;
    return true;
}
static __declspec(naked) void PerfPropertyPrefixPatch()
{
    __asm {
        // Both outer source temporaries have already been released. Native
        // x87 stack is empty; original key is the float at [ESP+34h].
        pushad
        lea eax, [esp+20h]
        push edi
        push eax
        call PerfPropertyCanSkipPrefix
        test al, al
        popad
        jnz nextOuter
        cmp esi, edi
        jz nextOuter
        push kPerfPropertyPrefixContinue
        ret
    nextOuter:
        push kPerfPropertyNextOuter
        ret
    }
}
static void __stdcall PerfPropertyOrderInvoke(UInt8* property, const float* bound)
{
    PerfCount(&s_perfPropertyOrderCounters.calls);
    PerfPropertyOrderContext context = {};
    context.previous = s_perfPropertyOrderContext;
    if (context.previous) context.previous->enabled = false;
    context.property = property; context.bound = bound;
    context.enabled = PerfBeginPropertyOrder(&context);
    if (context.enabled) PerfCount(&s_perfPropertyOrderCounters.scoped);
    s_perfPropertyOrderContext = &context;
    __try
    {
        ((void (__thiscall*)(UInt8*, const float*))kPerfPropertyNativeOrderAddr)(property, bound);
    }
    __finally
    {
        s_perfPropertyOrderContext = context.previous;
    }
}
static __declspec(naked) void PerfPropertyOrderCallPatch()
{
    __asm {
        pop eax
        push ecx
        push eax
        jmp PerfPropertyOrderInvoke
    }
}
