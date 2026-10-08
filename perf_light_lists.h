// PERF-9/12/13. Native ref-list mutation interfaces maintain bounded
// auxiliary indices. No plugin pointer or layout is written into an engine object.
struct IndexedLightLightEntry { UInt32 key; PerfRefNode* node; UInt32 state; };
struct IndexedLightLightCache
{
    PerfRefList* list;
    void* vtable;
    PerfRefNode* head;
    PerfRefNode* tail;
    UInt32 count, capacity;
    IndexedLightLightEntry* entries;
    bool payloadReady, sourceReady, attempted, sourceAttempted, sourceOwner;
    PerfUInt64 serial, lastUse;
};
struct IndexedLightReceiverScope
{
    UInt8* owner;
    PerfRefList* list;
    PerfRefNode* fence;
    IndexedLightLightEntry* entries;
    UInt32 capacity;
    LONG64 epoch;
    bool valid;
};
static INIT_ONCE s_indexedLightOnce = INIT_ONCE_STATIC_INIT;
static CRITICAL_SECTION s_indexedLightLock;
static IndexedLightLightCache s_indexedLightCaches[4] = {};
static PerfUInt64 s_indexedLightUseClock;
static DWORD s_indexedLightThread;
static DWORD (__cdecl* s_indexedLightThreadProvider)() = NULL;
static volatile LONG s_indexedLightWriters;
static __declspec(align(8)) volatile LONG64 s_indexedLightEpoch;
static __declspec(thread) IndexedLightReceiverScope s_indexedReceiverScopes[4];
static void (__cdecl* s_perfLightListsMutationObserver)() = NULL;
struct __declspec(align(8)) IndexedLightLightCounters
{
    volatile LONG64 sourceHits, sourceFallbacks, removeHits, removeFallbacks;
    volatile LONG64 receiverHits, receiverMisses, receiverFallbacks, builds;
};
static IndexedLightLightCounters s_indexedLightCounters = {};
static BOOL CALLBACK IndexedLightInitLightLock(PINIT_ONCE, PVOID, PVOID*)
{ return InitializeCriticalSectionAndSpinCount(&s_indexedLightLock, 1000); }
static bool IndexedLightLockLights()
{
    if (!InitOnceExecuteOnce(&s_indexedLightOnce, IndexedLightInitLightLock, NULL, NULL))
        return false;
    EnterCriticalSection(&s_indexedLightLock);
    return true;
}
static void IndexedLightUnlockLights() { LeaveCriticalSection(&s_indexedLightLock); }
static LONG64 PerfLightListsEpoch()
{ return InterlockedCompareExchange64(&s_indexedLightEpoch, 0, 0); }
static void IndexedLightLightNotify()
{
    InterlockedIncrement64(&s_indexedLightEpoch);
    if (s_perfLightListsMutationObserver) s_perfLightListsMutationObserver();
}
static bool IndexedLightLightThreadAllowed()
{
    DWORD thread=s_indexedLightThreadProvider ? s_indexedLightThreadProvider() : s_indexedLightThread;
    return thread && GetCurrentThreadId()==thread;
}
static bool IndexedLightKnownRefList(PerfRefList* list)
{
    return list && (list->vtable == (void*)kIndexedLightFullLightListVtable ||
        list->vtable == (void*)kIndexedLightReceiverListVtable);
}
static IndexedLightLightEntry* IndexedLightFindEntry(IndexedLightLightEntry* table, UInt32 cap, UInt32 key)
{
    if (!table || !cap) return NULL;
    UInt32 slot = PerfHash(key) & (cap-1);
    for (UInt32 n=0; n<cap; ++n, slot=(slot+1)&(cap-1))
    {
        IndexedLightLightEntry* e=table+slot;
        if (!e->state) return NULL;
        if (e->state==1 && e->key==key) return e;
    }
    return NULL;
}
static bool IndexedLightInsertEntry(IndexedLightLightEntry* table, UInt32 cap, UInt32 key, PerfRefNode* node)
{
    UInt32 slot=PerfHash(key)&(cap-1), vacant=cap;
    for(UInt32 n=0;n<cap;++n,slot=(slot+1)&(cap-1))
    {
        IndexedLightLightEntry* e=table+slot;
        if(e->state==1 && e->key==key) return e->node==node;
        if(e->state!=1 && vacant==cap) vacant=slot;
        if(!e->state) break;
    }
    if(vacant==cap) return false;
    table[vacant]={key,node,1}; return true;
}
static void IndexedLightEraseEntry(IndexedLightLightEntry* table, UInt32 cap, UInt32 key, PerfRefNode* node)
{
    IndexedLightLightEntry* e=IndexedLightFindEntry(table,cap,key);
    if(!e || e->node!=node) return;
    // Backshift deletion keeps probe chains intact without accumulating
    // tombstones during long-lived remove/add churn.
    UInt32 hole=(UInt32)(e-table), scan=(hole+1)&(cap-1);
    for(UInt32 n=0;n<cap-1 && table[scan].state;++n,scan=(scan+1)&(cap-1))
    {
        UInt32 home=PerfHash(table[scan].key)&(cap-1);
        if(((hole-home)&(cap-1)) < ((scan-home)&(cap-1)))
        { table[hole]=table[scan]; hole=scan; }
    }
    table[hole]={};
}
static void IndexedLightCaptureList(IndexedLightLightCache* cache, PerfRefList* list)
{
    cache->list=list; cache->vtable=list->vtable; cache->head=list->head;
    cache->tail=list->tail; cache->count=list->count;
}
static bool IndexedLightSameList(const IndexedLightLightCache* c, const PerfRefList* list)
{
    return c->list==list && c->vtable==list->vtable && c->head==list->head &&
        c->tail==list->tail && c->count==list->count;
}
static bool IndexedLightStableSourceKey(PerfRefNode* node, UInt32* key)
{
    if(!node || !node->light) return false;
    LONG wrapperRefs=*(volatile LONG*)(node->light+4);
    if(wrapperRefs<=0 || wrapperRefs>0x7FFFFFFDL) return false;
    UInt8* source=*(UInt8**)(node->light+0x100);
    if(source)
    {
        LONG sourceRefs=*(volatile LONG*)(source+4);
        if(sourceRefs<=0 || sourceRefs>0x7FFFFFFDL) return false;
    }
    *key=(UInt32)source; return true;
}
static void IndexedLightDropCache(IndexedLightLightCache* c)
{ c->payloadReady=c->sourceReady=c->attempted=c->sourceAttempted=false; ++c->serial; }
static IndexedLightLightCache* IndexedLightFindCache(PerfRefList* list)
{
    for(UInt32 i=0;i<4;++i) if(s_indexedLightCaches[i].list==list) return s_indexedLightCaches+i;
    return NULL;
}
static bool IndexedLightBuildCache(IndexedLightLightCache* c, PerfRefList* list, bool source)
{
    IndexedLightCaptureList(c,list); c->attempted=true;
    c->payloadReady=c->sourceReady=false; c->sourceAttempted=source;
    if(!IndexedLightKnownRefList(list) || list->count<32 || list->count>32768) return false;
    UInt32 cap=128;
    while(cap<list->count*2) cap*=2;
    if(cap>c->capacity)
    {
        // OS process heap only: no native allocator/destructor callback under lock.
        IndexedLightLightEntry* p=(IndexedLightLightEntry*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
            2*cap*sizeof(IndexedLightLightEntry));
        if(!p) return false;
        if(c->entries) HeapFree(GetProcessHeap(),0,c->entries);
        c->entries=p; c->capacity=cap;
    }
    std::memset(c->entries,0,2*c->capacity*sizeof(IndexedLightLightEntry));
    PerfRefNode* node=list->head; PerfRefNode* previous=NULL;
    bool sourceUnique=true;
    for(UInt32 i=0;i<list->count;++i)
    {
        if(!node || node->prev!=previous || !node->light ||
            !IndexedLightInsertEntry(c->entries,c->capacity,(UInt32)node->light,node)) return false;
        if(source)
        {
            UInt32 sourceKey;
            if(!IndexedLightStableSourceKey(node,&sourceKey) ||
                !IndexedLightInsertEntry(c->entries+c->capacity,c->capacity,sourceKey,node)) sourceUnique=false;
        }
        previous=node; node=node->next;
    }
    if(node || previous!=list->tail) return false;
    c->payloadReady=true; c->sourceReady=source && sourceUnique;
    PerfCount(&s_indexedLightCounters.builds); return true;
}
static bool IndexedLightLookupLight(PerfRefList* list, UInt32 key, bool source, PerfRefNode** result)
{
    *result=NULL;
    if(!IndexedLightLightThreadAllowed() || !IndexedLightKnownRefList(list) ||
        (source && list->vtable!=(void*)kIndexedLightFullLightListVtable) ||
        InterlockedCompareExchange(&s_indexedLightWriters,0,0)) return false;
    bool handled=false;
    if(!IndexedLightLockLights()) return false;
    if(!s_indexedLightWriters)
    {
        IndexedLightLightCache* c=IndexedLightFindCache(list);
        if(!c)
        {
            c=s_indexedLightCaches;
            for(UInt32 i=0;i<4;++i)
            {
                IndexedLightLightCache* candidate=s_indexedLightCaches+i;
                if(!candidate->list) { c=candidate; break; }
                if((c->sourceOwner && !candidate->sourceOwner) ||
                    (c->sourceOwner==candidate->sourceOwner && candidate->lastUse<c->lastUse)) c=candidate;
            }
            IndexedLightDropCache(c); c->list=list; c->sourceOwner=false;
        }
        c->lastUse=++s_indexedLightUseClock;
        if(source) c->sourceOwner=true;
        if(!IndexedLightSameList(c,list))
        {
            IndexedLightDropCache(c); IndexedLightCaptureList(c,list);
            if(list->vtable!=(void*)kIndexedLightFullLightListVtable) c->sourceOwner=false;
        }
        if(!c->attempted || (source && !c->sourceAttempted)) IndexedLightBuildCache(c,list,source);
        handled=source ? c->sourceReady : c->payloadReady;
        if(handled)
        {
            IndexedLightLightEntry* e=IndexedLightFindEntry(c->entries+(source?c->capacity:0),c->capacity,key);
            if(e) *result=e->node;
        }
    }
    IndexedLightUnlockLights(); return handled;
}
enum IndexedLightListOperation { IndexedLightAddHead, IndexedLightAddTail, IndexedLightInsertBefore, IndexedLightRemove, IndexedLightMove, IndexedLightClear };
struct IndexedLightListEdit
{
    PerfRefList* list; IndexedLightLightCache* cache; PerfUInt64 serial;
    UInt32 count; IndexedLightListOperation operation; PerfRefNode* node;
};
static void IndexedLightScopeMutation(PerfRefList* list,IndexedLightListOperation operation,
    PerfRefNode* node,PerfRefNode* before,LONG64 oldEpoch)
{
    for(UInt32 i=0;i<4;++i)
    {
        IndexedLightReceiverScope* s=s_indexedReceiverScopes+i;
        if(!s->valid) continue;
        if(s->epoch!=oldEpoch || s->list!=list) { s->valid=false; continue; }
        s->epoch=PerfLightListsEpoch();
        if(operation==IndexedLightAddHead) continue; // new accepted nodes precede fence
        if(operation==IndexedLightInsertBefore && before==s->fence) continue;
        if(operation==IndexedLightMove && before==s->fence && node!=before)
        { IndexedLightEraseEntry(s->entries,s->capacity,(UInt32)node->light,node); continue; }
        if(operation==IndexedLightRemove && node && node!=s->fence)
        { IndexedLightEraseEntry(s->entries,s->capacity,(UInt32)node->light,node); continue; }
        s->valid=false;
    }
}
static IndexedLightListEdit IndexedLightBeginListEdit(PerfRefList* list,IndexedLightListOperation op,
    PerfRefNode* node=NULL,PerfRefNode* before=NULL)
{
    InterlockedIncrement(&s_indexedLightWriters);
    LONG64 epoch=PerfLightListsEpoch(); IndexedLightLightNotify();
    IndexedLightScopeMutation(list,op,node,before,epoch);
    IndexedLightListEdit edit={list,NULL,0,list->count,op,node};
    if(!IndexedLightLockLights()) return edit;
    IndexedLightLightCache* c=IndexedLightFindCache(list);
    if(c)
    {
        if(!IndexedLightSameList(c,list)) IndexedLightDropCache(c);
        edit.cache=c; edit.serial=++c->serial;
        if(op==IndexedLightClear) IndexedLightDropCache(c);
        else if(op==IndexedLightRemove && node && c->payloadReady)
        {
            IndexedLightEraseEntry(c->entries,c->capacity,(UInt32)node->light,node);
            if(c->sourceReady) IndexedLightEraseEntry(c->entries+c->capacity,c->capacity,
                *(UInt32*)(node->light+0x100),node);
        }
    }
    IndexedLightUnlockLights(); return edit;
}
static void IndexedLightEndListEdit(IndexedLightListEdit* edit,bool succeeded,PerfRefNode* inserted=NULL)
{
    if(!IndexedLightLockLights()) { InterlockedDecrement(&s_indexedLightWriters); return; }
    IndexedLightLightCache* c=edit->cache; PerfRefList* list=edit->list;
    if(c && c->list==list)
    {
        bool ready=succeeded && c->serial==edit->serial && c->payloadReady;
        if(edit->operation==IndexedLightAddHead || edit->operation==IndexedLightAddTail || edit->operation==IndexedLightInsertBefore)
        {
            ready=ready && list->count==edit->count+1 && inserted && inserted->light &&
                list->count<=c->capacity/2;
            if(ready) ready=IndexedLightInsertEntry(c->entries,c->capacity,(UInt32)inserted->light,inserted);
            if(ready && c->sourceReady)
            {
                UInt32 sourceKey;
                if(!IndexedLightStableSourceKey(inserted,&sourceKey) ||
                    !IndexedLightInsertEntry(c->entries+c->capacity,c->capacity,sourceKey,inserted)) c->sourceReady=false;
            }
        }
        else if(edit->operation==IndexedLightRemove) ready=ready && edit->count && list->count==edit->count-1;
        else if(edit->operation==IndexedLightMove) ready=ready && list->count==edit->count;
        else ready=false;
        if(!ready) IndexedLightDropCache(c);
        IndexedLightCaptureList(c,list);
    }
    IndexedLightUnlockLights(); InterlockedDecrement(&s_indexedLightWriters);
}
static void __cdecl PerfLightListsInvalidateExternal()
{
    IndexedLightLightNotify();
    for(UInt32 i=0;i<4;++i) s_indexedReceiverScopes[i].valid=false;
    if(!IndexedLightLockLights()) return;
    for(UInt32 i=0;i<4;++i) IndexedLightDropCache(s_indexedLightCaches+i);
    IndexedLightUnlockLights();
}
static void IndexedLightInvalidateSource(UInt8* wrapper)
{
    IndexedLightLightNotify();
    for(UInt32 i=0;i<4;++i) s_indexedReceiverScopes[i].valid=false;
    if(!IndexedLightLockLights()) return;
    for(UInt32 i=0;i<4;++i)
    {
        IndexedLightLightCache* c=s_indexedLightCaches+i;
        if(c->payloadReady && IndexedLightFindEntry(c->entries,c->capacity,(UInt32)wrapper))
        { c->sourceReady=c->sourceAttempted=false; ++c->serial; }
    }
    IndexedLightUnlockLights();
}

