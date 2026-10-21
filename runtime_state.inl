	static HMODULE s_module = NULL;
	static HANDLE s_log = INVALID_HANDLE_VALUE;
	// Per-group installation ledger is declared in patch_groups.h.
	static bool s_patchInstallBlocked = false;
	static bool s_traceSaveReads = false;
	static mef::PatchTransaction* s_patchTransaction = NULL;
	static const char s_emptyString[] = "";
	static UInt32 s_actorAnimRebaseLogCount = 0;
	static UInt32 s_rejectedArchiveRawReadTargets = 0;
	static UInt32 s_clampedArchiveRawReads = 0;
	static UInt32 s_rejectedCompressedArchiveRefills = 0;
	static UInt32 s_rejectedTESFileRecordAdvances = 0;
	static UInt32 s_rejectedTESFileChunkAdvances = 0;
	static UInt32 s_rejectedTESFileCompressedRecords = 0;
	static UInt32 s_rejectedTESFileCompressedChunkHeaders = 0;
	static UInt32 s_clippedTESFileCompressedChunkCopies = 0;
	static UInt32 s_rejectedTESFileJumpTargets = 0;
	static UInt32 s_rejectedTESFileGroupAdvances = 0;
	static UInt32 s_skippedTESFormRecordCompressions = 0;
	static UInt32 s_loadedWorldspaceOFSTTables = 0;
	static UInt32 s_rejectedWorldspaceOFSTTables = 0;
	static UInt32 s_worldspaceOFSTTrackingFailures = 0;
	static UInt32 s_untrackedWorldspaceOFSTReads = 0;
	static UInt32 s_rejectedWorldspaceOFSTIndexes = 0;
	static UInt32 s_rejectedWorldspaceOFSTTargets = 0;
	static UInt32 s_trackedRebuiltWorldspaceOFSTTables = 0;
	static UInt32 s_rejectedRebuiltWorldspaceOFSTWrites = 0;
	static UInt32 s_trackedSaveBuffers = 0;
	static UInt32 s_saveBufferTrackingFailures = 0;
	static UInt32 s_clampedSaveRawBlobCopies = 0;
	static UInt32 s_untrackedSaveRawBlobCopies = 0;
	static UInt32 s_failedSaveRawBlobAllocations = 0;
	static UInt32 s_clampedSaveCountLoops = 0;
	static UInt32 s_skippedTESSpellListNodeAllocations = 0;
	static UInt32 s_skippedMiddleHighProcessFormIDNodes = 0;
	static UInt32 s_skippedActorRelationshipDispositionEntries = 0;
	static UInt32 s_skippedActorRelationshipDispositionHistoryNodes = 0;

	struct WorldspaceOFSTTrackingEntry
	{
		volatile UInt32 worldspace;
		volatile UInt32 table;
		volatile UInt32 count;
		volatile UInt32 byteLength;
	};

	static WorldspaceOFSTTrackingEntry s_worldspaceOFSTTracking[kWorldspaceOFSTTrackingCapacity];

	struct SaveBufferRangeTrackingEntry
	{
		UInt8* volatile owner;
		volatile UInt32 base;
		volatile UInt32 end;
		volatile UInt32 size;
		UInt32 scopeTop; // One-based index; suspended ranges exist only during a live loader call.
		UInt32 scopeOverflowDepth;
	};

	static SaveBufferRangeTrackingEntry s_saveBufferRanges[kSaveBufferRangeTrackingCapacity];
	struct SaveBufferScopeTrackingEntry
	{
		bool used;
		UInt32 parent;
		UInt32 base;
		UInt32 end;
		UInt32 size;
	};
	static SaveBufferScopeTrackingEntry s_saveBufferScopes[kSaveBufferRangeTrackingCapacity];

	enum ActorAnimPreSamplePatchState
	{
		kActorAnimPreSamplePatchMismatch = 0,
		kActorAnimPreSamplePatchNeedsInstall
	};

	enum ActorAnimClockPatchState
	{
		kActorAnimClockPatchMismatch = 0,
		kActorAnimClockPatchNeedsInstall
	};

	enum TESWorldSpaceOFSTPatchState
	{
		kTESWorldSpaceOFSTPatchMismatch = 0,
		kTESWorldSpaceOFSTPatchNeedsInstall,
		kTESWorldSpaceOFSTPatchExternallySkipped
	};

	enum ActorRelationshipDispositionLayout
	{
		kActorRelationshipValueThenForm = 0,
		kActorDispositionFormThenValue = 1
	};
