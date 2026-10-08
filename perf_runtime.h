// Runtime wiring only. Every patch is staged in the existing single transaction.
static DWORD __cdecl PerfEngineMainThread()
{
    UInt8* main=*(UInt8**)kPerfActorMainAddr;
    return main ? *(DWORD*)(main+0x10) : 0;
}
static void __cdecl PerfSaveMapMutationNotification(RawSaveBlobMap*)
{ PerfExteriorInvalidate(); }
static void InitializePerformanceRuntime()
{
    InterlockedCompareExchange((volatile LONG*)&s_indexedLightThread,
        (LONG)GetCurrentThreadId(),0);
    s_actorMapCollectCounters=s_perfCollectCounters;
    s_indexedLightThreadProvider=PerfEngineMainThread;
    s_perfActorMapPressureObserver=PerfLightListsInvalidateExternal;
    s_perfLightListsMutationObserver=PerfPropertyInvalidate;
    s_perfExteriorCollectCounters=s_perfCollectCounters;
    PerfExteriorSetThreadProvider(PerfEngineMainThread);
    s_perfSaveMapMutationObserver=PerfSaveMapMutationNotification;
    s_perfSnapshotReadStream=SaveReadIsSnapshot;
    s_perfSnapshotExtent=SaveReadSnapshotExtent;
}
static void ReleasePerformanceThreadResources()
{
    // TLS storage only. Production releases use the OS process heap; never
    // acquire the global light-cache lock or call an engine object destructor
    // while processing the loader's thread-detach notification.
    PerfReleaseThreadSaveMapIndices();
    PerfExteriorReleaseThreadCaches();
    for(UInt32 i=0;i<4;++i)
    {
        IndexedLightLightEntry* entries=s_indexedReceiverScopes[i].entries;
        s_indexedReceiverScopes[i]={};
        if(entries) HeapFree(GetProcessHeap(),0,entries);
    }
}
static void LogExtendedPerformanceCounters()
{
    if(!s_perfCollectCounters) return;
    Log("SR1 save snapshots accepted=%I64d nativeFallback=%I64d rejected=%I64d unexpectedContract=%I64d",
        s_saveReadCounters.accepted,s_saveReadCounters.unsupported,
        s_saveReadCounters.rejected,s_saveReadCounters.unexpected);
    Log("PERF18 exterior calls=%I64d builds=%I64d visits=%I64d negative=%I64d reuses=%I64d fallback=%I64d; PERF20 queue calls=%I64d reserves=%I64d copiedSlots=%I64d fallback=%I64d",
        s_perfExteriorCounters.calls,s_perfExteriorCounters.builds,s_perfExteriorCounters.buildVisits,
        s_perfExteriorCounters.negativeHits,s_perfExteriorCounters.reuses,s_perfExteriorCounters.fallbacks,
        s_perfSaveQueueCounters.calls,s_perfSaveQueueCounters.reserves,
        s_perfSaveQueueCounters.copiedSlots,s_perfSaveQueueCounters.rejected);
    Log("PERF1/2 archive calls=%I64d names=%I64d bytes=%I64d rejected=%I64d; model calls=%I64d indexed=%I64d comparisons=%I64d rejected=%I64d",
        s_perfAMCounters.archiveCalls,s_perfAMCounters.archiveNames,s_perfAMCounters.archiveBytes,s_perfAMCounters.archiveRejected,
        s_perfAMCounters.modelCalls,s_perfAMCounters.modelIndexed,s_perfAMCounters.modelComparisons,s_perfAMCounters.modelRejected);
    Log("PERF9/12/13 source hits=%I64d fallback=%I64d remove hits=%I64d fallback=%I64d receiver hits=%I64d misses=%I64d fallback=%I64d builds=%I64d",
        s_indexedLightCounters.sourceHits,s_indexedLightCounters.sourceFallbacks,s_indexedLightCounters.removeHits,s_indexedLightCounters.removeFallbacks,
        s_indexedLightCounters.receiverHits,s_indexedLightCounters.receiverMisses,s_indexedLightCounters.receiverFallbacks,s_indexedLightCounters.builds);
    Log("PERF10/11 actor calls=%I64d indexed=%I64d fallback=%I64d map calls=%I64d indexed=%I64d visits=%I64d fallback=%I64d",
        s_actorMapCounters.actorCalls,s_actorMapCounters.actorIndexed,s_actorMapCounters.actorFallbacks,
        s_actorMapCounters.mapCalls,s_actorMapCounters.mapIndexed,s_actorMapCounters.mapBuildVisits,s_actorMapCounters.mapFallbacks);
    Log("PERF14 calls=%I64d scoped=%I64d nodes=%I64d predecessor scores avoided=%I64d disabled=%I64d",
        s_perfPropertyOrderCounters.calls,s_perfPropertyOrderCounters.scoped,s_perfPropertyOrderCounters.nodes,
        s_perfPropertyOrderCounters.predecessorScoresAvoided,s_perfPropertyOrderCounters.disabled);
    Log("PERF15/16/17 ordered checks=%I64d skipped=%I64d nodes=%I64d dense16=%I64d dense32=%I64d probes avoided=%I64d reserves=%I64d slots=%I64d",
        s_perfArraySortCounters.orderedChecks,s_perfArraySortCounters.orderedSkips,s_perfArraySortCounters.orderedNodes,
        s_perfArraySortCounters.dense16Skips,s_perfArraySortCounters.dense32Skips,s_perfArraySortCounters.holeProbesAvoided,
        s_perfArraySortCounters.transferReserves,s_perfArraySortCounters.transferSlots);
    Log("LC1 calls=%I64d bounded=%I64d omitted=%I64d failures=%I64d",
        s_perfLC1Counters.calls,s_perfLC1Counters.bounded,s_perfLC1Counters.omitted,s_perfLC1Counters.failures);
}
