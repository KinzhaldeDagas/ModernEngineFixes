// Included after performance_fixes.h inside dllmain.cpp's private namespace.
// PERF-1/2: temporary, call-scoped state; no engine object/layout changes.
typedef int (__cdecl* PerfAMCompareFn)(const char*, const char*);
typedef UInt32 (__thiscall* PerfAMReadFn)(UInt8*, void*, UInt32);
typedef void (__thiscall* PerfAMSeekFn)(UInt8*, UInt32, UInt32);
typedef bool (*PerfAMExtentFn)(UInt8*, UInt32*);
struct PerfAMServices
{
    FormHeapAllocFn allocate, scratch;
    FormHeapFreeFn release, freeScratch;
    PerfAMCompareFn compare;
    volatile UInt32* locale;
    PerfAMReadFn read;
    PerfAMSeekFn seek;
    PerfAMExtentFn extent;
};
static const PerfAMServices s_perfAMNative = {
    (FormHeapAllocFn)kFormHeapAllocAddr, PerfScratchAllocate,
    (FormHeapFreeFn)kFormHeapFreeAddr, PerfScratchRelease,
    (PerfAMCompareFn)kPerfMasterCompareAddr, (volatile UInt32*)kPerfCRTLocaleFlagAddr,
    (PerfAMReadFn)kArchiveReadBytesAddr, (PerfAMSeekFn)kPerfAMSeekAddr, PerfStreamExtent
};
struct __declspec(align(8)) PerfAMCounters
{
    volatile LONG64 archiveCalls, archiveBytes, archiveNames, archiveRejected;
    volatile LONG64 modelCalls, modelComparisons, modelIndexed, modelRejected;
};
static PerfAMCounters s_perfAMCounters = {};

struct PerfAMArchiveContext
{
    PerfAMArchiveContext* previous;
    UInt8* archive;
    UInt32 start, size, position, ordinal, folder, prefix;
    bool initialized, failed;
};
static __declspec(thread) PerfAMArchiveContext* s_perfAMArchive = NULL;