// Trampolines replay only complete nonrelative prolog instructions; signatures
// and every continuation are declared in dllmain.cpp for the transaction verifier.
static __declspec(naked) void IndexedLightNativeAddTail()
{ __asm { push ebx } __asm { push ebp } __asm { push esi } __asm { mov esi,ecx }
  __asm { push kIndexedLightAddTailContinue } __asm { ret } }
static __declspec(naked) void IndexedLightNativeAddHead()
{ __asm { push ebx } __asm { push ebp } __asm { push esi } __asm { mov esi,ecx }
  __asm { push kIndexedLightAddHeadContinue } __asm { ret } }
static __declspec(naked) void IndexedLightNativeInsertBefore()
{ __asm { push ebx } __asm { push ebp } __asm { mov ebx,ecx } __asm { mov eax,[ebx] }
  __asm { push kIndexedLightInsertBeforeContinue } __asm { ret } }
static __declspec(naked) void IndexedLightNativeRemovePosition()
{ __asm { push 0FFFFFFFFh } __asm { push kIndexedLightRemovePositionHandler }
  __asm { push kIndexedLightRemovePositionContinue } __asm { ret } }
static __declspec(naked) void IndexedLightNativeRemoveHead()
{ __asm { push 0FFFFFFFFh } __asm { push kIndexedLightRemoveHeadHandler }
  __asm { push kIndexedLightRemoveHeadContinue } __asm { ret } }
