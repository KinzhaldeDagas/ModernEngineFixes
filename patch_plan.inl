	// Retained for the offline source/engine verifier; never called at startup.
	static bool ValidateOfflineDecodeEvidence()
	{
		return ValidateDecodeBytes("native global frame offline evidence", kSaveReadGlobalFrameAddr,
				kSaveReadGlobalFrameAddrExpectedBytes, sizeof(kSaveReadGlobalFrameAddrExpectedBytes));
	}

	static bool ValidateDecode(ActorAnimClockPatchState actorAnimClockPatchState,
		TESWorldSpaceOFSTPatchState worldspaceOFSTPatchState)
	{
		// Non-short-circuit conjunction reports all independent contract mismatches.

		return

			(!PatchGroupSelected(kGroupSaveReadSnapshot) || ValidateDecodeBytes("kSaveReadGatePatchSite", kSaveReadGatePatchSite, kSaveReadGatePatchSiteExpectedBytes, sizeof(kSaveReadGatePatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSaveReadSnapshot) || ValidateDecodeBytes("kSaveReadNativePluginsAddr", kSaveReadNativePluginsAddr, kSaveReadNativePluginsAddrExpectedBytes, sizeof(kSaveReadNativePluginsAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSaveReadSnapshot) || ValidateDecodeBytes("kSaveReadBSFileTableAddr", kSaveReadBSFileTableAddr, kSaveReadBSFileTableAddrExpectedBytes, sizeof(kSaveReadBSFileTableAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSaveReadSnapshot) || ValidateDecodeBytes("kSaveReadNativeReadAddr", kSaveReadNativeReadAddr, kSaveReadNativeReadAddrExpectedBytes, sizeof(kSaveReadNativeReadAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSaveReadSnapshot) || ValidateDecodeBytes("kSaveReadNativeSeekAddr", kSaveReadNativeSeekAddr, kSaveReadNativeSeekAddrExpectedBytes, sizeof(kSaveReadNativeSeekAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSaveReadSnapshot) || ValidateDecodeBytes("kSaveReadNativeDtorAddr", kSaveReadNativeDtorAddr, kSaveReadNativeDtorAddrExpectedBytes, sizeof(kSaveReadNativeDtorAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSaveReadSnapshot) || ValidateDecodeBytes("kSaveReadCachedSizeAddr", kSaveReadCachedSizeAddr, kSaveReadCachedSizeAddrExpectedBytes, sizeof(kSaveReadCachedSizeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSaveReadSnapshot) || ValidateDecodeBytes("kSaveReadDirectSizeAddr", kSaveReadDirectSizeAddr, kSaveReadDirectSizeAddrExpectedBytes, sizeof(kSaveReadDirectSizeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSaveReadSnapshot) || ValidateDecodeBytes("kSaveReadInitialFrameAddr", kSaveReadInitialFrameAddr, kSaveReadInitialFrameAddrExpectedBytes, sizeof(kSaveReadInitialFrameAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSaveReadSnapshot) || ValidateDecodeBytes("kSaveReadRemainingFrameAddr", kSaveReadRemainingFrameAddr, kSaveReadRemainingFrameAddrExpectedBytes, sizeof(kSaveReadRemainingFrameAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSaveReadSnapshot) || ValidateDecodeBytes("kSaveReadGlobalValuesAddr", kSaveReadGlobalValuesAddr, kSaveReadGlobalValuesAddrExpectedBytes, sizeof(kSaveReadGlobalValuesAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSaveReadSnapshot) || ValidateDecodeBytes("kSaveReadTempEffectsAddr", kSaveReadTempEffectsAddr, kSaveReadTempEffectsAddrExpectedBytes, sizeof(kSaveReadTempEffectsAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSaveReadSnapshot) || ValidateDecodeBytes("kSaveReadArraysBodyAddr", kSaveReadArraysBodyAddr, kSaveReadArraysBodyAddrExpectedBytes, sizeof(kSaveReadArraysBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSaveReadSnapshot) || ValidateDecodeBytes("kSaveReadCreatedPrefixAddr", kSaveReadCreatedPrefixAddr, kSaveReadCreatedPrefixAddrExpectedBytes, sizeof(kSaveReadCreatedPrefixAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSaveReadSnapshot) || ValidateDecodeBytes("kSaveReadRecordFlowAddr", kSaveReadRecordFlowAddr, kSaveReadRecordFlowAddrExpectedBytes, sizeof(kSaveReadRecordFlowAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSaveReadSnapshot) || ValidateDecodeBytes("kSaveReadModeAddr", kSaveReadModeAddr, kSaveReadModeAddrExpectedBytes, sizeof(kSaveReadModeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfSaveQueuePatchSite", kPerfSaveQueuePatchSite, kPerfSaveQueuePatchSiteExpectedBytes, sizeof(kPerfSaveQueuePatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfSaveQueueAppendBodyAddr", kPerfSaveQueueAppendBodyAddr, kPerfSaveQueueAppendBodyAddrExpectedBytes, sizeof(kPerfSaveQueueAppendBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfSaveQueueVtableAddr", kPerfSaveQueueVtableAddr, kPerfSaveQueueVtableAddrExpectedBytes, sizeof(kPerfSaveQueueVtableAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfExteriorQueryPatchSite", kPerfExteriorQueryPatchSite, kPerfExteriorQueryPatchSiteExpectedBytes, sizeof(kPerfExteriorQueryPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfExteriorConsumerPatchSite", kPerfExteriorConsumerPatchSite, kPerfExteriorConsumerPatchSiteExpectedBytes, sizeof(kPerfExteriorConsumerPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfExteriorAddPatchSite", kPerfExteriorAddPatchSite, kPerfExteriorAddPatchSiteExpectedBytes, sizeof(kPerfExteriorAddPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfExteriorCtorPatchSite", kPerfExteriorCtorPatchSite, kPerfExteriorCtorPatchSiteExpectedBytes, sizeof(kPerfExteriorCtorPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfExteriorDtorPatchSite", kPerfExteriorDtorPatchSite, kPerfExteriorDtorPatchSiteExpectedBytes, sizeof(kPerfExteriorDtorPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfExteriorConsumerBodyAddr", kPerfExteriorConsumerBodyAddr, kPerfExteriorConsumerBodyAddrExpectedBytes, sizeof(kPerfExteriorConsumerBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfExteriorAddBodyAddr", kPerfExteriorAddBodyAddr, kPerfExteriorAddBodyAddrExpectedBytes, sizeof(kPerfExteriorAddBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfExteriorCtorBodyAddr", kPerfExteriorCtorBodyAddr, kPerfExteriorCtorBodyAddrExpectedBytes, sizeof(kPerfExteriorCtorBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfExteriorDtorBodyAddr", kPerfExteriorDtorBodyAddr, kPerfExteriorDtorBodyAddrExpectedBytes, sizeof(kPerfExteriorDtorBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfExteriorMapVtableAddr", kPerfExteriorMapVtableAddr, kPerfExteriorMapVtableAddrExpectedBytes, sizeof(kPerfExteriorMapVtableAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupThreadHandle) || ValidateDecodeBytes("OSGlobals main thread handle duplicate",
				kMainThreadHandlePatchSite,
				kMainThreadHandleExpectedBytes,
				sizeof(kMainThreadHandleExpectedBytes))) &
			(!PatchGroupSelected(kGroupThreadHandle) || ValidateDecodeBytes("BinkOpen main thread suspend use",
				kBinkOpenThreadHandleUseDecodeAddr,
				kBinkOpenThreadHandleUseExpectedBytes,
				sizeof(kBinkOpenThreadHandleUseExpectedBytes))) &
			(!PatchGroupSelected(kGroupThreadHandle) || ValidateDecodeBytes("BinkOpen main thread resume use",
				kBinkOpenResumeThreadUseDecodeAddr,
				kBinkOpenResumeThreadUseExpectedBytes,
				sizeof(kBinkOpenResumeThreadUseExpectedBytes))) &
			(!PatchGroupSelected(kGroupListAllocation) || ValidateDecodeBytes("BSSimpleList InsertSorted allocation-failure branch",
				kBSSimpleListInsertSortedAllocFailureBranchSite,
				kBSSimpleListInsertSortedAllocFailureExpectedBytes,
				sizeof(kBSSimpleListInsertSortedAllocFailureExpectedBytes))) &
			(!PatchGroupSelected(kGroupListAllocation) || ValidateDecodeBytes("BSSimpleList PushFront allocation-failure branch",
				kBSSimpleListPushFrontAllocFailureBranchSite,
				kBSSimpleListPushFrontAllocFailureExpectedBytes,
				sizeof(kBSSimpleListPushFrontAllocFailureExpectedBytes))) &
			(!PatchGroupSelected(kGroupEffectListAllocation) || ValidateDecodeBytes("EffectSetting filtered-list head allocation",
				kEffectSettingFilteredListHeadAllocPatchSite,
				kEffectSettingFilteredListHeadAllocExpectedBytes,
				sizeof(kEffectSettingFilteredListHeadAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupEffectListAllocation) || ValidateDecodeBytes("EffectSetting filtered-list node allocation-failure branch",
				kEffectSettingFilteredListNodeAllocFailureBranchSite,
				kEffectSettingFilteredListNodeAllocFailureExpectedBytes,
				sizeof(kEffectSettingFilteredListNodeAllocFailureExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad create-buffer tracker",
				kSaveLoadCreateBufferPatchSite,
				kSaveLoadCreateBufferExpectedBytes,
				sizeof(kSaveLoadCreateBufferExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad free-buffer tracker",
				kSaveLoadFreeBufferPatchSite,
				kSaveLoadFreeBufferExpectedBytes,
				sizeof(kSaveLoadFreeBufferExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad bounded active-buffer read",
				kSaveLoadBoundedLoadDataPatchSite,
				kSaveLoadBoundedLoadDataExpectedBytes,
				sizeof(kSaveLoadBoundedLoadDataExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad ActivateDeferredBuffer lifetime",
				kSaveLoadActivateDeferredBufferPatchSite,
				kSaveLoadActivateDeferredBufferExpectedBytes,
				sizeof(kSaveLoadActivateDeferredBufferExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad AnimationBufferBegin lifetime",
				kSaveLoadAnimationBufferBeginPatchSite,
				kSaveLoadAnimationBufferBeginExpectedBytes,
				sizeof(kSaveLoadAnimationBufferBeginExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad AnimationBufferEnd lifetime",
				kSaveLoadAnimationBufferEndPatchSite,
				kSaveLoadAnimationBufferEndExpectedBytes,
				sizeof(kSaveLoadAnimationBufferEndExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad AttachedAnimationBufferBegin lifetime",
				kSaveLoadAttachedAnimationBufferBeginPatchSite,
				kSaveLoadAttachedAnimationBufferBeginExpectedBytes,
				sizeof(kSaveLoadAttachedAnimationBufferBeginExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad AttachedAnimationBufferEnd lifetime",
				kSaveLoadAttachedAnimationBufferEndPatchSite,
				kSaveLoadAttachedAnimationBufferEndExpectedBytes,
				sizeof(kSaveLoadAttachedAnimationBufferEndExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad CharacterBufferBegin lifetime",
				kSaveLoadCharacterBufferBeginPatchSite,
				kSaveLoadCharacterBufferBeginExpectedBytes,
				sizeof(kSaveLoadCharacterBufferBeginExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad CharacterBufferEnd lifetime",
				kSaveLoadCharacterBufferEndPatchSite,
				kSaveLoadCharacterBufferEndExpectedBytes,
				sizeof(kSaveLoadCharacterBufferEndExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad HavokBufferBegin lifetime",
				kSaveLoadHavokBufferBeginPatchSite,
				kSaveLoadHavokBufferBeginExpectedBytes,
				sizeof(kSaveLoadHavokBufferBeginExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad HavokBufferEnd lifetime",
				kSaveLoadHavokBufferEndPatchSite,
				kSaveLoadHavokBufferEndExpectedBytes,
				sizeof(kSaveLoadHavokBufferEndExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad +0x54 raw blob helper",
				kSaveLoadRawBlobMap54PatchSite,
				kSaveLoadRawBlobMap54ExpectedBytes,
				sizeof(kSaveLoadRawBlobMap54ExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad +0x58 raw blob helper",
				kSaveLoadRawBlobMap58PatchSite,
				kSaveLoadRawBlobMap58ExpectedBytes,
				sizeof(kSaveLoadRawBlobMap58ExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad +0x5C raw blob helper",
				kSaveLoadRawBlobMap5CPatchSite,
				kSaveLoadRawBlobMap5CExpectedBytes,
				sizeof(kSaveLoadRawBlobMap5CExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad +0x60 raw blob helper",
				kSaveLoadRawBlobMap60PatchSite,
				kSaveLoadRawBlobMap60ExpectedBytes,
				sizeof(kSaveLoadRawBlobMap60ExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad expired-cell coordinate allocation",
				kSaveLoadExpiredCellEntryAllocPatchSite,
				kSaveLoadExpiredCellEntryAllocExpectedBytes,
				sizeof(kSaveLoadExpiredCellEntryAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("SaveLoad expired-cell list allocation",
				kSaveLoadExpiredCellListAllocPatchSite,
				kSaveLoadExpiredCellListAllocExpectedBytes,
				sizeof(kSaveLoadExpiredCellListAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ContainerExtraData list-node allocation",
				kContainerExtraDataListNodePatchSite,
				kContainerExtraDataListNodeExpectedBytes,
				sizeof(kContainerExtraDataListNodeExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ExtraData XCLR region-list buffer allocation",
				kExtraDataRegionListBufferAllocPatchSite,
				kExtraDataRegionListBufferAllocExpectedBytes,
				sizeof(kExtraDataRegionListBufferAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ExtraData XLOC lock-data allocation",
				kExtraDataLockDataAllocPatchSite,
				kExtraDataLockDataAllocExpectedBytes,
				sizeof(kExtraDataLockDataAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ExtraData XLOC wrapper allocation",
				kExtraDataLockWrapperAllocPatchSite,
				kExtraDataLockWrapperAllocExpectedBytes,
				sizeof(kExtraDataLockWrapperAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ExtraData XMRK map-marker allocation",
				kExtraDataMapMarkerAllocPatchSite,
				kExtraDataMapMarkerAllocExpectedBytes,
				sizeof(kExtraDataMapMarkerAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ExtraData XTEL teleport allocation",
				kExtraDataTeleportAllocPatchSite,
				kExtraDataTeleportAllocExpectedBytes,
				sizeof(kExtraDataTeleportAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ExtraData XRGD wrapper allocation",
				kExtraDataRagdollAllocPatchSite,
				kExtraDataRagdollAllocExpectedBytes,
				sizeof(kExtraDataRagdollAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ExtraData XRGD payload allocation",
				kExtraDataRagdollPayloadAllocPatchSite,
				kExtraDataRagdollPayloadAllocExpectedBytes,
				sizeof(kExtraDataRagdollPayloadAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ExtraData XCLR region-list object allocation",
				kExtraDataRegionListAllocPatchSite,
				kExtraDataRegionListAllocExpectedBytes,
				sizeof(kExtraDataRegionListAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESAnimation KFFZ chunk buffer allocation",
				kAnimationChunkBufferAllocPatchSite,
				kAnimationChunkBufferAllocExpectedBytes,
				sizeof(kAnimationChunkBufferAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESCreature NIFZ chunk buffer allocation",
				kTESCreatureNIFZBufferAllocPatchSite,
				kTESCreatureNIFZBufferAllocExpectedBytes,
				sizeof(kTESCreatureNIFZBufferAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ScriptEventList optional data allocation",
				kScriptEventListOptionalDataAllocPatchSite,
				kScriptEventListOptionalDataAllocExpectedBytes,
				sizeof(kScriptEventListOptionalDataAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("IndexedLight native main-thread identity", kIndexedLightMainThreadIdentityDecodeAddr,
				kIndexedLightMainThreadIdentityDecodeAddrExpectedBytes, sizeof(kIndexedLightMainThreadIdentityDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMArchiveEnumerateSite", kPerfAMArchiveEnumerateSite, kPerfAMArchiveEnumerateSiteExpectedBytes, sizeof(kPerfAMArchiveEnumerateSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMArchiveEmitSite", kPerfAMArchiveEmitSite, kPerfAMArchiveEmitSiteExpectedBytes, sizeof(kPerfAMArchiveEmitSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMAddModelSite", kPerfAMAddModelSite, kPerfAMAddModelSiteExpectedBytes, sizeof(kPerfAMAddModelSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMCopySite", kPerfAMCopySite, kPerfAMCopySiteExpectedBytes, sizeof(kPerfAMCopySiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMClearModelSite", kPerfAMClearModelSite, kPerfAMClearModelSiteExpectedBytes, sizeof(kPerfAMClearModelSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMNifzReadSite", kPerfAMNifzReadSite, kPerfAMNifzReadSiteExpectedBytes, sizeof(kPerfAMNifzReadSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMNifzBatchSite", kPerfAMNifzBatchSite, kPerfAMNifzBatchSiteExpectedBytes, sizeof(kPerfAMNifzBatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMArchiveEnumerateSiteBody", kPerfAMArchiveEnumerateSiteBody, kPerfAMArchiveEnumerateSiteBodyExpectedBytes, sizeof(kPerfAMArchiveEnumerateSiteBodyExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMArchiveGetterAddrBody", kPerfAMArchiveGetterAddrBody, kPerfAMArchiveGetterAddrBodyExpectedBytes, sizeof(kPerfAMArchiveGetterAddrBodyExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMAddModelSiteBody", kPerfAMAddModelSiteBody, kPerfAMAddModelSiteBodyExpectedBytes, sizeof(kPerfAMAddModelSiteBodyExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMCopySiteBody", kPerfAMCopySiteBody, kPerfAMCopySiteBodyExpectedBytes, sizeof(kPerfAMCopySiteBodyExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMClearModelSiteBody", kPerfAMClearModelSiteBody, kPerfAMClearModelSiteBodyExpectedBytes, sizeof(kPerfAMClearModelSiteBodyExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMNifzBodyAddrBody", kPerfAMNifzBodyAddrBody, kPerfAMNifzBodyAddrBodyExpectedBytes, sizeof(kPerfAMNifzBodyAddrBodyExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMContainsBodyAddrBody", kPerfAMContainsBodyAddrBody, kPerfAMContainsBodyAddrBodyExpectedBytes, sizeof(kPerfAMContainsBodyAddrBodyExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMClearTexturesAddrBody", kPerfAMClearTexturesAddrBody, kPerfAMClearTexturesAddrBodyExpectedBytes, sizeof(kPerfAMClearTexturesAddrBodyExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMSeekAddrBody", kPerfAMSeekAddrBody, kPerfAMSeekAddrBodyExpectedBytes, sizeof(kPerfAMSeekAddrBodyExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMArchiveVtableAddrBody", kPerfAMArchiveVtableAddrBody, kPerfAMArchiveVtableAddrBodyExpectedBytes, sizeof(kPerfAMArchiveVtableAddrBodyExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMChunkDataBodyAddrBody", kPerfAMChunkDataBodyAddrBody, kPerfAMChunkDataBodyAddrBodyExpectedBytes, sizeof(kPerfAMChunkDataBodyAddrBodyExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMEnterCSBodyAddrBody", kPerfAMEnterCSBodyAddrBody, kPerfAMEnterCSBodyAddrBodyExpectedBytes, sizeof(kPerfAMEnterCSBodyAddrBodyExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMLeaveCSBodyAddrBody", kPerfAMLeaveCSBodyAddrBody, kPerfAMLeaveCSBodyAddrBodyExpectedBytes, sizeof(kPerfAMLeaveCSBodyAddrBodyExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfAMPushFrontAddrBody", kPerfAMPushFrontAddrBody, kPerfAMPushFrontAddrBodyExpectedBytes, sizeof(kPerfAMPushFrontAddrBodyExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfLC1Site", kPerfLC1Site, kPerfLC1SiteExpectedBytes, sizeof(kPerfLC1SiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfLC1CountFillBodyAddr", kPerfLC1CountFillBodyAddr, kPerfLC1CountFillBodyAddrExpectedBytes, sizeof(kPerfLC1CountFillBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfLC1RequestKeyBodyAddr", kPerfLC1RequestKeyBodyAddr, kPerfLC1RequestKeyBodyAddrExpectedBytes, sizeof(kPerfLC1RequestKeyBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfLC1EpilogueBodyAddr", kPerfLC1EpilogueBodyAddr, kPerfLC1EpilogueBodyAddrExpectedBytes, sizeof(kPerfLC1EpilogueBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfLC1ContinuationBodyAddr", kPerfLC1ContinuationBodyAddr, kPerfLC1ContinuationBodyAddrExpectedBytes, sizeof(kPerfLC1ContinuationBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfLC1PublicationBodyAddr", kPerfLC1PublicationBodyAddr, kPerfLC1PublicationBodyAddrExpectedBytes, sizeof(kPerfLC1PublicationBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfLC1FirstAddr", kPerfLC1FirstAddr, kPerfLC1FirstAddrExpectedBytes, sizeof(kPerfLC1FirstAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfLC1NextAddr", kPerfLC1NextAddr, kPerfLC1NextAddrExpectedBytes, sizeof(kPerfLC1NextAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfLC1ConstructorAddr", kPerfLC1ConstructorAddr, kPerfLC1ConstructorAddrExpectedBytes, sizeof(kPerfLC1ConstructorAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfLC1FlagsBodyAddr", kPerfLC1FlagsBodyAddr, kPerfLC1FlagsBodyAddrExpectedBytes, sizeof(kPerfLC1FlagsBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfActorRebuildPatchSite", kPerfActorRebuildPatchSite, kPerfActorRebuildPatchSiteExpectedBytes, sizeof(kPerfActorRebuildPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfSaveMapSetAtPatchSite", kPerfSaveMapSetAtPatchSite, kPerfSaveMapSetAtPatchSiteExpectedBytes, sizeof(kPerfSaveMapSetAtPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfSaveMapRemovePatchSite", kPerfSaveMapRemovePatchSite, kPerfSaveMapRemovePatchSiteExpectedBytes, sizeof(kPerfSaveMapRemovePatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfSaveMapClearPatchSite", kPerfSaveMapClearPatchSite, kPerfSaveMapClearPatchSiteExpectedBytes, sizeof(kPerfSaveMapClearPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfSaveMapCtorPatchSite", kPerfSaveMapCtorPatchSite, kPerfSaveMapCtorPatchSiteExpectedBytes, sizeof(kPerfSaveMapCtorPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfSaveMapDtorPatchSite", kPerfSaveMapDtorPatchSite, kPerfSaveMapDtorPatchSiteExpectedBytes, sizeof(kPerfSaveMapDtorPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfActorMapPressurePatchSite", kPerfActorMapPressurePatchSite, kPerfActorMapPressurePatchSiteExpectedBytes, sizeof(kPerfActorMapPressurePatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfActorTruePredicateAddr", kPerfActorTruePredicateAddr, kPerfActorTruePredicateAddrExpectedBytes, sizeof(kPerfActorTruePredicateAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfActorZeroMovementAddr", kPerfActorZeroMovementAddr, kPerfActorZeroMovementAddrExpectedBytes, sizeof(kPerfActorZeroMovementAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfActorMovementAddr", kPerfActorMovementAddr, kPerfActorMovementAddrExpectedBytes, sizeof(kPerfActorMovementAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfActorPositionAddr", kPerfActorPositionAddr, kPerfActorPositionAddrExpectedBytes, sizeof(kPerfActorPositionAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfActorDistanceAddr", kPerfActorDistanceAddr, kPerfActorDistanceAddrExpectedBytes, sizeof(kPerfActorDistanceAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfActorPushFrontAddr", kPerfActorPushFrontAddr, kPerfActorPushFrontAddrExpectedBytes, sizeof(kPerfActorPushFrontAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfActorRebuildNativeAddr", kPerfActorRebuildNativeAddr, kPerfActorRebuildNativeAddrExpectedBytes, sizeof(kPerfActorRebuildNativeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfSaveMapVtableAddr", kPerfSaveMapVtableAddr, kPerfSaveMapVtableAddrExpectedBytes, sizeof(kPerfSaveMapVtableAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfSaveMapHashAddr", kPerfSaveMapHashAddr, kPerfSaveMapHashAddrExpectedBytes, sizeof(kPerfSaveMapHashAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfSaveMapEqualAddr", kPerfSaveMapEqualAddr, kPerfSaveMapEqualAddrExpectedBytes, sizeof(kPerfSaveMapEqualAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfSaveMapSetValueAddr", kPerfSaveMapSetValueAddr, kPerfSaveMapSetValueAddrExpectedBytes, sizeof(kPerfSaveMapSetValueAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfSaveMapClearValueAddr", kPerfSaveMapClearValueAddr, kPerfSaveMapClearValueAddrExpectedBytes, sizeof(kPerfSaveMapClearValueAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfSaveMapAllocateAddr", kPerfSaveMapAllocateAddr, kPerfSaveMapAllocateAddrExpectedBytes, sizeof(kPerfSaveMapAllocateAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfOrderedSortPatchSite", kPerfOrderedSortPatchSite, kPerfOrderedSortPatchSiteExpectedBytes, sizeof(kPerfOrderedSortPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfOrderedSortContinue", kPerfOrderedSortContinue, kPerfOrderedSortContinueExpectedBytes, sizeof(kPerfOrderedSortContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfOrderedSortDone", kPerfOrderedSortDone, kPerfOrderedSortDoneExpectedBytes, sizeof(kPerfOrderedSortDoneExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfOrderedSelectorCompareAddr", kPerfOrderedSelectorCompareAddr, kPerfOrderedSelectorCompareAddrExpectedBytes, sizeof(kPerfOrderedSelectorCompareAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfOrderedGeometryCompareAddr", kPerfOrderedGeometryCompareAddr, kPerfOrderedGeometryCompareAddrExpectedBytes, sizeof(kPerfOrderedGeometryCompareAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfDenseRefPatchSite", kPerfDenseRefPatchSite, kPerfDenseRefPatchSiteExpectedBytes, sizeof(kPerfDenseRefPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfDenseRefContinue", kPerfDenseRefContinue, kPerfDenseRefContinueExpectedBytes, sizeof(kPerfDenseRefContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfDenseRefAppend", kPerfDenseRefAppend, kPerfDenseRefAppendExpectedBytes, sizeof(kPerfDenseRefAppendExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfDenseRawPatchSite", kPerfDenseRawPatchSite, kPerfDenseRawPatchSiteExpectedBytes, sizeof(kPerfDenseRawPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfDenseRawContinue", kPerfDenseRawContinue, kPerfDenseRawContinueExpectedBytes, sizeof(kPerfDenseRawContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfDenseRawAppend", kPerfDenseRawAppend, kPerfDenseRawAppendExpectedBytes, sizeof(kPerfDenseRawAppendExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfFreshTransferPatchSite", kPerfFreshTransferPatchSite, kPerfFreshTransferPatchSiteExpectedBytes, sizeof(kPerfFreshTransferPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfFreshTransferContinue", kPerfFreshTransferContinue, kPerfFreshTransferContinueExpectedBytes, sizeof(kPerfFreshTransferContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfRefArrayVtable", kPerfRefArrayVtable, kPerfRefArrayVtableExpectedBytes, sizeof(kPerfRefArrayVtableExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfRawArrayVtable", kPerfRawArrayVtable, kPerfRawArrayVtableExpectedBytes, sizeof(kPerfRawArrayVtableExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfRefCtorDecodeAddr", kPerfRefCtorDecodeAddr, kPerfRefCtorDecodeAddrExpectedBytes, sizeof(kPerfRefCtorDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("kPerfRefSlotInitDecodeAddr", kPerfRefSlotInitDecodeAddr, kPerfRefSlotInitDecodeAddrExpectedBytes, sizeof(kPerfRefSlotInitDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfPropertyOrderCallSite", kPerfPropertyOrderCallSite, kPerfPropertyOrderCallSiteExpectedBytes, sizeof(kPerfPropertyOrderCallSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfPropertyPrefixPatchSite", kPerfPropertyPrefixPatchSite, kPerfPropertyPrefixPatchSiteExpectedBytes, sizeof(kPerfPropertyPrefixPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfPropertyPrefixContinue", kPerfPropertyPrefixContinue, kPerfPropertyPrefixContinueExpectedBytes, sizeof(kPerfPropertyPrefixContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfPropertyNextOuter", kPerfPropertyNextOuter, kPerfPropertyNextOuterExpectedBytes, sizeof(kPerfPropertyNextOuterExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfPropertyNativeOrderAddr", kPerfPropertyNativeOrderAddr, kPerfPropertyNativeOrderAddrExpectedBytes, sizeof(kPerfPropertyNativeOrderAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kPerfPropertyGetSourceAddr", kPerfPropertyGetSourceAddr, kPerfPropertyGetSourceAddrExpectedBytes, sizeof(kPerfPropertyGetSourceAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightAddTailPatchSite", kIndexedLightAddTailPatchSite, kIndexedLightAddTailPatchSiteExpectedBytes, sizeof(kIndexedLightAddTailPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightAddTailContinue", kIndexedLightAddTailContinue, kIndexedLightAddTailContinueExpectedBytes, sizeof(kIndexedLightAddTailContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightAddHeadPatchSite", kIndexedLightAddHeadPatchSite, kIndexedLightAddHeadPatchSiteExpectedBytes, sizeof(kIndexedLightAddHeadPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightAddHeadContinue", kIndexedLightAddHeadContinue, kIndexedLightAddHeadContinueExpectedBytes, sizeof(kIndexedLightAddHeadContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightInsertBeforePatchSite", kIndexedLightInsertBeforePatchSite, kIndexedLightInsertBeforePatchSiteExpectedBytes, sizeof(kIndexedLightInsertBeforePatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightInsertBeforeContinue", kIndexedLightInsertBeforeContinue, kIndexedLightInsertBeforeContinueExpectedBytes, sizeof(kIndexedLightInsertBeforeContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightRemovePositionPatchSite", kIndexedLightRemovePositionPatchSite, kIndexedLightRemovePositionPatchSiteExpectedBytes, sizeof(kIndexedLightRemovePositionPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightRemovePositionContinue", kIndexedLightRemovePositionContinue, kIndexedLightRemovePositionContinueExpectedBytes, sizeof(kIndexedLightRemovePositionContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightRemoveHeadPatchSite", kIndexedLightRemoveHeadPatchSite, kIndexedLightRemoveHeadPatchSiteExpectedBytes, sizeof(kIndexedLightRemoveHeadPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightRemoveHeadContinue", kIndexedLightRemoveHeadContinue, kIndexedLightRemoveHeadContinueExpectedBytes, sizeof(kIndexedLightRemoveHeadContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightRemoveTailPatchSite", kIndexedLightRemoveTailPatchSite, kIndexedLightRemoveTailPatchSiteExpectedBytes, sizeof(kIndexedLightRemoveTailPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightRemoveTailContinue", kIndexedLightRemoveTailContinue, kIndexedLightRemoveTailContinueExpectedBytes, sizeof(kIndexedLightRemoveTailContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightClearListPatchSite", kIndexedLightClearListPatchSite, kIndexedLightClearListPatchSiteExpectedBytes, sizeof(kIndexedLightClearListPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightClearListContinue", kIndexedLightClearListContinue, kIndexedLightClearListContinueExpectedBytes, sizeof(kIndexedLightClearListContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightMoveBeforePatchSite", kIndexedLightMoveBeforePatchSite, kIndexedLightMoveBeforePatchSiteExpectedBytes, sizeof(kIndexedLightMoveBeforePatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightMoveBeforeContinue", kIndexedLightMoveBeforeContinue, kIndexedLightMoveBeforeContinueExpectedBytes, sizeof(kIndexedLightMoveBeforeContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightSetSourcePatchSite", kIndexedLightSetSourcePatchSite, kIndexedLightSetSourcePatchSiteExpectedBytes, sizeof(kIndexedLightSetSourcePatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightSetSourceContinue", kIndexedLightSetSourceContinue, kIndexedLightSetSourceContinueExpectedBytes, sizeof(kIndexedLightSetSourceContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightFindSourcePatchSite", kIndexedLightFindSourcePatchSite, kIndexedLightFindSourcePatchSiteExpectedBytes, sizeof(kIndexedLightFindSourcePatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightFindSourceContinue", kIndexedLightFindSourceContinue, kIndexedLightFindSourceContinueExpectedBytes, sizeof(kIndexedLightFindSourceContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightRemoveValuePatchSite", kIndexedLightRemoveValuePatchSite, kIndexedLightRemoveValuePatchSiteExpectedBytes, sizeof(kIndexedLightRemoveValuePatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightRemoveValueContinue", kIndexedLightRemoveValueContinue, kIndexedLightRemoveValueContinueExpectedBytes, sizeof(kIndexedLightRemoveValueContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightBeginReceiversPatchSite", kIndexedLightBeginReceiversPatchSite, kIndexedLightBeginReceiversPatchSiteExpectedBytes, sizeof(kIndexedLightBeginReceiversPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightBeginReceiversContinue", kIndexedLightBeginReceiversContinue, kIndexedLightBeginReceiversContinueExpectedBytes, sizeof(kIndexedLightBeginReceiversContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightEndReceiversPatchSite", kIndexedLightEndReceiversPatchSite, kIndexedLightEndReceiversPatchSiteExpectedBytes, sizeof(kIndexedLightEndReceiversPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightEndReceiversContinue", kIndexedLightEndReceiversContinue, kIndexedLightEndReceiversContinueExpectedBytes, sizeof(kIndexedLightEndReceiversContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightReceiverSearchPatchSite", kIndexedLightReceiverSearchPatchSite, kIndexedLightReceiverSearchPatchSiteExpectedBytes, sizeof(kIndexedLightReceiverSearchPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightReceiverSearchContinue", kIndexedLightReceiverSearchContinue, kIndexedLightReceiverSearchContinueExpectedBytes, sizeof(kIndexedLightReceiverSearchContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightFullLightListVtable", kIndexedLightFullLightListVtable, kIndexedLightFullLightListVtableExpectedBytes, sizeof(kIndexedLightFullLightListVtableExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightReceiverListVtable", kIndexedLightReceiverListVtable, kIndexedLightReceiverListVtableExpectedBytes, sizeof(kIndexedLightReceiverListVtableExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightReceiverHitContinue", kIndexedLightReceiverHitContinue, kIndexedLightReceiverHitContinueExpectedBytes, sizeof(kIndexedLightReceiverHitContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightReceiverMissContinue", kIndexedLightReceiverMissContinue, kIndexedLightReceiverMissContinueExpectedBytes, sizeof(kIndexedLightReceiverMissContinueExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightAddTailBodyAddr", kIndexedLightAddTailBodyAddr, kIndexedLightAddTailBodyAddrExpectedBytes, sizeof(kIndexedLightAddTailBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightAddHeadBodyAddr", kIndexedLightAddHeadBodyAddr, kIndexedLightAddHeadBodyAddrExpectedBytes, sizeof(kIndexedLightAddHeadBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightInsertBeforeBodyAddr", kIndexedLightInsertBeforeBodyAddr, kIndexedLightInsertBeforeBodyAddrExpectedBytes, sizeof(kIndexedLightInsertBeforeBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightRemovePositionBodyAddr", kIndexedLightRemovePositionBodyAddr, kIndexedLightRemovePositionBodyAddrExpectedBytes, sizeof(kIndexedLightRemovePositionBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightRemoveHeadBodyAddr", kIndexedLightRemoveHeadBodyAddr, kIndexedLightRemoveHeadBodyAddrExpectedBytes, sizeof(kIndexedLightRemoveHeadBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightRemoveTailBodyAddr", kIndexedLightRemoveTailBodyAddr, kIndexedLightRemoveTailBodyAddrExpectedBytes, sizeof(kIndexedLightRemoveTailBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightClearListBodyAddr", kIndexedLightClearListBodyAddr, kIndexedLightClearListBodyAddrExpectedBytes, sizeof(kIndexedLightClearListBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightMoveBeforeBodyAddr", kIndexedLightMoveBeforeBodyAddr, kIndexedLightMoveBeforeBodyAddrExpectedBytes, sizeof(kIndexedLightMoveBeforeBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightSetSourceBodyAddr", kIndexedLightSetSourceBodyAddr, kIndexedLightSetSourceBodyAddrExpectedBytes, sizeof(kIndexedLightSetSourceBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightFindSourceBodyAddr", kIndexedLightFindSourceBodyAddr, kIndexedLightFindSourceBodyAddrExpectedBytes, sizeof(kIndexedLightFindSourceBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightRemoveValueBodyAddr", kIndexedLightRemoveValueBodyAddr, kIndexedLightRemoveValueBodyAddrExpectedBytes, sizeof(kIndexedLightRemoveValueBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightBeginReceiversBodyAddr", kIndexedLightBeginReceiversBodyAddr, kIndexedLightBeginReceiversBodyAddrExpectedBytes, sizeof(kIndexedLightBeginReceiversBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("kIndexedLightEndReceiversBodyAddr", kIndexedLightEndReceiversBodyAddr, kIndexedLightEndReceiversBodyAddrExpectedBytes, sizeof(kIndexedLightEndReceiversBodyAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfFilenoAddr", kPerfFilenoAddr, kPerfFilenoAddrExpectedBytes, sizeof(kPerfFilenoAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfOSHandleLayoutDecodeAddr", kPerfOSHandleLayoutDecodeAddr, kPerfOSHandleLayoutDecodeAddrExpectedBytes, sizeof(kPerfOSHandleLayoutDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfNativeListVtableDecodeAddr", kPerfNativeListVtableDecodeAddr, kPerfNativeListVtableDecodeAddrExpectedBytes, sizeof(kPerfNativeListVtableDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfAsciiCompareDecodeAddr", kPerfAsciiCompareDecodeAddr, kPerfAsciiCompareDecodeAddrExpectedBytes, sizeof(kPerfAsciiCompareDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfCreatedIDDecodeAddr", kPerfCreatedIDDecodeAddr, kPerfCreatedIDDecodeAddrExpectedBytes, sizeof(kPerfCreatedIDDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfListRemoveDecodeAddr", kPerfListRemoveDecodeAddr, kPerfListRemoveDecodeAddrExpectedBytes, sizeof(kPerfListRemoveDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("Master missing-file diagnostic", kPerfMasterErrorAddr,
				kPerfMasterErrorExpectedBytes, sizeof(kPerfMasterErrorExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfSortReusePatchSite",
				kPerfSortReusePatchSite, kPerfSortReusePatchSiteExpectedBytes, sizeof(kPerfSortReusePatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfSortDoneSite",
				kPerfSortDoneSite, kPerfSortDoneSiteExpectedBytes, sizeof(kPerfSortDoneSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfSortRebuildNodeSite",
				kPerfSortRebuildNodeSite, kPerfSortRebuildNodeSiteExpectedBytes, sizeof(kPerfSortRebuildNodeSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfSortRebuildHeadSite",
				kPerfSortRebuildHeadSite, kPerfSortRebuildHeadSiteExpectedBytes, sizeof(kPerfSortRebuildHeadSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfShadowTrimPatchSite",
				kPerfShadowTrimPatchSite, kPerfShadowTrimPatchSiteExpectedBytes, sizeof(kPerfShadowTrimPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfPeriodicTrimPatchSite",
				kPerfPeriodicTrimPatchSite, kPerfPeriodicTrimPatchSiteExpectedBytes, sizeof(kPerfPeriodicTrimPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfPeriodicTrimDoneSite",
				kPerfPeriodicTrimDoneSite, kPerfPeriodicTrimDoneSiteExpectedBytes, sizeof(kPerfPeriodicTrimDoneSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfNativeTrimAddr",
				kPerfNativeTrimAddr, kPerfNativeTrimAddrExpectedBytes, sizeof(kPerfNativeTrimAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("PerfLightRankPatchSite",
				kPerfLightRankPatchSite, kPerfLightRankPatchSiteExpectedBytes, sizeof(kPerfLightRankPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("PerfLightScoreAddr",
				kPerfLightScoreAddr, kPerfLightScoreAddrExpectedBytes, sizeof(kPerfLightScoreAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("PerfLightRemoveAddr",
				kPerfLightRemoveAddr, kPerfLightRemoveAddrExpectedBytes, sizeof(kPerfLightRemoveAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("PerfLightInsertAddr",
				kPerfLightInsertAddr, kPerfLightInsertAddrExpectedBytes, sizeof(kPerfLightInsertAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfFormIDPatchSite",
				kPerfFormIDPatchSite, kPerfFormIDPatchSiteExpectedBytes, sizeof(kPerfFormIDPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfWorldIDPatchSite",
				kPerfWorldIDPatchSite, kPerfWorldIDPatchSiteExpectedBytes, sizeof(kPerfWorldIDPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfOwnerCtorPatchSite",
				kPerfOwnerCtorPatchSite, kPerfOwnerCtorPatchSiteExpectedBytes, sizeof(kPerfOwnerCtorPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfOwnerDestroyPatchSite",
				kPerfOwnerDestroyPatchSite, kPerfOwnerDestroyPatchSiteExpectedBytes, sizeof(kPerfOwnerDestroyPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfOwnerResetPatchSite",
				kPerfOwnerResetPatchSite, kPerfOwnerResetPatchSiteExpectedBytes, sizeof(kPerfOwnerResetPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfIDLoadPatchSite",
				kPerfIDLoadPatchSite, kPerfIDLoadPatchSiteExpectedBytes, sizeof(kPerfIDLoadPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfArraySetPatchSite",
				kPerfArraySetPatchSite, kPerfArraySetPatchSiteExpectedBytes, sizeof(kPerfArraySetPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfArrayResizePatchSite",
				kPerfArrayResizePatchSite, kPerfArrayResizePatchSiteExpectedBytes, sizeof(kPerfArrayResizePatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfNumericPrefixPatchSite",
				kPerfNumericPrefixPatchSite, kPerfNumericPrefixPatchSiteExpectedBytes, sizeof(kPerfNumericPrefixPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfWorldPrefixPatchSite",
				kPerfWorldPrefixPatchSite, kPerfWorldPrefixPatchSiteExpectedBytes, sizeof(kPerfWorldPrefixPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfNumericReservePatchSite",
				kPerfNumericReservePatchSite, kPerfNumericReservePatchSiteExpectedBytes, sizeof(kPerfNumericReservePatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfWorldReservePatchSite",
				kPerfWorldReservePatchSite, kPerfWorldReservePatchSiteExpectedBytes, sizeof(kPerfWorldReservePatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfMasterPatchSite",
				kPerfMasterPatchSite, kPerfMasterPatchSiteExpectedBytes, sizeof(kPerfMasterPatchSiteExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfMasterCompareAddr",
				kPerfMasterCompareAddr, kPerfMasterCompareAddrExpectedBytes, sizeof(kPerfMasterCompareAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfStreamSizeAddr",
				kPerfStreamSizeAddr, kPerfStreamSizeAddrExpectedBytes, sizeof(kPerfStreamSizeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfIDLoadDecodeAddr",
				kPerfIDLoadDecodeAddr, kPerfIDLoadDecodeAddrExpectedBytes, sizeof(kPerfIDLoadDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfFormIDDecodeAddr",
				kPerfFormIDDecodeAddr, kPerfFormIDDecodeAddrExpectedBytes, sizeof(kPerfFormIDDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfWorldIDDecodeAddr",
				kPerfWorldIDDecodeAddr, kPerfWorldIDDecodeAddrExpectedBytes, sizeof(kPerfWorldIDDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfMasterDecodeAddr",
				kPerfMasterDecodeAddr, kPerfMasterDecodeAddrExpectedBytes, sizeof(kPerfMasterDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupRenderOptimizations) || ValidateDecodeBytes("PerfLightRankDecodeAddr",
				kPerfLightRankDecodeAddr, kPerfLightRankDecodeAddrExpectedBytes, sizeof(kPerfLightRankDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfSortDecodeAddr",
				kPerfSortDecodeAddr, kPerfSortDecodeAddrExpectedBytes, sizeof(kPerfSortDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfOwnerCtorDecodeAddr",
				kPerfOwnerCtorDecodeAddr, kPerfOwnerCtorDecodeAddrExpectedBytes, sizeof(kPerfOwnerCtorDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfOwnerResetDecodeAddr",
				kPerfOwnerResetDecodeAddr, kPerfOwnerResetDecodeAddrExpectedBytes, sizeof(kPerfOwnerResetDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfOwnerDestroyDecodeAddr",
				kPerfOwnerDestroyDecodeAddr, kPerfOwnerDestroyDecodeAddrExpectedBytes, sizeof(kPerfOwnerDestroyDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfArraySetDecodeAddr",
				kPerfArraySetDecodeAddr, kPerfArraySetDecodeAddrExpectedBytes, sizeof(kPerfArraySetDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfArrayResizeDecodeAddr",
				kPerfArrayResizeDecodeAddr, kPerfArrayResizeDecodeAddrExpectedBytes, sizeof(kPerfArrayResizeDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfBSFileVtableAddr",
				kPerfBSFileVtableAddr, kPerfBSFileVtableAddrExpectedBytes, sizeof(kPerfBSFileVtableAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("PerfStreamOpenDecodeAddr",
				kPerfStreamOpenDecodeAddr, kPerfStreamOpenDecodeAddrExpectedBytes, sizeof(kPerfStreamOpenDecodeAddrExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("Archive folder-name allocation",
				kArchiveFolderNameAllocPatchSite,
				kArchiveFolderNameAllocExpectedBytes,
				sizeof(kArchiveFolderNameAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ArchiveConstructorDecode",
				kArchiveConstructorDecodeAddr, kArchiveConstructorDecodeExpectedBytes, sizeof(kArchiveConstructorDecodeExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ArchiveFactoryDecode",
				kArchiveFactoryDecodeAddr, kArchiveFactoryDecodeExpectedBytes, sizeof(kArchiveFactoryDecodeExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ArchiveReadBytes",
				kArchiveReadBytesAddr, kArchiveReadBytesExpectedBytes, sizeof(kArchiveReadBytesExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ArchiveFilenameCleanupDecode",
				kArchiveFilenameCleanupDecodeAddr, kArchiveFilenameCleanupDecodeExpectedBytes, sizeof(kArchiveFilenameCleanupDecodeExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ArchiveDestructorDecode",
				kArchiveDestructorDecodeAddr, kArchiveDestructorDecodeExpectedBytes, sizeof(kArchiveDestructorDecodeExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ArchiveDeletingDestructorDecode",
				kArchiveDeletingDestructorDecodeAddr, kArchiveDeletingDestructorDecodeExpectedBytes, sizeof(kArchiveDeletingDestructorDecodeExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ArchiveReadCallbackDecode",
				kArchiveReadCallbackDecodeAddr, kArchiveReadCallbackDecodeExpectedBytes, sizeof(kArchiveReadCallbackDecodeExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ArchiveBSFileReadDecode",
				kArchiveBSFileReadDecodeAddr, kArchiveBSFileReadDecodeExpectedBytes, sizeof(kArchiveBSFileReadDecodeExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("ArchiveNiFileReadDecode",
				kArchiveNiFileReadDecodeAddr, kArchiveNiFileReadDecodeExpectedBytes, sizeof(kArchiveNiFileReadDecodeExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("Archive cached name deferred publication",
				kArchiveCachedNamesDeferPatchSite, kArchiveCachedNamesDeferExpectedBytes, sizeof(kArchiveCachedNamesDeferExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("Archive cached name private construction",
				kArchiveCachedNamesBuildPatchSite, kArchiveCachedNamesBuildExpectedBytes, sizeof(kArchiveCachedNamesBuildExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("Archive registration",
				kArchiveRegistrationPatchSite, kArchiveRegistrationExpectedBytes, sizeof(kArchiveRegistrationExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("BSFile swapped-write allocation",
				kBSFileWriteSwappedAllocPatchSite,
				kBSFileWriteSwappedAllocExpectedBytes,
				sizeof(kBSFileWriteSwappedAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("BSFile open buffer allocation",
				kBSFileOpenBufferAllocPatchSite,
				kBSFileOpenBufferAllocExpectedBytes,
				sizeof(kBSFileOpenBufferAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("model wildcard path allocation",
				kModelWildcardPathAllocPatchSite,
				kModelWildcardPathAllocExpectedBytes,
				sizeof(kModelWildcardPathAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("KF list head allocation",
				kKFListHeadAllocPatchSite,
				kKFListHeadAllocExpectedBytes,
				sizeof(kKFListHeadAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("KF list string allocation",
				kKFListStringAllocPatchSite,
				kKFListStringAllocExpectedBytes,
				sizeof(kKFListStringAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("KF list node allocation",
				kKFListNodeAllocPatchSite,
				kKFListNodeAllocExpectedBytes,
				sizeof(kKFListNodeAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESFile update-open-groups allocation",
				kTESFileUpdateGroupsAllocPatchSite,
				kTESFileUpdateGroupsAllocExpectedBytes,
				sizeof(kTESFileUpdateGroupsAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESFile loaded-master array allocation",
				kTESFileLoadedMastersAllocPatchSite,
				kTESFileLoadedMastersAllocExpectedBytes,
				sizeof(kTESFileLoadedMastersAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("archive master-list buffer allocation",
				kArchiveMasterListAllocPatchSite,
				kArchiveMasterListAllocExpectedBytes,
				sizeof(kArchiveMasterListAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("archive master-list bounded append",
				kArchiveMasterListAppendPatchSite,
				kArchiveMasterListAppendExpectedBytes,
				sizeof(kArchiveMasterListAppendExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("archive search-handle close",
				kArchiveSearchHandleClosePatchSite,
				kArchiveSearchHandleCloseExpectedBytes,
				sizeof(kArchiveSearchHandleCloseExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("archive Plugins.txt close",
				kArchivePluginsFileClosePatchSite,
				kArchivePluginsFileCloseExpectedBytes,
				sizeof(kArchivePluginsFileCloseExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("archive master-list newline trim",
				kArchiveMasterListNewlinePatchSite,
				kArchiveMasterListNewlineExpectedBytes,
				sizeof(kArchiveMasterListNewlineExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESSpellList saved count",
				kTESSpellListCountPatchSite,
				kTESSpellListCountExpectedBytes,
				sizeof(kTESSpellListCountExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESSpellList list-node allocation",
				kTESSpellListNodeAllocPatchSite,
				kTESSpellListNodeAllocExpectedBytes,
				sizeof(kTESSpellListNodeAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("AVCollection saved count",
				kAVCollectionCountPatchSite,
				kAVCollectionCountExpectedBytes,
				sizeof(kAVCollectionCountExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("MiddleHighProcess FormID saved count",
				kMiddleHighProcessFormIDCountPatchSite,
				kMiddleHighProcessFormIDCountExpectedBytes,
				sizeof(kMiddleHighProcessFormIDCountExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("MiddleHighProcess FormID list-node allocation",
				kMiddleHighProcessFormIDNodeAllocPatchSite,
				kMiddleHighProcessFormIDNodeAllocExpectedBytes,
				sizeof(kMiddleHighProcessFormIDNodeAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("KnownEffects saved count",
				kKnownEffectsCountPatchSite,
				kKnownEffectsCountExpectedBytes,
				sizeof(kKnownEffectsCountExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("Actor relationship/disposition first loop",
				kActorRelationshipDispositionFirstPatchSite,
				kActorRelationshipDispositionFirstLoopExpectedBytes,
				sizeof(kActorRelationshipDispositionFirstLoopExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("Actor relationship/disposition second loop",
				kActorRelationshipDispositionSecondPatchSite,
				kActorRelationshipDispositionSecondLoopExpectedBytes,
				sizeof(kActorRelationshipDispositionSecondLoopExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESForm current-save data wrapper",
				kTESFormLoadDataFromCurrentSaveGameAddr,
				kTESFormLoadDataFromCurrentSaveGameExpectedBytes,
				sizeof(kTESFormLoadDataFromCurrentSaveGameExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESForm current-save FormID wrapper",
				kTESFormLoadFormIDFromCurrentSaveGameAddr,
				kTESFormLoadFormIDFromCurrentSaveGameExpectedBytes,
				sizeof(kTESFormLoadFormIDFromCurrentSaveGameExpectedBytes))) &
			(!kInstallArchiveStreamingGuards ||
				((!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("archive raw read target guard",
					kArchiveRawReadTargetPatchSite,
					kArchiveRawReadTargetExpectedBytes,
					sizeof(kArchiveRawReadTargetExpectedBytes))) &&
				(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("archive raw read count clamp",
					kArchiveRawReadCountClampPatchSite,
					kArchiveRawReadCountClampExpectedBytes,
					sizeof(kArchiveRawReadCountClampExpectedBytes))) &&
				(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("compressed archive refill",
					kCompressedArchiveRefillPatchSite,
					kCompressedArchiveRefillExpectedBytes,
					sizeof(kCompressedArchiveRefillExpectedBytes))))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESFile_NextRecord advance",
				kTESFileNextRecordAdvancePatchSite,
				kTESFileNextRecordAdvanceExpectedBytes,
				sizeof(kTESFileNextRecordAdvanceExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESFile_GetNextChunk advance",
				kTESFileGetNextChunkAdvancePatchSite,
				kTESFileGetNextChunkAdvanceExpectedBytes,
				sizeof(kTESFileGetNextChunkAdvanceExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESFile compressed input bounds",
				kTESFileCompressedInputBoundsPatchSite,
				kTESFileCompressedInputBoundsExpectedBytes,
				sizeof(kTESFileCompressedInputBoundsExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESFile inflated output size",
				kTESFileInflatedSizePatchSite,
				kTESFileInflatedSizeExpectedBytes,
				sizeof(kTESFileInflatedSizeExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESFile inflate-init failure cleanup",
				kTESFileInflateInitCleanupPatchSite,
				kTESFileInflateInitCleanupExpectedBytes,
				sizeof(kTESFileInflateInitCleanupExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESFile common inflate failure cleanup",
				kTESFileInflateFailureCleanupDecodeAddr,
				kTESFileInflateFailureCleanupExpectedBytes,
				sizeof(kTESFileInflateFailureCleanupExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESFile compressed record size",
				kTESFileDecompressedRecordSizePatchSite,
				kTESFileDecompressedRecordSizeExpectedBytes,
				sizeof(kTESFileDecompressedRecordSizeExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESFile decompressed-record allocation",
				kTESFileDecompressedRecordAllocPatchSite,
				kTESFileDecompressedRecordAllocExpectedBytes,
				sizeof(kTESFileDecompressedRecordAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESFile compressed chunk header tail",
				kTESFileLoadChunkHeaderCompressedTailPatchSite,
				kTESFileLoadChunkHeaderCompressedTailExpectedBytes,
				sizeof(kTESFileLoadChunkHeaderCompressedTailExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESFile truncated compressed chunk copy",
				kTESFileGetChunkDataTruncatedCopyPatchSite,
				kTESFileGetChunkDataTruncatedCopyExpectedBytes,
				sizeof(kTESFileGetChunkDataTruncatedCopyExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESFile full compressed chunk copy",
				kTESFileGetChunkDataFullCopyPatchSite,
				kTESFileGetChunkDataFullCopyExpectedBytes,
				sizeof(kTESFileGetChunkDataFullCopyExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESFile JumpToRecord target",
				kTESFileJumpToRecordValidatePatchSite,
				kTESFileJumpToRecordValidateExpectedBytes,
				sizeof(kTESFileJumpToRecordValidateExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESFile NextGroup",
				kTESFileNextGroupPatchSite,
				kTESFileNextGroupExpectedBytes,
				sizeof(kTESFileNextGroupExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESForm record compression temp allocation",
				kTESFormCompressRecordTempAllocPatchSite,
				kTESFormCompressRecordTempAllocExpectedBytes,
				sizeof(kTESFormCompressRecordTempAllocExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESForm record compression deflate-error cleanup",
				kTESFormCompressRecordDeflateErrorCleanupPatchSite,
				kTESFormCompressRecordDeflateErrorCleanupExpectedBytes,
				sizeof(kTESFormCompressRecordDeflateErrorCleanupExpectedBytes))) &
			(!PatchGroupSelected(kGroupSavePerformanceRuntime) || ValidateDecodeBytes("TESForm record compression final allocation",
				kTESFormCompressRecordFinalAllocPatchSite,
				kTESFormCompressRecordFinalAllocExpectedBytes,
				sizeof(kTESFormCompressRecordFinalAllocExpectedBytes))) &
			(worldspaceOFSTPatchState != kTESWorldSpaceOFSTPatchNeedsInstall ||
				((!PatchGroupSelected(kGroupWorldspaceOffsets) || ValidateDecodeBytes("TESWorldSpace WRLD OFST load",
					kTESWorldSpaceLoadOFSTPatchSite,
					kTESWorldSpaceLoadOFSTExpectedBytes,
					sizeof(kTESWorldSpaceLoadOFSTExpectedBytes))) &&
				(!PatchGroupSelected(kGroupWorldspaceOffsets) || ValidateDecodeBytes("TESWorldSpace exterior cell offset lookup",
					kTESWorldSpaceFindCellOffsetPatchSite,
					kTESWorldSpaceFindCellOffsetExpectedBytes,
					sizeof(kTESWorldSpaceFindCellOffsetExpectedBytes))) &&
				(!PatchGroupSelected(kGroupWorldspaceOffsets) || ValidateDecodeBytes("TESWorldSpace rebuilt offset allocation",
					kTESWorldSpaceRebuildAllocPatchSite,
					kTESWorldSpaceRebuildAllocExpectedBytes,
					sizeof(kTESWorldSpaceRebuildAllocExpectedBytes))) &&
				(!PatchGroupSelected(kGroupWorldspaceOffsets) || ValidateDecodeBytes("TESWorldSpace post-fixup rebuilt offset write",
					kTESWorldSpacePostFixupOffsetWritePatchSite,
					kTESWorldSpacePostFixupOffsetWriteExpectedBytes,
					sizeof(kTESWorldSpacePostFixupOffsetWriteExpectedBytes))) &&
				(!PatchGroupSelected(kGroupWorldspaceOffsets) || ValidateDecodeBytes("TESWorldSpace post-fixup rebuilt offset commit",
					kTESWorldSpacePostFixupOffsetCommitPatchSite,
					kTESWorldSpacePostFixupOffsetCommitExpectedBytes,
					sizeof(kTESWorldSpacePostFixupOffsetCommitExpectedBytes))) &&
				(!PatchGroupSelected(kGroupWorldspaceOffsets) || ValidateDecodeBytes("TESWorldSpace post-fixup offset validation read",
					kTESWorldSpacePostFixupOffsetReadPatchSite,
					kTESWorldSpacePostFixupOffsetReadExpectedBytes,
					sizeof(kTESWorldSpacePostFixupOffsetReadExpectedBytes))))) &
			(!PatchGroupSelected(kGroupLoadingWait) || ValidateDecodeBytes("loading texture wait",
				kLoadingTextureWaitPatchSite,
				kLoadingTextureWaitExpectedBytes,
				sizeof(kLoadingTextureWaitExpectedBytes))) &
			(!PatchGroupSelected(kGroupLoadTick) || ValidateDecodeBytes("load-game start tick",
				kLoadGameStartTickPatchSite,
				kLoadGameStartTickExpectedBytes,
				sizeof(kLoadGameStartTickExpectedBytes))) &
			(!PatchGroupSelected(kGroupMessageTick) || ValidateDecodeBytes("worldspace message delay",
				kWorldspaceMessageDelayPatchSite,
				kWorldspaceMessageDelayExpectedBytes,
				sizeof(kWorldspaceMessageDelayExpectedBytes))) &
			(!PatchGroupSelected(kGroupActorAnimationClock) || ValidateDecodeBytes("ActorAnimData restore clock write",
				kActorAnimRestoreClockWriteSite,
				kActorAnimRestoreClockWriteExpectedBytes,
				sizeof(kActorAnimRestoreClockWriteExpectedBytes))) &
			(!PatchGroupSelected(kGroupActorAnimationClock) || ValidateDecodeBytes("ActorAnimData clock write",
				kActorAnimUpdateClockWriteSite,
				kActorAnimClockWriteExpectedBytes,
				sizeof(kActorAnimClockWriteExpectedBytes))) &
			(actorAnimClockPatchState != kActorAnimClockPatchNeedsInstall ||
				(!PatchGroupSelected(kGroupActorAnimationClock) || ValidateDecodeBytes("ActorAnimData post-clock loop",
					kActorAnimUpdateClockPatchSite,
					kActorAnimClockPatchExpectedBytes,
					sizeof(kActorAnimClockPatchExpectedBytes)))) &
			(!PatchGroupSelected(kGroupActorAnimationClock) || ValidateDecodeBytes("BSAnimGroupSequence sample update",
				kBSAnimGroupSequenceSampleUpdateAddr,
				kBSAnimGroupSequenceSampleUpdateExpectedBytes,
				sizeof(kBSAnimGroupSequenceSampleUpdateExpectedBytes))) &
			(!PatchGroupSelected(kGroupActorAnimationClock) || ValidateDecodeBytes("BSAnimGroupSequence save state",
				kBSAnimGroupSequenceSaveStateAddr,
				kBSAnimGroupSequenceSaveStateExpectedBytes,
				sizeof(kBSAnimGroupSequenceSaveStateExpectedBytes))) &
			(!PatchGroupSelected(kGroupActorAnimationClock) || ValidateDecodeBytes("BSAnimGroupSequence load state",
				kBSAnimGroupSequenceLoadStateAddr,
				kBSAnimGroupSequenceLoadStateExpectedBytes,
				sizeof(kBSAnimGroupSequenceLoadStateExpectedBytes))) &
			(!PatchGroupSelected(kGroupActorAnimationClock) || ValidateDecodeBytes("NiControllerSequence update",
				kNiControllerSequenceUpdateAddr,
				kNiControllerSequenceUpdateExpectedBytes,
				sizeof(kNiControllerSequenceUpdateExpectedBytes))) ;
	}

	static TexturePalettePatchState texturePaletteState = (TexturePalettePatchState)-1;

	static ActorAnimPreSamplePatchState actorAnimPreSampleState = (ActorAnimPreSamplePatchState)-1;
	static ActorAnimClockPatchState actorAnimClockPatchState = (ActorAnimClockPatchState)-1;
	static TESWorldSpaceOFSTPatchState worldspaceOFSTPatchState = (TESWorldSpaceOFSTPatchState)-1;
    static bool ValidateSelectedPatchGroup()
    {
        if (PatchGroupSelected(kGroupTexturePaletteGuard)) {
            texturePaletteState = GetTexturePalettePatchState();
            if (texturePaletteState == kTexturePalettePatchMismatch) return false;
        }
        if (PatchGroupSelected(kGroupActorAnimationClock)) {
            actorAnimPreSampleState = GetActorAnimPreSamplePatchState();
            actorAnimClockPatchState = GetActorAnimClockPatchState();
            if (actorAnimPreSampleState == kActorAnimPreSamplePatchMismatch ||
                actorAnimClockPatchState == kActorAnimClockPatchMismatch) return false;
        }
        if (PatchGroupSelected(kGroupWorldspaceOffsets)) {
            worldspaceOFSTPatchState = GetTESWorldSpaceOFSTPatchState();
            if (worldspaceOFSTPatchState == kTESWorldSpaceOFSTPatchMismatch) return false;
        }
        return ValidateDecode(actorAnimClockPatchState, worldspaceOFSTPatchState);
    }

    static bool StageSelectedPatchGroup()
    {

		if (!WriteRelCall(kSaveReadGatePatchSite, (UInt32)&SaveReadPreflightGate)) return false;

		if (!(WriteRelCall(kPerfSaveQueuePatchSite, (UInt32)&PerfSaveQueueAppendPatch) &&
			WriteRelJump(kPerfExteriorQueryPatchSite, (UInt32)&PerfExteriorQueryPatch, sizeof(kPerfExteriorQueryPatchSiteExpectedBytes)) &&
			WriteRelJump(kPerfExteriorConsumerPatchSite, (UInt32)&PerfExteriorConsumerPatch, sizeof(kPerfExteriorConsumerPatchSiteExpectedBytes)) &&
			WriteRelJump(kPerfExteriorAddPatchSite, (UInt32)&PerfExteriorAddPatch, sizeof(kPerfExteriorAddPatchSiteExpectedBytes)) &&
			WriteRelJump(kPerfExteriorCtorPatchSite, (UInt32)&PerfExteriorCtorPatch, sizeof(kPerfExteriorCtorPatchSiteExpectedBytes)) &&
			WriteRelJump(kPerfExteriorDtorPatchSite, (UInt32)&PerfExteriorDtorPatch, sizeof(kPerfExteriorDtorPatchSiteExpectedBytes)))) return false;

		if (!(WriteRelJump(kPerfAMArchiveEnumerateSite, (UInt32)&PerfAMArchiveEnumerate, sizeof(kPerfAMArchiveEnumerateSiteExpectedBytes)) &&
			WriteRelJump(kPerfAMArchiveEmitSite, (UInt32)&PerfAMArchiveEmitPatch, sizeof(kPerfAMArchiveEmitSiteExpectedBytes)) &&
			WriteRelJump(kPerfAMAddModelSite, (UInt32)&PerfAMAddModelPatch, sizeof(kPerfAMAddModelSiteExpectedBytes)) &&
			WriteRelJump(kPerfAMCopySite, (UInt32)&PerfAMCopyModelPatch, sizeof(kPerfAMCopySiteExpectedBytes)) &&
			WriteRelJump(kPerfAMClearModelSite, (UInt32)&PerfAMClearModelPatch, sizeof(kPerfAMClearModelSiteExpectedBytes)) &&
			WriteRelCall(kPerfAMNifzReadSite, (UInt32)&PerfAMReadNifzPatch) &&
			WriteRelJump(kPerfAMNifzBatchSite, (UInt32)&PerfAMNifzBatchPatch, sizeof(kPerfAMNifzBatchSiteExpectedBytes)) &&
			WriteRelJump(kPerfLC1Site, (UInt32)&PerfLC1BuildPatch, sizeof(kPerfLC1SiteExpectedBytes)) &&
			WriteRelCall(kPerfActorRebuildPatchSite, (UInt32)&PerfActorRebuildPatch) &&
			WriteRelJump(kPerfSaveMapSetAtPatchSite, (UInt32)&PerfSaveMapSetAtPatch, sizeof(kPerfSaveMapSetAtPatchSiteExpectedBytes)) &&
			WriteRelJump(kPerfSaveMapRemovePatchSite, (UInt32)&PerfSaveMapRemovePatch, sizeof(kPerfSaveMapRemovePatchSiteExpectedBytes)) &&
			WriteRelJump(kPerfSaveMapClearPatchSite, (UInt32)&PerfSaveMapClearPatch, sizeof(kPerfSaveMapClearPatchSiteExpectedBytes)) &&
			WriteRelJump(kPerfSaveMapCtorPatchSite, (UInt32)&PerfSaveMapCtorPatch, sizeof(kPerfSaveMapCtorPatchSiteExpectedBytes)) &&
			WriteRelJump(kPerfSaveMapDtorPatchSite, (UInt32)&PerfSaveMapDtorPatch, sizeof(kPerfSaveMapDtorPatchSiteExpectedBytes)) &&
			WriteRelJump(kPerfActorMapPressurePatchSite, (UInt32)&PerfActorMapPressurePatch, sizeof(kPerfActorMapPressurePatchSiteExpectedBytes)) &&
			WriteRelJump(kPerfOrderedSortPatchSite, (UInt32)&PerfOrderedPassSortPatch, sizeof(kPerfOrderedSortPatchSiteExpectedBytes)) &&
			WriteRelJump(kPerfDenseRefPatchSite, (UInt32)&PerfDenseRefArrayPatch, sizeof(kPerfDenseRefPatchSiteExpectedBytes)) &&
			WriteRelJump(kPerfDenseRawPatchSite, (UInt32)&PerfDenseRawArrayPatch, sizeof(kPerfDenseRawPatchSiteExpectedBytes)) &&
			WriteRelJump(kPerfFreshTransferPatchSite, (UInt32)&PerfFreshTransferReservePatch, sizeof(kPerfFreshTransferPatchSiteExpectedBytes)) &&
			WriteRelCall(kPerfPropertyOrderCallSite, (UInt32)&PerfPropertyOrderCallPatch) &&
			WriteRelJump(kPerfPropertyPrefixPatchSite, (UInt32)&PerfPropertyPrefixPatch, sizeof(kPerfPropertyPrefixPatchSiteExpectedBytes)) &&
			WriteRelJump(kIndexedLightAddTailPatchSite, (UInt32)&IndexedLightAddTailPatch, sizeof(kIndexedLightAddTailPatchSiteExpectedBytes)) &&
			WriteRelJump(kIndexedLightAddHeadPatchSite, (UInt32)&IndexedLightAddHeadPatch, sizeof(kIndexedLightAddHeadPatchSiteExpectedBytes)) &&
			WriteRelJump(kIndexedLightInsertBeforePatchSite, (UInt32)&IndexedLightInsertBeforePatch, sizeof(kIndexedLightInsertBeforePatchSiteExpectedBytes)) &&
			WriteRelJump(kIndexedLightRemovePositionPatchSite, (UInt32)&IndexedLightRemovePositionPatch, sizeof(kIndexedLightRemovePositionPatchSiteExpectedBytes)) &&
			WriteRelJump(kIndexedLightRemoveHeadPatchSite, (UInt32)&IndexedLightRemoveHeadPatch, sizeof(kIndexedLightRemoveHeadPatchSiteExpectedBytes)) &&
			WriteRelJump(kIndexedLightRemoveTailPatchSite, (UInt32)&IndexedLightRemoveTailPatch, sizeof(kIndexedLightRemoveTailPatchSiteExpectedBytes)) &&
			WriteRelJump(kIndexedLightClearListPatchSite, (UInt32)&IndexedLightClearListPatch, sizeof(kIndexedLightClearListPatchSiteExpectedBytes)) &&
			WriteRelJump(kIndexedLightMoveBeforePatchSite, (UInt32)&IndexedLightMoveBeforePatch, sizeof(kIndexedLightMoveBeforePatchSiteExpectedBytes)) &&
			WriteRelJump(kIndexedLightSetSourcePatchSite, (UInt32)&IndexedLightSetSourcePatch, sizeof(kIndexedLightSetSourcePatchSiteExpectedBytes)) &&
			WriteRelJump(kIndexedLightFindSourcePatchSite, (UInt32)&IndexedLightFindSourcePatch, sizeof(kIndexedLightFindSourcePatchSiteExpectedBytes)) &&
			WriteRelJump(kIndexedLightRemoveValuePatchSite, (UInt32)&IndexedLightRemoveValuePatch, sizeof(kIndexedLightRemoveValuePatchSiteExpectedBytes)) &&
			WriteRelJump(kIndexedLightBeginReceiversPatchSite, (UInt32)&IndexedLightBeginReceiversPatch, sizeof(kIndexedLightBeginReceiversPatchSiteExpectedBytes)) &&
			WriteRelJump(kIndexedLightEndReceiversPatchSite, (UInt32)&IndexedLightEndReceiversPatch, sizeof(kIndexedLightEndReceiversPatchSiteExpectedBytes)) &&
			WriteRelJump(kIndexedLightReceiverSearchPatchSite, (UInt32)&IndexedLightReceiverSearchPatch, sizeof(kIndexedLightReceiverSearchPatchSiteExpectedBytes))))
		{
			Log("failed to stage performance and light-count fixes");
			return false;
		}

		if (!WriteRelJump(kPerfSortReusePatchSite, (UInt32)&PerfSortReusePatch, sizeof(kPerfSortReusePatchSiteExpectedBytes)) ||
			!WriteRelCall(kPerfShadowTrimPatchSite, (UInt32)&PerfShadowTrimPatch) ||
			!WriteRelJump(kPerfPeriodicTrimPatchSite, (UInt32)&PerfPeriodicTrimPatch, sizeof(kPerfPeriodicTrimPatchSiteExpectedBytes)) ||
			!WriteRelJump(kPerfLightRankPatchSite, (UInt32)&PerfLightRankPatch, sizeof(kPerfLightRankPatchSiteExpectedBytes)) ||
			!WriteRelJump(kPerfFormIDPatchSite, (UInt32)&PerfFormIDPatch, sizeof(kPerfFormIDPatchSiteExpectedBytes)) ||
			!WriteRelJump(kPerfWorldIDPatchSite, (UInt32)&PerfWorldIDPatch, sizeof(kPerfWorldIDPatchSiteExpectedBytes)) ||
			!WriteRelJump(kPerfOwnerCtorPatchSite, (UInt32)&PerfOwnerCtorPatch, sizeof(kPerfOwnerCtorPatchSiteExpectedBytes)) ||
			!WriteRelJump(kPerfOwnerDestroyPatchSite, (UInt32)&PerfOwnerDestroyPatch, sizeof(kPerfOwnerDestroyPatchSiteExpectedBytes)) ||
			!WriteRelJump(kPerfOwnerResetPatchSite, (UInt32)&PerfOwnerResetPatch, sizeof(kPerfOwnerResetPatchSiteExpectedBytes)) ||
			!WriteRelJump(kPerfIDLoadPatchSite, (UInt32)&PerfIDLoadPatch, sizeof(kPerfIDLoadPatchSiteExpectedBytes)) ||
			!WriteRelJump(kPerfArraySetPatchSite, (UInt32)&PerfArraySetPatch, sizeof(kPerfArraySetPatchSiteExpectedBytes)) ||
			!WriteRelJump(kPerfArrayResizePatchSite, (UInt32)&PerfArrayResizePatch, sizeof(kPerfArrayResizePatchSiteExpectedBytes)) ||
			!WriteRelJump(kPerfNumericPrefixPatchSite, (UInt32)&PerfNumericPrefixPatch, sizeof(kPerfNumericPrefixPatchSiteExpectedBytes)) ||
			!WriteRelJump(kPerfWorldPrefixPatchSite, (UInt32)&PerfWorldPrefixPatch, sizeof(kPerfWorldPrefixPatchSiteExpectedBytes)) ||
			!WriteRelCall(kPerfNumericReservePatchSite, (UInt32)&PerfNumericReservePatch) ||
			!WriteRelCall(kPerfWorldReservePatchSite, (UInt32)&PerfWorldReservePatch) ||
			!WriteRelJump(kPerfMasterPatchSite, (UInt32)&PerfMasterPatch, sizeof(kPerfMasterPatchSiteExpectedBytes)))
		{
			Log("failed to stage performance fixes and ID-array lifetime guards");
			return false;
		}
		// Both constructor hooks and checked factory publication commit with the
		// entire installation. No half-installed cached-name path can execute.
		if (!WriteRelJump(kArchiveCachedNamesDeferPatchSite,
			kArchiveCachedNamesDeferContinueSite, sizeof(kArchiveCachedNamesDeferExpectedBytes)) ||
			!WriteRelJump(kArchiveCachedNamesBuildPatchSite,
				(UInt32)&ArchiveCachedNamesBuildPatch, sizeof(kArchiveCachedNamesBuildExpectedBytes)) ||
			!WriteRelJump(kArchiveRegistrationPatchSite,
				(UInt32)&ArchiveRegistrationPatch, sizeof(kArchiveRegistrationExpectedBytes)))
		{
			Log("failed to stage archive cached-name construction/registration guards");
			return false;
		}

		if (!WriteRelJump(kMainThreadHandlePatchSite, (UInt32)&MainThreadHandleDuplicatePatch, sizeof(kMainThreadHandleExpectedBytes)))
		{
			Log("failed to write OSGlobals main thread handle patch at %08X", kMainThreadHandlePatchSite);
			return false;
		}

		if (!WriteBytes(kBSSimpleListInsertSortedAllocFailureBranchSite,
				kBSSimpleListInsertSortedAllocFailureReplacementBytes,
				sizeof(kBSSimpleListInsertSortedAllocFailureReplacementBytes)))
		{
			Log("failed to write BSSimpleList InsertSorted allocation-failure branch at %08X",
				kBSSimpleListInsertSortedAllocFailureBranchSite);
			return false;
		}

		if (!WriteBytes(kBSSimpleListPushFrontAllocFailureBranchSite,
				kBSSimpleListPushFrontAllocFailureReplacementBytes,
				sizeof(kBSSimpleListPushFrontAllocFailureReplacementBytes)))
		{
			Log("failed to write BSSimpleList PushFront allocation-failure branch at %08X",
				kBSSimpleListPushFrontAllocFailureBranchSite);
			return false;
		}

		if (!WriteRelCall(kEffectSettingFilteredListHeadAllocPatchSite,
				(UInt32)&EffectSettingFilteredListHeadAllocPatch) ||
			!WriteBytes(kEffectSettingFilteredListNodeAllocFailureBranchSite,
				kEffectSettingFilteredListNodeAllocFailureReplacementBytes,
				sizeof(kEffectSettingFilteredListNodeAllocFailureReplacementBytes)))
		{
			Log("failed to write EffectSetting filtered-list allocation guards at %08X/%08X",
				kEffectSettingFilteredListHeadAllocPatchSite,
				kEffectSettingFilteredListNodeAllocFailureBranchSite);
			return false;
		}

		if (texturePaletteState == kTexturePalettePatchNeedsInstall &&
			!WriteRelJump(kTexturePaletteUnderscorePatchSite, (UInt32)&TexturePaletteUnderscorePatch, sizeof(kTexturePaletteExpectedBytes)))
		{
			Log("failed to write relative jump at %08X", kTexturePaletteUnderscorePatchSite);
			return false;
		}

		if (!WriteRelJump(kSaveLoadCreateBufferPatchSite,
				(UInt32)&SaveLoadCreateBufferPatch,
				sizeof(kSaveLoadCreateBufferExpectedBytes)) ||
			!WriteRelJump(kSaveLoadFreeBufferPatchSite,
				(UInt32)&SaveLoadFreeBufferPatch,
				sizeof(kSaveLoadFreeBufferExpectedBytes)) ||
			!WriteRelJump(kSaveLoadRawBlobMap54PatchSite,
				(UInt32)&SaveLoadRawBlobMap54Patch,
				sizeof(kSaveLoadRawBlobMap54ExpectedBytes)) ||
			!WriteRelJump(kSaveLoadRawBlobMap58PatchSite,
				(UInt32)&SaveLoadRawBlobMap58Patch,
				sizeof(kSaveLoadRawBlobMap58ExpectedBytes)) ||
			!WriteRelJump(kSaveLoadRawBlobMap5CPatchSite,
				(UInt32)&SaveLoadRawBlobMap5CPatch,
				sizeof(kSaveLoadRawBlobMap5CExpectedBytes)) ||
			!WriteRelJump(kSaveLoadRawBlobMap60PatchSite,
				(UInt32)&SaveLoadRawBlobMap60Patch,
				sizeof(kSaveLoadRawBlobMap60ExpectedBytes)))
		{
			Log("failed to write SaveLoad raw blob guards at %08X/%08X/%08X/%08X/%08X/%08X",
				kSaveLoadCreateBufferPatchSite,
				kSaveLoadFreeBufferPatchSite,
				kSaveLoadRawBlobMap54PatchSite,
				kSaveLoadRawBlobMap58PatchSite,
				kSaveLoadRawBlobMap5CPatchSite,
				kSaveLoadRawBlobMap60PatchSite);
			return false;
		}

		if (!WriteRelJump(kSaveLoadBoundedLoadDataPatchSite,
				(UInt32)&SaveLoadBoundedLoadDataPatch,
				sizeof(kSaveLoadBoundedLoadDataExpectedBytes)) ||
			!WriteRelJump(kSaveLoadActivateDeferredBufferPatchSite,
				(UInt32)&SaveLoadActivateDeferredBufferPatch,
				sizeof(kSaveLoadActivateDeferredBufferExpectedBytes)) ||
			!WriteRelJump(kSaveLoadAnimationBufferBeginPatchSite,
				(UInt32)&SaveLoadAnimationBufferBeginPatch,
				sizeof(kSaveLoadAnimationBufferBeginExpectedBytes)) ||
			!WriteRelJump(kSaveLoadAnimationBufferEndPatchSite,
				(UInt32)&SaveLoadAnimationBufferEndPatch,
				sizeof(kSaveLoadAnimationBufferEndExpectedBytes)) ||
			!WriteRelJump(kSaveLoadAttachedAnimationBufferBeginPatchSite,
				(UInt32)&SaveLoadAttachedAnimationBufferBeginPatch,
				sizeof(kSaveLoadAttachedAnimationBufferBeginExpectedBytes)) ||
			!WriteRelJump(kSaveLoadAttachedAnimationBufferEndPatchSite,
				(UInt32)&SaveLoadAttachedAnimationBufferEndPatch,
				sizeof(kSaveLoadAttachedAnimationBufferEndExpectedBytes)) ||
			!WriteRelJump(kSaveLoadCharacterBufferBeginPatchSite,
				(UInt32)&SaveLoadCharacterBufferBeginPatch,
				sizeof(kSaveLoadCharacterBufferBeginExpectedBytes)) ||
			!WriteRelJump(kSaveLoadCharacterBufferEndPatchSite,
				(UInt32)&SaveLoadCharacterBufferEndPatch,
				sizeof(kSaveLoadCharacterBufferEndExpectedBytes)) ||
			!WriteRelJump(kSaveLoadHavokBufferBeginPatchSite,
				(UInt32)&SaveLoadHavokBufferBeginPatch,
				sizeof(kSaveLoadHavokBufferBeginExpectedBytes)) ||
			!WriteRelJump(kSaveLoadHavokBufferEndPatchSite,
				(UInt32)&SaveLoadHavokBufferEndPatch,
				sizeof(kSaveLoadHavokBufferEndExpectedBytes)))
		{
			Log("failed to write SaveLoad buffer lifetime hooks");
			return false;
		}

		if (!WriteRelCall(kSaveLoadExpiredCellEntryAllocPatchSite,
				(UInt32)&SaveLoadExpiredCellEntryAllocPatch) ||
			!WriteRelCall(kSaveLoadExpiredCellListAllocPatchSite,
				(UInt32)&SaveLoadExpiredCellListAllocPatch))
		{
			Log("failed to write SaveLoad expired-cell allocation guards at %08X/%08X",
				kSaveLoadExpiredCellEntryAllocPatchSite,
				kSaveLoadExpiredCellListAllocPatchSite);
			return false;
		}

		if (!WriteRelJump(kContainerExtraDataListNodePatchSite,
			(UInt32)&ContainerExtraDataListNodeAllocPatch,
			sizeof(kContainerExtraDataListNodeExpectedBytes)))
		{
			Log("failed to write ContainerExtraData list-node allocation guard at %08X",
				kContainerExtraDataListNodePatchSite);
			return false;
		}

		if (!WriteRelCall(kExtraDataRegionListBufferAllocPatchSite,
			(UInt32)&ExtraDataRegionListBufferAllocPatch))
		{
			Log("failed to write ExtraData XCLR region-list buffer allocation guard at %08X",
				kExtraDataRegionListBufferAllocPatchSite);
			return false;
		}

		if (!WriteRelJump(kExtraDataLockDataAllocPatchSite,
				(UInt32)&ExtraDataLockDataAllocPatch,
				sizeof(kExtraDataLockDataAllocExpectedBytes)) ||
			!WriteRelJump(kExtraDataLockWrapperAllocPatchSite,
				(UInt32)&ExtraDataLockWrapperAllocPatch,
				sizeof(kExtraDataLockWrapperAllocExpectedBytes)))
		{
			Log("failed to write ExtraData XLOC allocation guards at %08X/%08X",
				kExtraDataLockDataAllocPatchSite,
				kExtraDataLockWrapperAllocPatchSite);
			return false;
		}

		if (!WriteRelJump(kExtraDataMapMarkerAllocPatchSite,
				(UInt32)&ExtraDataMapMarkerAllocPatch,
				sizeof(kExtraDataMapMarkerAllocExpectedBytes)) ||
			!WriteRelJump(kExtraDataTeleportAllocPatchSite,
				(UInt32)&ExtraDataTeleportAllocPatch,
				sizeof(kExtraDataTeleportAllocExpectedBytes)))
		{
			Log("failed to write ExtraData XMRK/XTEL allocation guards at %08X/%08X",
				kExtraDataMapMarkerAllocPatchSite,
				kExtraDataTeleportAllocPatchSite);
			return false;
		}

		if (!WriteRelJump(kExtraDataRagdollAllocPatchSite,
				(UInt32)&ExtraDataRagdollAllocPatch,
				sizeof(kExtraDataRagdollAllocExpectedBytes)) ||
			!WriteRelJump(kExtraDataRagdollPayloadAllocPatchSite,
				(UInt32)&ExtraDataRagdollPayloadAllocPatch,
				sizeof(kExtraDataRagdollPayloadAllocExpectedBytes)))
		{
			Log("failed to write ExtraData XRGD allocation guards at %08X/%08X",
				kExtraDataRagdollAllocPatchSite,
				kExtraDataRagdollPayloadAllocPatchSite);
			return false;
		}

		if (!WriteRelJump(kExtraDataRegionListAllocPatchSite,
			(UInt32)&ExtraDataRegionListAllocPatch,
			sizeof(kExtraDataRegionListAllocExpectedBytes)))
		{
			Log("failed to write ExtraData XCLR region-list object allocation guard at %08X",
				kExtraDataRegionListAllocPatchSite);
			return false;
		}

		if (!WriteRelCall(kAnimationChunkBufferAllocPatchSite,
			(UInt32)&AnimationChunkBufferAllocPatch))
		{
			Log("failed to write TESAnimation KFFZ chunk buffer guard at %08X",
				kAnimationChunkBufferAllocPatchSite);
			return false;
		}

		if (!WriteRelCall(kTESCreatureNIFZBufferAllocPatchSite,
			(UInt32)&PerfAMAllocateNifzPatch))
		{
			Log("failed to write TESCreature NIFZ chunk buffer guard at %08X",
				kTESCreatureNIFZBufferAllocPatchSite);
			return false;
		}

		if (!WriteRelJump(kScriptEventListOptionalDataAllocPatchSite,
			(UInt32)&ScriptEventListOptionalDataAllocPatch,
			sizeof(kScriptEventListOptionalDataAllocExpectedBytes)))
		{
			Log("failed to write ScriptEventList optional data allocation guard at %08X",
				kScriptEventListOptionalDataAllocPatchSite);
			return false;
		}

		if (!WriteRelCall(kArchiveFolderNameAllocPatchSite,
			(UInt32)&ArchiveFolderNameAllocPatch))
		{
			Log("failed to write Archive folder-name allocation guard at %08X",
				kArchiveFolderNameAllocPatchSite);
			return false;
		}

		if (!WriteRelCall(kBSFileWriteSwappedAllocPatchSite,
			(UInt32)&BSFileWriteSwappedAllocPatch))
		{
			Log("failed to write BSFile swapped-write allocation guard at %08X",
				kBSFileWriteSwappedAllocPatchSite);
			return false;
		}

		if (!WriteRelJump(kBSFileOpenBufferAllocPatchSite,
			(UInt32)&BSFileOpenBufferAllocPatch,
			sizeof(kBSFileOpenBufferAllocExpectedBytes)))
		{
			Log("failed to write BSFile open buffer allocation guard at %08X",
				kBSFileOpenBufferAllocPatchSite);
			return false;
		}

		if (!WriteRelCall(kModelWildcardPathAllocPatchSite,
			(UInt32)&ModelWildcardPathAllocPatch))
		{
			Log("failed to write model wildcard path allocation guard at %08X",
				kModelWildcardPathAllocPatchSite);
			return false;
		}

		if (!WriteRelCall(kKFListHeadAllocPatchSite,
				(UInt32)&KFListHeadAllocPatch) ||
			!WriteRelCall(kKFListStringAllocPatchSite,
				(UInt32)&KFListStringAllocPatch) ||
			!WriteRelJump(kKFListNodeAllocPatchSite,
				(UInt32)&KFListNodeAllocPatch,
				sizeof(kKFListNodeAllocExpectedBytes)))
		{
			Log("failed to write KF list allocation guards at %08X/%08X/%08X",
				kKFListHeadAllocPatchSite,
				kKFListStringAllocPatchSite,
				kKFListNodeAllocPatchSite);
			return false;
		}

		if (!WriteRelCall(kTESFileUpdateGroupsAllocPatchSite,
			(UInt32)&TESFileUpdateGroupsAllocPatch))
		{
			Log("failed to write TESFile update-open-groups allocation guard at %08X",
				kTESFileUpdateGroupsAllocPatchSite);
			return false;
		}

		// PERF-5 replaces the complete resolver, including checked allocation.
		// Retain the old signature; its interior allocation hook is superseded.

		if (!WriteRelJump(kArchiveMasterListAllocPatchSite,
			(UInt32)&ArchiveMasterListAllocPatch,
			sizeof(kArchiveMasterListAllocExpectedBytes)))
		{
			Log("failed to write archive master-list buffer allocation guard at %08X",
				kArchiveMasterListAllocPatchSite);
			return false;
		}

		if (!WriteRelJump(kArchiveMasterListAppendPatchSite,
			(UInt32)&ArchiveMasterListAppendPatch,
			sizeof(kArchiveMasterListAppendExpectedBytes)))
		{
			Log("failed to write archive master-list bounded append at %08X",
				kArchiveMasterListAppendPatchSite);
			return false;
		}

		if (!WriteRelJump(kArchivePluginsFileClosePatchSite,
			(UInt32)&ArchivePluginsFileClosePatch,
			sizeof(kArchivePluginsFileCloseExpectedBytes)))
		{
			Log("failed to write archive Plugins.txt close hook at %08X",
				kArchivePluginsFileClosePatchSite);
			return false;
		}

		if (!WriteRelJump(kArchiveSearchHandleClosePatchSite,
			(UInt32)&ArchiveSearchHandleClosePatch,
			sizeof(kArchiveSearchHandleCloseExpectedBytes)))
		{
			Log("failed to write archive search-handle close hook at %08X",
				kArchiveSearchHandleClosePatchSite);
			return false;
		}

		if (!WriteRelJump(kArchiveMasterListNewlinePatchSite,
			(UInt32)&ArchiveMasterListNewlinePatch,
			sizeof(kArchiveMasterListNewlineExpectedBytes)))
		{
			Log("failed to write archive master-list newline guard at %08X",
				kArchiveMasterListNewlinePatchSite);
			return false;
		}

		if (!WriteRelJump(kTESSpellListCountPatchSite,
				(UInt32)&TESSpellListCountPatch,
				sizeof(kTESSpellListCountExpectedBytes)) ||
			!WriteRelJump(kTESSpellListNodeAllocPatchSite,
				(UInt32)&TESSpellListNodeAllocPatch,
				sizeof(kTESSpellListNodeAllocExpectedBytes)) ||
			!WriteRelJump(kAVCollectionCountPatchSite,
				(UInt32)&AVCollectionCountPatch,
				sizeof(kAVCollectionCountExpectedBytes)) ||
			!WriteRelJump(kMiddleHighProcessFormIDCountPatchSite,
				(UInt32)&MiddleHighProcessFormIDCountPatch,
				sizeof(kMiddleHighProcessFormIDCountExpectedBytes)) ||
			!WriteRelJump(kMiddleHighProcessFormIDNodeAllocPatchSite,
				(UInt32)&MiddleHighProcessFormIDNodeAllocPatch,
				sizeof(kMiddleHighProcessFormIDNodeAllocExpectedBytes)) ||
			!WriteRelJump(kKnownEffectsCountPatchSite,
				(UInt32)&KnownEffectsCountPatch,
				sizeof(kKnownEffectsCountExpectedBytes)) ||
			!WriteRelJump(kActorRelationshipDispositionFirstPatchSite,
				(UInt32)&ActorRelationshipDispositionFirstLoopPatch,
				sizeof(kActorRelationshipDispositionFirstLoopExpectedBytes)) ||
			!WriteRelJump(kActorRelationshipDispositionSecondPatchSite,
				(UInt32)&ActorRelationshipDispositionSecondLoopPatch,
				sizeof(kActorRelationshipDispositionSecondLoopExpectedBytes)))
		{
			Log("failed to write save count/list guards at %08X/%08X/%08X/%08X/%08X/%08X/%08X/%08X",
				kTESSpellListCountPatchSite,
				kTESSpellListNodeAllocPatchSite,
				kAVCollectionCountPatchSite,
				kMiddleHighProcessFormIDCountPatchSite,
				kMiddleHighProcessFormIDNodeAllocPatchSite,
				kKnownEffectsCountPatchSite,
				kActorRelationshipDispositionFirstPatchSite,
				kActorRelationshipDispositionSecondPatchSite);
			return false;
		}

		if (kInstallArchiveStreamingGuards &&
			(!WriteRelJump(kArchiveRawReadTargetPatchSite,
					(UInt32)&ArchiveRawReadTargetGuardPatch,
					sizeof(kArchiveRawReadTargetExpectedBytes)) ||
				!WriteRelJump(kArchiveRawReadCountClampPatchSite,
					(UInt32)&ArchiveRawReadCountClampPatch,
					sizeof(kArchiveRawReadCountClampExpectedBytes)) ||
				!WriteRelJump(kCompressedArchiveRefillPatchSite,
					(UInt32)&CompressedArchiveRefillPatch,
					sizeof(kCompressedArchiveRefillExpectedBytes))))
		{
			Log("failed to write archive streaming guards at %08X/%08X/%08X",
				kArchiveRawReadTargetPatchSite,
				kArchiveRawReadCountClampPatchSite,
				kCompressedArchiveRefillPatchSite);
			return false;
		}

		if (!WriteRelJump(kTESFileNextRecordAdvancePatchSite,
				(UInt32)&TESFileNextRecordAdvancePatch,
				sizeof(kTESFileNextRecordAdvanceExpectedBytes)) ||
			!WriteRelJump(kTESFileGetNextChunkAdvancePatchSite,
				(UInt32)&TESFileGetNextChunkAdvancePatch,
				sizeof(kTESFileGetNextChunkAdvanceExpectedBytes)))
		{
			Log("failed to write TESFile parser guards at %08X/%08X",
				kTESFileNextRecordAdvancePatchSite,
				kTESFileGetNextChunkAdvancePatchSite);
			return false;
		}

		if (!WriteRelJump(kTESFileCompressedInputBoundsPatchSite,
				(UInt32)&TESFileCompressedInputBoundsPatch,
				sizeof(kTESFileCompressedInputBoundsExpectedBytes)) ||
			!WriteRelJump(kTESFileInflatedSizePatchSite,
				(UInt32)&TESFileInflatedSizePatch,
				sizeof(kTESFileInflatedSizeExpectedBytes)) ||
			!WriteRelJump(kTESFileInflateInitCleanupPatchSite,
				kTESFileInflateFailureCleanupDecodeAddr,
				sizeof(kTESFileInflateInitCleanupExpectedBytes)) ||
			!WriteRelJump(kTESFileDecompressedRecordSizePatchSite,
				(UInt32)&TESFileDecompressedRecordSizePatch,
				sizeof(kTESFileDecompressedRecordSizeExpectedBytes)) ||
			!WriteRelCall(kTESFileDecompressedRecordAllocPatchSite,
				(UInt32)&TESFileDecompressedRecordAllocPatch) ||
			!WriteRelJump(kTESFileLoadChunkHeaderCompressedTailPatchSite,
				(UInt32)&TESFileLoadChunkHeaderCompressedTailPatch,
				sizeof(kTESFileLoadChunkHeaderCompressedTailExpectedBytes)) ||
			!WriteRelJump(kTESFileGetChunkDataTruncatedCopyPatchSite,
				(UInt32)&TESFileGetChunkDataTruncatedCopyPatch,
				sizeof(kTESFileGetChunkDataTruncatedCopyExpectedBytes)) ||
			!WriteRelJump(kTESFileGetChunkDataFullCopyPatchSite,
				(UInt32)&TESFileGetChunkDataFullCopyPatch,
				sizeof(kTESFileGetChunkDataFullCopyExpectedBytes)) ||
			!WriteRelJump(kTESFileJumpToRecordValidatePatchSite,
				(UInt32)&TESFileJumpToRecordValidatePatch,
				sizeof(kTESFileJumpToRecordValidateExpectedBytes)) ||
			!WriteRelJump(kTESFileNextGroupPatchSite,
				(UInt32)&TESFileNextGroupPatch,
				sizeof(kTESFileNextGroupExpectedBytes)))
		{
			Log("failed to write TESFile compressed/seek guards at %08X/%08X/%08X/%08X/%08X/%08X/%08X",
				kTESFileDecompressedRecordSizePatchSite,
				kTESFileDecompressedRecordAllocPatchSite,
				kTESFileLoadChunkHeaderCompressedTailPatchSite,
				kTESFileGetChunkDataTruncatedCopyPatchSite,
				kTESFileGetChunkDataFullCopyPatchSite,
				kTESFileJumpToRecordValidatePatchSite,
				kTESFileNextGroupPatchSite);
			return false;
		}

		if (!WriteRelJump(kTESFormCompressRecordTempAllocPatchSite,
				(UInt32)&TESFormCompressRecordTempAllocPatch,
				sizeof(kTESFormCompressRecordTempAllocExpectedBytes)) ||
			!WriteRelJump(kTESFormCompressRecordDeflateErrorCleanupPatchSite,
				(UInt32)&TESFormCompressRecordDeflateErrorCleanupPatch,
				sizeof(kTESFormCompressRecordDeflateErrorCleanupExpectedBytes)) ||
			!WriteRelJump(kTESFormCompressRecordFinalAllocPatchSite,
				(UInt32)&TESFormCompressRecordFinalAllocPatch,
				sizeof(kTESFormCompressRecordFinalAllocExpectedBytes)))
		{
			Log("failed to write TESForm record compression memory guards at %08X/%08X/%08X",
				kTESFormCompressRecordTempAllocPatchSite,
				kTESFormCompressRecordDeflateErrorCleanupPatchSite,
				kTESFormCompressRecordFinalAllocPatchSite);
			return false;
		}

		if (worldspaceOFSTPatchState == kTESWorldSpaceOFSTPatchNeedsInstall &&
			(!WriteRelJump(kTESWorldSpaceLoadOFSTPatchSite,
				(UInt32)&TESWorldSpaceLoadOFSTPatch,
				sizeof(kTESWorldSpaceLoadOFSTExpectedBytes)) ||
			!WriteRelJump(kTESWorldSpaceFindCellOffsetPatchSite,
				(UInt32)&TESWorldSpaceFindCellOffsetPatch,
				sizeof(kTESWorldSpaceFindCellOffsetExpectedBytes)) ||
			!WriteRelJump(kTESWorldSpaceRebuildAllocPatchSite,
				(UInt32)&TESWorldSpaceRebuildAllocPatch,
				sizeof(kTESWorldSpaceRebuildAllocExpectedBytes)) ||
			!WriteRelJump(kTESWorldSpacePostFixupOffsetWritePatchSite,
				(UInt32)&TESWorldSpacePostFixupOffsetWritePatch,
				sizeof(kTESWorldSpacePostFixupOffsetWriteExpectedBytes)) ||
			!WriteRelJump(kTESWorldSpacePostFixupOffsetCommitPatchSite,
				(UInt32)&TESWorldSpacePostFixupOffsetCommitPatch,
				sizeof(kTESWorldSpacePostFixupOffsetCommitExpectedBytes)) ||
			!WriteRelJump(kTESWorldSpacePostFixupOffsetReadPatchSite,
				(UInt32)&TESWorldSpacePostFixupOffsetReadPatch,
				sizeof(kTESWorldSpacePostFixupOffsetReadExpectedBytes))))
		{
			Log("failed to write TESWorldSpace WRLD OFST guards at %08X/%08X/%08X/%08X/%08X",
				kTESWorldSpaceLoadOFSTPatchSite,
				kTESWorldSpaceFindCellOffsetPatchSite,
				kTESWorldSpacePostFixupOffsetWritePatchSite,
				kTESWorldSpacePostFixupOffsetCommitPatchSite,
				kTESWorldSpacePostFixupOffsetReadPatchSite);
			return false;
		}

		if (!WriteRelCall(kLoadingTextureWaitPatchSite, (UInt32)&LoadingTextureWaitPatch) ||
			!WriteRelJump(kLoadingTextureWaitPatchSite + 5, kLoadingTextureWaitContinueSite, 5) ||
			!WriteNop(kLoadingTextureWaitPatchSite + 10, sizeof(kLoadingTextureWaitExpectedBytes) - 10))
		{
			Log("failed to write loading texture wait patch at %08X", kLoadingTextureWaitPatchSite);
			return false;
		}

		if (!WriteRelJump(kLoadGameStartTickPatchSite, (UInt32)&LoadGameStartTickPatch, sizeof(kLoadGameStartTickExpectedBytes)))
		{
			Log("failed to write load-game start tick patch at %08X", kLoadGameStartTickPatchSite);
			return false;
		}

		if (!WriteRelCall(kWorldspaceMessageDelayPatchSite, (UInt32)&WorldspaceMessageDelayPatch) ||
			!WriteRelJump(kWorldspaceMessageDelayPatchSite + 5, kWorldspaceMessageDelayContinueSite, 5) ||
			!WriteNop(kWorldspaceMessageDelayPatchSite + 10, sizeof(kWorldspaceMessageDelayExpectedBytes) - 10))
		{
			Log("failed to write worldspace message delay patch at %08X", kWorldspaceMessageDelayPatchSite);
			return false;
		}

		if ((actorAnimPreSampleState == kActorAnimPreSamplePatchNeedsInstall &&
			(!WriteRelCall(kActorAnimUpdatePreSamplePatchSite, (UInt32)&ActorAnimPreSamplePatch) ||
			!WriteNop(kActorAnimUpdatePreSamplePatchSite + 5, sizeof(kActorAnimPreSamplePatchExpectedBytes) - 5))) ||
			(actorAnimClockPatchState == kActorAnimClockPatchNeedsInstall &&
			!WriteRelCall(kActorAnimUpdateClockPatchSite, (UInt32)&ActorAnimClockPatch)))
		{
			Log("failed to write ActorAnimData clock rebase patches at %08X/%08X",
				kActorAnimUpdatePreSamplePatchSite,
				kActorAnimUpdateClockPatchSite);
			return false;
		}

        return true;
    }
    #include "patch_installation.h"