static bool PerfAMNativeArchive(UInt8* archive)
{
    return archive && *(void**)archive == (void*)kPerfAMArchiveVtableAddr &&
        *(void**)(archive + 4) == (void*)kArchiveReadCallbackDecodeAddr &&
        *(UInt32*)(archive + 0x20) == 0 && archive[0x24] &&
        *(void**)(archive + 0x1C) && *(UInt32*)(archive + 0x30) == 0xFFFFFFFFu;
}
static bool PerfAMReadName(PerfAMArchiveContext* cursor, char* destination,
    UInt32 capacity, const PerfAMServices& service)
{
    UInt32 length = 0;
    bool fits = true;
    while (cursor->position < cursor->size)
    {
        UInt32 absolute = cursor->start + cursor->position;
        // Recursive enumeration owns a separate cursor. Reconcile the shared
        // native file position before each read, including after callbacks.
        if (*(UInt32*)(cursor->archive + 0x148) != absolute)
            service.seek(cursor->archive, absolute, 0);
        if (*(UInt32*)(cursor->archive + 0x148) != absolute) break;
        char byte = 0;
        if (service.read(cursor->archive, &byte, 1) != 1 ||
            *(UInt32*)(cursor->archive + 0x148) != absolute + 1) break;
        ++cursor->position;
        if (destination && length < capacity) destination[length] = byte;
        else if (destination) fits = false;
        ++length;
        if (!byte)
        {
            ++cursor->ordinal;
            PerfCount(&s_perfAMCounters.archiveBytes, length);
            return fits;
        }
    }
    // A truncated/unterminated table must not spin, overread, or emit a prefix
    // as a different asset. Later selected entries of this scope are skipped.
    cursor->failed = true;
    PerfCount(&s_perfAMCounters.archiveBytes, length);
    return false;
}
static const char* PerfAMArchiveName(PerfAMArchiveContext* cursor, UInt32 folder,
    UInt32 file, char (&scratch)[256], UInt32* length, const PerfAMServices& service)
{
    UInt8* archive = cursor->archive;
    UInt32 folders = *(UInt32*)(archive + 0x164);
    UInt8* records = *(UInt8**)(archive + 0x178);
    UInt32 size = *(UInt32*)(archive + 0x170);
    if (!records || folder >= folders || folders > 0xFFFFFFFFu / 16 ||
        file >= *(UInt32*)(records + folder * 16 + 8)) return NULL;
    bool cached = (archive[0x194] & 0x20) != 0;
    UInt32 policy = *(volatile UInt32*)kPerfAMRetainOffsetsAddr;
    bool offsets = policy == 1 ? (*(UInt32*)(archive + 0x160) & 0x20) != 0 : policy != 0;
    UInt32 offset = 0;
    if (cached || offsets)
    {
        UInt32** table = *(UInt32***)(archive + 0x1A4);
        if (!table || !table[folder]) return NULL;
        offset = table[folder][file];
        if (offset >= size) return NULL;
    }
    if (cached)
    {
        const char* names = *(const char**)(archive + 0x1A0);
        if (!names) return NULL;
        const char* end = (const char*)std::memchr(names + offset, 0, size - offset);
        if (!end) return NULL;
        *length = (UInt32)(end - names - offset);
        return names + offset;
    }
    if (!(*(UInt32*)(archive + 0x160) & 2) || cursor->failed) return NULL;
    UInt32 start = *(UInt32*)(archive + 0x188);
    if (!cursor->initialized)
    {
        UInt32 extent;
        if (size > 0xFFFFFFFFu - start ||
            (service.extent(archive, &extent) && (start > extent || size > extent - start)))
        { cursor->failed = true; return NULL; }
        cursor->start = start; cursor->size = size; cursor->folder = folder;
        cursor->prefix = 0; cursor->position = 0; cursor->ordinal = 0;
        for (UInt32 i = 0; i < folder; ++i)
        {
            UInt32 count = *(UInt32*)(records + i * 16 + 8);
            if (count > size - cursor->prefix) { cursor->failed = true; return NULL; }
            cursor->prefix += count;
        }
        cursor->initialized = true;
    }
    if (cursor->start != start || cursor->size != size || cursor->folder != folder)
    { cursor->failed = true; return NULL; }
    if (offsets) cursor->position = offset;
    else
    {
        if (file > size - cursor->prefix) return NULL;
        UInt32 ordinal = cursor->prefix + file;
        if (ordinal < cursor->ordinal) { cursor->position = 0; cursor->ordinal = 0; }
        while (cursor->ordinal < ordinal)
            if (!PerfAMReadName(cursor, NULL, 0, service)) return NULL;
    }
    if (!PerfAMReadName(cursor, scratch, sizeof(scratch), service)) return NULL;
    *length = (UInt32)std::strlen(scratch);
    return scratch;
}
static bool PerfAMEmitArchiveName(PerfAMArchiveContext* cursor, UInt32 folder,
    UInt32 file, PerfListNode* output, const char* prefix, const PerfAMServices& service)
{
    char scratch[256];
    UInt32 nameLength = 0;
    const char* name = PerfAMArchiveName(cursor, folder, file, scratch, &nameLength, service);
    if (!name || !output || !prefix) return false;
    size_t prefixLength = std::strlen(prefix);
    if (prefixLength > 0xFFFFFFFEu - nameLength) return false;
    UInt32 length = (UInt32)prefixLength + nameLength + 1;
    char* snapshot = (char*)service.scratch(length);
    if (!snapshot) return false;
    // Neither the borrowed cached name nor the prefix survives an engine
    // allocator callback. Complete the private copy before entering FormHeap.
    std::memcpy(snapshot, prefix, prefixLength);
    std::memcpy(snapshot + prefixLength, name, nameLength + 1);
    char* owned = NULL;
    PerfListNode* prepared = NULL;
    bool published = false;
    __try
    {
        owned = (char*)service.allocate(length);
        if (owned)
        {
            std::memcpy(owned, snapshot, length);
            if (output->data) prepared = (PerfListNode*)service.allocate(sizeof(PerfListNode));
            if (prepared || !output->data)
            {
                // Re-read the current embedded head after every allocation.
                // Original PushFront order is preserved even under recursion.
                if (output->data)
                {
                    *prepared = *output;
                    output->next = prepared;
                    prepared = NULL;
                }
                output->data = owned;
                owned = NULL;
                published = true;
            }
        }
    }
    __finally
    {
        if (prepared) service.release(prepared);
        if (owned) service.release(owned);
        service.freeScratch(snapshot);
    }
    return published;
}
static __declspec(naked) void PerfAMNativeArchiveEnumerate()
{
    __asm { push -1 }
    __asm { push kPerfAMArchiveSEHAddr }
    __asm { push kPerfAMArchiveEnumerateContinueSite }
    __asm { ret }
}
typedef void* (__cdecl* PerfAMEnumerateFn)(UInt8*, UInt32, void*, void*, void*, PerfListNode*, const char*);
static void* __cdecl PerfAMArchiveEnumerate(UInt8* archive, UInt32 count,
    void* exclusions, void* folderHash, void* filter, PerfListNode* output, const char* prefix)
{
    PerfCount(&s_perfAMCounters.archiveCalls);
    PerfAMArchiveContext cursor = {};
    cursor.previous = s_perfAMArchive; cursor.archive = archive;
    s_perfAMArchive = &cursor;
    CRITICAL_SECTION* lock = (CRITICAL_SECTION*)(archive + 0x200);
    HANDLE thread = (HANDLE)(ULONG_PTR)GetCurrentThreadId();
    LONG recursion = lock->OwningThread == thread ? lock->RecursionCount : 0;
    void* result = NULL;
    __try
    {
        result = ((PerfAMEnumerateFn)PerfAMNativeArchiveEnumerate)(archive, count,
            exclusions, folderHash, filter, output, prefix);
    }
    __finally
    {
        s_perfAMArchive = cursor.previous;
        // The native enumerator acquires this lock once and normally releases
        // it once. Its constructor-only SEH does not release it on an exception.
        if (AbnormalTermination() && lock->OwningThread == thread && lock->RecursionCount > recursion)
            LeaveCriticalSection(lock);
    }
    return result;
}
static void __stdcall PerfAMArchiveEmit(UInt8* archive, UInt32 folder, UInt32 file,
    PerfListNode* output, const char* prefix)
{
    PerfAMArchiveContext* cursor = s_perfAMArchive;
    if (!cursor || cursor->archive != archive || !PerfAMNativeArchive(archive))
    {
        // Custom stream subclasses retain their own getter/callback contract.
        // Native supported archives always take the bounded reader above.
        const char* name = ((const char* (__thiscall*)(UInt8*, UInt32, UInt32))kPerfAMArchiveGetterAddr)(archive, folder, file);
        if (!name) { PerfCount(&s_perfAMCounters.archiveRejected); return; }
        UInt32 length = (UInt32)std::strlen(name);
        size_t prefixLength = std::strlen(prefix);
        if (prefixLength > 0xFFFFFFFEu - length) return;
        // This fallback preserves native extension behavior and allocation
        // semantics; it does not claim to harden arbitrary custom callbacks.
        char* owned = (char*)((FormHeapAllocFn)kFormHeapAllocAddr)((UInt32)prefixLength + length + 1);
        if (!owned) return;
        std::memcpy(owned, prefix, prefixLength);
        std::memcpy(owned + prefixLength, name, length + 1);
        ((void (__thiscall*)(PerfListNode*, void*))kPerfAMPushFrontAddr)(output, owned);
        return;
    }
    if (PerfAMEmitArchiveName(cursor, folder, file, output, prefix, s_perfAMNative))
        PerfCount(&s_perfAMCounters.archiveNames);
    else PerfCount(&s_perfAMCounters.archiveRejected);
}
static __declspec(naked) void PerfAMArchiveEmitPatch()
{
    __asm {
        pushad
        lea eax, [esp+32]
        push dword ptr [eax+58h]
        push dword ptr [eax+54h]
        push dword ptr [eax+14h]
        push dword ptr [eax+1Ch]
        push dword ptr [eax+40h]
        call PerfAMArchiveEmit
        popad
        push kPerfAMArchiveEmitContinueSite
        ret
    }
}