static __declspec(naked) void IndexedLightNativeRemoveTail()
{ __asm { push 0FFFFFFFFh } __asm { push kIndexedLightRemoveTailHandler }
  __asm { push kIndexedLightRemoveTailContinue } __asm { ret } }
static __declspec(naked) void IndexedLightNativeClearList()
{ __asm { push esi } __asm { push edi } __asm { mov edi,ecx } __asm { mov esi,[edi+4] }
  __asm { push kIndexedLightClearListContinue } __asm { ret } }
static __declspec(naked) void IndexedLightNativeMoveBefore()
{ __asm { mov eax,[esp+4] } __asm { mov edx,[esp+8] }
  __asm { push kIndexedLightMoveBeforeContinue } __asm { ret } }
static __declspec(naked) void IndexedLightNativeSetSource()
{ __asm { push ebx } __asm { push esi } __asm { mov ebx,ecx } __asm { mov esi,[ebx+100h] }
  __asm { push kIndexedLightSetSourceContinue } __asm { ret } }
static __declspec(naked) void IndexedLightNativeFindSource()
{ __asm { sub esp,8 } __asm { push ebx } __asm { push ebp } __asm { xor ebx,ebx }
  __asm { push kIndexedLightFindSourceContinue } __asm { ret } }
static __declspec(naked) void IndexedLightNativeRemoveValue()
{ __asm { push 0FFFFFFFFh } __asm { push kIndexedLightRemoveValueHandler }
  __asm { push kIndexedLightRemoveValueContinue } __asm { ret } }