struct PerfAMModelContext
{
    PerfAMModelContext* previous;
    UInt8* owner;
    const char** keys;
    UInt32 capacity, count;
    PerfListNode* tail;
    PerfUInt64 epoch;
    bool valid;
};
static __declspec(thread) PerfAMModelContext* s_perfAMModel = NULL;
static __declspec(thread) PerfUInt64 s_perfAMModelEpoch = 1;
static UInt32 PerfAMStringHash(const char* string)
{
    UInt32 hash = 2166136261u;
    for (const unsigned char* p = (const unsigned char*)string; *p; ++p)
    {
        UInt32 byte = *p;
        if (byte >= 'A' && byte <= 'Z') byte += 'a' - 'A';
        hash = (hash ^ byte) * 16777619u;
    }
    return hash;
}
static void PerfAMIndexInsert(PerfAMModelContext* context, const char* name,
    const PerfAMServices& service)
{
    UInt32 slot = PerfAMStringHash(name) & (context->capacity - 1);
    while (context->keys[slot])
    {
        PerfCount(&s_perfAMCounters.modelComparisons);
        if (!service.compare(context->keys[slot], name)) return;
        slot = (slot + 1) & (context->capacity - 1);
    }
    context->keys[slot] = name;
}
static bool PerfAMRefreshIndex(PerfAMModelContext* context, const PerfAMServices& service)
{
    if (!context || *service.locale) return false;
    if (context->valid && context->epoch == s_perfAMModelEpoch &&
        context->count < context->capacity / 2) return true;
    context->valid = false;
    PerfListNode* head = (PerfListNode*)(context->owner + 4);
    UInt32 count = 0;
    PerfListNode* fast = head;
    for (PerfListNode* p = head; p; p = p->next)
    {
        if (++count > 0x200000u) return false;
        fast = fast && fast->next ? fast->next->next : NULL;
        if (fast && fast == p->next) return false;
    }
    UInt32 capacity;
    if (!PerfTableCapacity(count + 1, &capacity)) return false;
    if (capacity > context->capacity)
    {
        const char** keys = (const char**)service.scratch(capacity * sizeof(char*));
        if (!keys) return false;
        if (context->keys) service.freeScratch(context->keys);
        context->keys = keys; context->capacity = capacity;
    }
    // Fresh traversal after scratch callbacks. A changing owner cannot publish
    // borrowed keys or a tail based on the earlier traversal.
    PerfUInt64 epoch = s_perfAMModelEpoch;
    std::memset(context->keys, 0, context->capacity * sizeof(char*));
    context->count = 0; context->tail = NULL;
    for (PerfListNode* p = head; p; p = p->next)
    {
        if (++context->count >= context->capacity / 2) return false;
        if (p->data) PerfAMIndexInsert(context, (const char*)p->data, service);
        context->tail = p;
    }
    if (*service.locale || epoch != s_perfAMModelEpoch) return false;
    context->epoch = epoch; context->valid = true;
    return true;
}
static bool PerfAMFindModel(UInt8* owner, const char* name, PerfListNode** tail,
    PerfAMModelContext* context, const PerfAMServices& service)
{
    if (PerfAMRefreshIndex(context, service))
    {
        UInt32 slot = PerfAMStringHash(name) & (context->capacity - 1);
        bool found = false;
        while (context->keys[slot])
        {
            PerfCount(&s_perfAMCounters.modelComparisons);
            if (!service.compare(context->keys[slot], name)) { found = true; break; }
            slot = (slot + 1) & (context->capacity - 1);
        }
        if (!*service.locale && context->epoch == s_perfAMModelEpoch)
        {
            *tail = context->tail;
            PerfCount(&s_perfAMCounters.modelIndexed);
            return found;
        }
        context->valid = false;
    }
    *tail = NULL;
    for (PerfListNode* p = (PerfListNode*)(owner + 4); p; p = p->next)
    {
        *tail = p;
        if (p->data)
        {
            PerfCount(&s_perfAMCounters.modelComparisons);
            if (!service.compare((const char*)p->data, name)) return true;
        }
    }
    return false;
}
static bool PerfAMAddModel(UInt8* owner, const char* name, const PerfAMServices& service)
{
    PerfCount(&s_perfAMCounters.modelCalls);
    if (!owner || !name) return false;
    PerfAMModelContext* context = s_perfAMModel;
    while (context && context->owner != owner) context = context->previous;
    PerfListNode* tail = NULL;
    if (PerfAMFindModel(owner, name, &tail, context, service)) return true;
    size_t length = std::strlen(name);
    if (length >= 0xFFFFFFFFu) return false;
    char* snapshot = (char*)service.scratch((UInt32)length + 1);
    if (!snapshot) return false;
    std::memcpy(snapshot, name, length + 1);
    char* owned = NULL;
    PerfListNode* prepared = NULL;
    bool done = false;
    __try
    {
        owned = (char*)service.allocate((UInt32)length + 1);
        if (owned)
        {
            std::memcpy(owned, snapshot, length + 1);
            bool duplicate = PerfAMFindModel(owner, snapshot, &tail, context, service);
            if (!duplicate && tail->data)
            {
                prepared = (PerfListNode*)service.allocate(sizeof(PerfListNode));
                duplicate = PerfAMFindModel(owner, snapshot, &tail, context, service);
            }
            if (duplicate || prepared || !tail->data)
            {
                // Engine allocators may recursively clear/add to the owner.
                // Both interfaces invalidate scoped keys before any free.
                if (!PerfAMFindModel(owner, snapshot, &tail, context, service))
                {
                    if (tail->data)
                    {
                        prepared->data = owned; prepared->next = NULL;
                        tail->next = prepared; tail = prepared; prepared = NULL;
                    }
                    else tail->data = owned;
                    ++s_perfAMModelEpoch;
                    if (context && context->valid && !*service.locale &&
                        context->count + 1 < context->capacity / 2)
                    {
                        PerfAMIndexInsert(context, owned, service);
                        context->tail = tail; ++context->count;
                        context->epoch = s_perfAMModelEpoch;
                    }
                    else if (context) context->valid = false;
                    owned = NULL;
                }
                done = true;
            }
        }
    }
    __finally
    {
        if (prepared) service.release(prepared);
        if (owned) service.release(owned);
        service.freeScratch(snapshot);
    }
    if (!done) PerfCount(&s_perfAMCounters.modelRejected);
    return done;
}
static void __stdcall PerfAMAddModelNative(UInt8* owner, const char* name)
{
    PerfAMAddModel(owner, name, s_perfAMNative);
}
static __declspec(naked) void PerfAMAddModelPatch()
{
    __asm { push dword ptr [esp+4] }
    __asm { push ecx }
    __asm { call PerfAMAddModelNative }
    __asm { ret 4 }
}
static void PerfAMClearPaths(UInt8* owner, const PerfAMServices& service)
{
    ++s_perfAMModelEpoch;
    __try
    {
        PerfListNode* head = (PerfListNode*)(owner + 4);
        while (head->next || head->data)
        {
            void* payload = head->data;
            PerfListNode* next = head->next;
            if (next)
            {
                *head = *next;
                service.release(next);
            }
            else head->data = NULL;
            service.release(payload);
        }
    }
    __finally { ++s_perfAMModelEpoch; }
}
static void __stdcall PerfAMClearModelNative(UInt8* owner)
{
    PerfAMClearPaths(owner, s_perfAMNative);
    ((void (__thiscall*)(UInt8*))kPerfAMClearTexturesAddr)(owner + 0xC);
}
static __declspec(naked) void PerfAMClearModelPatch()
{
    __asm { push ecx }
    __asm { call PerfAMClearModelNative }
    __asm { ret }
}
static void PerfAMNifzBatch(UInt8* owner, const char* bytes, UInt32 size,
    bool complete, const PerfAMServices& service)
{
    if (!complete || !bytes || size > 0xFFFFFFFDu) return;
    PerfAMModelContext context = {};
    context.previous = s_perfAMModel; context.owner = owner;
    s_perfAMModel = &context;
    __try
    {
        // v55 provides two initialized sentinels. The read wrapper initializes
        // the full extent too, and failed reads do not enter this loop.
        UInt32 offset = 0;
        while (offset <= size && bytes[offset])
        {
            const char* end = (const char*)std::memchr(bytes + offset, 0, size + 1 - offset);
            if (!end) break;
            if (!PerfAMAddModel(owner, bytes + offset, service)) break;
            offset = (UInt32)(end - bytes) + 1;
        }
    }
    __finally
    {
        s_perfAMModel = context.previous;
        if (context.keys) service.freeScratch(context.keys);
    }
}
struct PerfAMNifzExtent { char* buffer; UInt32 size; bool complete; };
static __declspec(thread) PerfAMNifzExtent s_perfAMNifzExtent = {};
static UInt8* __cdecl PerfAMAllocateNifz(UInt32 size)
{
    UInt8* buffer = AllocateDoubleSentinelBuffer(size);
    // Publish after allocation returns: recursive loads have already completed
    // their own allocation/read pair, and cannot replace this outer extent.
    s_perfAMNifzExtent.buffer = (char*)buffer;
    s_perfAMNifzExtent.size = size;
    s_perfAMNifzExtent.complete = false;
    return buffer;
}
static __declspec(naked) void PerfAMAllocateNifzPatch()
{
    __asm {
        push dword ptr [esp+4]
        call PerfAMAllocateNifz
        add esp,4
        test eax,eax
        jnz allocated
        mov edx,kTESCreatureNIFZAllocFailureSite
        mov dword ptr [esp],edx
        ret 4
    allocated:
        ret
    }
}
static char __stdcall PerfAMReadNifz(UInt8* file, char* bytes, UInt32 maxSize)
{
    UInt32 size = s_perfAMNifzExtent.size;
    if (!bytes || s_perfAMNifzExtent.buffer != bytes || size > 0xFFFFFFFDu || maxSize ||
        *(UInt32*)(file + 0x254) > size) return 0;
    std::memset(bytes, 0, size + 2);
    // Retain the allocation's extent across diagnostics or nested reads. The
    // explicit upper bound also protects a chunk-size change during callbacks.
    char status = ((TESFileGetChunkDataFn)kTESFileGetChunkDataAddr)(file, bytes, size + 1);
    s_perfAMNifzExtent.buffer = bytes;
    s_perfAMNifzExtent.size = size;
    s_perfAMNifzExtent.complete = status != 0;
    return status;
}
static __declspec(naked) void PerfAMReadNifzPatch()
{
    __asm { push dword ptr [esp+8] }
    __asm { push dword ptr [esp+8] }
    __asm { push ecx }
    __asm { call PerfAMReadNifz }
    __asm { ret 8 }
}
static void __stdcall PerfAMNifzNative(UInt8* owner, const char* bytes, UInt32 size, UInt32 status)
{
    bool complete = (status & 0xFF) != 0 && s_perfAMNifzExtent.buffer == bytes && s_perfAMNifzExtent.complete;
    size = s_perfAMNifzExtent.size;
    s_perfAMNifzExtent = {};
    PerfAMNifzBatch(owner, bytes, size, complete, s_perfAMNative);
}
static __declspec(naked) void PerfAMNifzBatchPatch()
{
    __asm {
        pushad
        push eax
        push dword ptr [edi+254h]
        push ebx
        push ebp
        call PerfAMNifzNative
        popad
        push kPerfAMNifzDoneSite
        ret
    }
}
static __declspec(naked) void PerfAMNativeCopyModel()
{
    __asm { mov eax, dword ptr [esp+4] }
    __asm { push esi }
    __asm { push edi }
    __asm { push kPerfAMCopyContinueSite }
    __asm { ret }
}
static void __stdcall PerfAMCopyModel(UInt8* owner, void* source)
{
    PerfAMModelContext context = {};
    context.previous = s_perfAMModel; context.owner = owner;
    s_perfAMModel = &context;
    __try { ((void (__thiscall*)(UInt8*, void*))PerfAMNativeCopyModel)(owner, source); }
    __finally
    {
        s_perfAMModel = context.previous;
        if (context.keys) s_perfAMNative.freeScratch(context.keys);
    }
}
static __declspec(naked) void PerfAMCopyModelPatch()
{
    __asm { push dword ptr [esp+4] }
    __asm { push ecx }
    __asm { call PerfAMCopyModel }
    __asm { ret 4 }
}