static __declspec(naked) void IndexedLightNativeBeginReceivers()
{ __asm { push 0FFFFFFFFh } __asm { push kIndexedLightBeginReceiversHandler }
  __asm { push kIndexedLightBeginReceiversContinue } __asm { ret } }
static __declspec(naked) void IndexedLightNativeEndReceivers()
{ __asm { push esi } __asm { push edi } __asm { mov edi,ecx } __asm { mov esi,[edi+144h] }
  __asm { push kIndexedLightEndReceiversContinue } __asm { ret } }

typedef UInt32 (__thiscall* IndexedLightListAddFn)(PerfRefList*,UInt8**);
typedef UInt32 (__thiscall* IndexedLightListInsertFn)(PerfRefList*,PerfRefNode*,UInt8**);
typedef UInt8** (__thiscall* IndexedLightListRemoveFn)(PerfRefList*,UInt8**);
static UInt32 __fastcall IndexedLightAddTailPatch(PerfRefList* list,void*,UInt8** payload)
{
    IndexedLightListEdit e=IndexedLightBeginListEdit(list,IndexedLightAddTail); bool done=false; UInt32 result=0;
    __try { result=((IndexedLightListAddFn)IndexedLightNativeAddTail)(list,payload); done=true; }
    __finally { IndexedLightEndListEdit(&e,done,done?list->tail:NULL); }
    return result;
}
static UInt32 __fastcall IndexedLightAddHeadPatch(PerfRefList* list,void*,UInt8** payload)
{
    IndexedLightListEdit e=IndexedLightBeginListEdit(list,IndexedLightAddHead); bool done=false; UInt32 result=0;
    __try { result=((IndexedLightListAddFn)IndexedLightNativeAddHead)(list,payload); done=true; }
    __finally { IndexedLightEndListEdit(&e,done,done?list->head:NULL); }
    return result;
}
static UInt32 __fastcall IndexedLightInsertBeforePatch(PerfRefList* list,void*,PerfRefNode* before,UInt8** payload)
{
    IndexedLightListEdit e=IndexedLightBeginListEdit(list,IndexedLightInsertBefore,NULL,before); bool done=false; UInt32 result=0;
    __try { result=((IndexedLightListInsertFn)IndexedLightNativeInsertBefore)(list,before,payload); done=true; }
    __finally { IndexedLightEndListEdit(&e,done,done?(PerfRefNode*)result:NULL); }
    return result;
}
static UInt8** __fastcall IndexedLightRemovePositionPatch(PerfRefList* list,void*,UInt8** output,PerfRefNode** position)
{
    IndexedLightListEdit e=IndexedLightBeginListEdit(list,IndexedLightRemove,*position); bool done=false; UInt8** result=NULL;
    __try { result=(UInt8**)((PerfRemovePositionFn)IndexedLightNativeRemovePosition)(list,output,position); done=true; }
    __finally { IndexedLightEndListEdit(&e,done); }
    return result;
}
static UInt8** __fastcall IndexedLightRemoveHeadPatch(PerfRefList* list,void*,UInt8** output)
{
    IndexedLightListEdit e=IndexedLightBeginListEdit(list,IndexedLightRemove,list->head); bool done=false; UInt8** result=NULL;
    __try { result=((IndexedLightListRemoveFn)IndexedLightNativeRemoveHead)(list,output); done=true; }
    __finally { IndexedLightEndListEdit(&e,done); }
    return result;
}
static UInt8** __fastcall IndexedLightRemoveTailPatch(PerfRefList* list,void*,UInt8** output)
{
    IndexedLightListEdit e=IndexedLightBeginListEdit(list,IndexedLightRemove,list->tail); bool done=false; UInt8** result=NULL;
    __try { result=((IndexedLightListRemoveFn)IndexedLightNativeRemoveTail)(list,output); done=true; }
    __finally { IndexedLightEndListEdit(&e,done); }
    return result;
}
static void __fastcall IndexedLightClearListPatch(PerfRefList* list,void*)
{
    IndexedLightListEdit e=IndexedLightBeginListEdit(list,IndexedLightClear); bool done=false;
    __try { ((void (__thiscall*)(PerfRefList*))IndexedLightNativeClearList)(list); done=true; }
    __finally { IndexedLightEndListEdit(&e,done); }
}
static UInt32 __fastcall IndexedLightMoveBeforePatch(PerfRefList* list,void*,PerfRefNode* node,PerfRefNode* before)
{
    IndexedLightListEdit e=IndexedLightBeginListEdit(list,IndexedLightMove,node,before); bool done=false; UInt32 result=0;
    __try { result=((UInt32 (__thiscall*)(PerfRefList*,PerfRefNode*,PerfRefNode*))IndexedLightNativeMoveBefore)(list,node,before); done=true; }
    __finally { IndexedLightEndListEdit(&e,done); }
    return result;
}
static UInt32 __fastcall IndexedLightSetSourcePatch(UInt8* wrapper,void*,void* source)
{
    InterlockedIncrement(&s_indexedLightWriters); IndexedLightInvalidateSource(wrapper); UInt32 result=0;
    __try { result=((UInt32 (__thiscall*)(UInt8*,void*))IndexedLightNativeSetSource)(wrapper,source); }
    __finally { InterlockedDecrement(&s_indexedLightWriters); }
    return result;
}
static UInt8* __fastcall IndexedLightFindSourcePatch(UInt8* owner,void*,void* source)
{
    PerfRefList* list=(PerfRefList*)(owner+0xE4); PerfRefNode* node=NULL;
    if(IndexedLightLookupLight(list,(UInt32)source,true,&node))
    { PerfCount(&s_indexedLightCounters.sourceHits); return node?node->light:NULL; }
    PerfCount(&s_indexedLightCounters.sourceFallbacks);
    return ((UInt8* (__thiscall*)(UInt8*,void*))IndexedLightNativeFindSource)(owner,source);
}
static UInt8** __fastcall IndexedLightRemoveValuePatch(PerfRefList* list,void*,UInt8** output,UInt8** payload)
{
    PerfRefNode* position=NULL;
    // Native head removal is already constant time; do not build an index for it.
    if(!list->head || list->head->light==*payload ||
        !IndexedLightLookupLight(list,(UInt32)*payload,false,&position))
    {
        PerfCount(&s_indexedLightCounters.removeFallbacks);
        return ((UInt8** (__thiscall*)(PerfRefList*,UInt8**,UInt8**))IndexedLightNativeRemoveValue)(list,output,payload);
    }
    PerfCount(&s_indexedLightCounters.removeHits);
    UInt8* removed=NULL; bool completedRemoval=false;
    __try
    {
        UInt8* selected=*payload;
        if(position)
        {
            ((PerfRemovePositionFn)kIndexedLightRemovePositionPatchSite)(list,&removed,&position);
            completedRemoval=true;
            selected=removed;
        }
        *output=selected;
        if(selected) PerfAddReference(selected);
    }
    __finally { if(completedRemoval && removed) PerfReleaseReference(removed); }
    return output;
}
static void IndexedLightForgetReceiver(UInt8* owner)
{
    for(UInt32 i=0;i<4;++i)
    {
        IndexedLightReceiverScope* s=s_indexedReceiverScopes+i;
        if(s->owner!=owner) continue;
        s->valid=false;
        if(s->entries) HeapFree(GetProcessHeap(),0,s->entries);
        *s={};
    }
}
static void IndexedLightStartReceiver(UInt8* owner)
{
    if(!IndexedLightLightThreadAllowed() || s_indexedLightWriters) return;
    PerfRefList* list=(PerfRefList*)(owner+0xE4);
    PerfRefNode* fence=*(PerfRefNode**)(owner+0x144);
    if(!IndexedLightKnownRefList(list) || !fence || fence!=list->head || list->count<32 || list->count>32768) return;
    IndexedLightReceiverScope* s=NULL;
    for(UInt32 i=0;i<4;++i) if(!s_indexedReceiverScopes[i].owner) { s=s_indexedReceiverScopes+i; break; }
    if(!s) return;
    UInt32 cap=128; while(cap<list->count*2) cap*=2;
    s->entries=(IndexedLightLightEntry*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,cap*sizeof(IndexedLightLightEntry));
    if(!s->entries) return;
    s->owner=owner; s->list=list; s->fence=fence; s->capacity=cap; s->epoch=PerfLightListsEpoch();
    PerfRefNode* node=fence; PerfRefNode* previous=NULL;
    for(UInt32 i=0;i<list->count;++i)
    {
        if(!node || node->prev!=previous || !node->light ||
            !IndexedLightInsertEntry(s->entries,cap,(UInt32)node->light,node)) { IndexedLightForgetReceiver(owner); return; }
        previous=node; node=node->next;
    }
    if(node || previous!=list->tail) { IndexedLightForgetReceiver(owner); return; }
    s->valid=true;
}
static UInt32 __fastcall IndexedLightBeginReceiversPatch(UInt8* owner,void*)
{
    // Same-owner reentry invalidates its old pending-tail view before callbacks.
    IndexedLightForgetReceiver(owner);
    UInt32 result=((UInt32 (__thiscall*)(UInt8*))IndexedLightNativeBeginReceivers)(owner);
    IndexedLightStartReceiver(owner); return result;
}
static void __fastcall IndexedLightEndReceiversPatch(UInt8* owner,void*)
{
    IndexedLightForgetReceiver(owner);
    ((void (__thiscall*)(UInt8*))IndexedLightNativeEndReceivers)(owner);
}
static PerfRefNode* __stdcall IndexedLightFindOldReceiver(UInt8* owner,UInt8* geometry)
{
    if(IndexedLightLightThreadAllowed() && !s_indexedLightWriters)
    {
        for(UInt32 i=0;i<4;++i)
        {
            IndexedLightReceiverScope* s=s_indexedReceiverScopes+i;
            if(s->owner!=owner) continue;
            if(s->valid && s->epoch==PerfLightListsEpoch() &&
                s->fence==*(PerfRefNode**)(owner+0x144))
            {
                IndexedLightLightEntry* e=IndexedLightFindEntry(s->entries,s->capacity,(UInt32)geometry);
                PerfCount(e?&s_indexedLightCounters.receiverHits:&s_indexedLightCounters.receiverMisses);
                return e?e->node:NULL;
            }
            s->valid=false; break;
        }
    }
    PerfCount(&s_indexedLightCounters.receiverFallbacks);
    return (PerfRefNode*)0xFFFFFFFFu;
}
static __declspec(naked) void IndexedLightReceiverSearchPatch()
{
    __asm {
        mov ebx,[ebp+144h]
        pushfd
        pushad
        push esi
        push ebp
        call IndexedLightFindOldReceiver
        cmp eax,0FFFFFFFFh
        je native
        test eax,eax
        jz missing
        mov [esp+20],eax
        popad
        popfd
        push kIndexedLightReceiverHitContinue
        ret
    missing:
        popad
        popfd
        push kIndexedLightReceiverMissContinue
        ret
    native:
        popad
        popfd
        push kIndexedLightReceiverSearchContinue
        ret
    }
}
