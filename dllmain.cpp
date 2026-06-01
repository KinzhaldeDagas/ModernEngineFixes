#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <float.h>

namespace
{
	typedef unsigned char UInt8;
	typedef unsigned short UInt16;
	typedef unsigned int UInt32;
	typedef int SInt32;
	typedef char (__thiscall* TESFileNextRecordFn)(UInt8* file);
	typedef void* (__cdecl* FormHeapAllocFn)(UInt32 size);
	typedef void (__cdecl* FormHeapFreeFn)(void* ptr);
	typedef char (__thiscall* TESFileGetChunkDataFn)(UInt8* file, char* dst, UInt32 maxSize);
	typedef char (__thiscall* NiTMapGetAtFn)(void* map, UInt32 key, UInt32* outValue);
	typedef int (__thiscall* NiTMapSetAtFn)(void* map, UInt32 key, void* value);
	typedef void (__thiscall* TESFormLoadCurrentSaveGameFn)(void* form, void* dst, UInt32 size);
	typedef void (__cdecl* PrintErrorFn)(const char* message);
	typedef int (__cdecl* ZlibStreamEndFn)(void* stream);

	static const UInt32 kPluginVersion = 29;
	static const UInt32 kPluginInfoVersion = 2;
	static const UInt32 kOblivionVersion_1_2_0_416 = 0x010201A0;

	static const UInt32 kMainThreadHandlePatchSite = 0x00404A55;
	static const UInt32 kMainThreadHandleContinueSite = 0x00404A68;
	static const UInt32 kBinkOpenThreadHandleUseDecodeAddr = 0x00410229;
	static const UInt32 kBinkOpenResumeThreadUseDecodeAddr = 0x0041028E;

	static const UInt32 kRendererInitFailurePatchSite = 0x004983C0;
	static const UInt32 kRendererInitFailureContinueSite = 0x004983C5;
	static const UInt32 kRendererInitFailureReturnSite = 0x00498E7D;

	static const UInt32 kTexturePaletteUnderscorePatchSite = 0x004A26F7;
	static const UInt32 kTexturePaletteContinueSite = 0x004A26FE;
	static const UInt32 kTexturePaletteFallbackSite = 0x004A27A1;

	static const UInt32 kArchiveRawReadTargetPatchSite = 0x0042C3EF;
	static const UInt32 kArchiveRawReadTargetContinueSite = 0x0042C3FD;
	static const UInt32 kArchiveRawReadCountClampPatchSite = 0x0042C439;
	static const UInt32 kArchiveRawReadCountClampContinueSite = 0x0042C45A;
	static const UInt32 kCompressedArchiveRefillPatchSite = 0x0042C7CA;
	static const UInt32 kCompressedArchiveRefillLoopSite = 0x0042C780;
	static const UInt32 kCompressedArchiveRefillDoneSite = 0x0042C818;
	static const UInt32 kCompressedArchiveRefillErrorSite = 0x0042C7F3;
	// Runtime WER crashes landed in CompressedArchiveFile::Read while this
	// load-critical archive group was installed. Keep the decode, do not patch.
	static const bool kInstallArchiveStreamingGuards = false;

	static const UInt32 kTESFileNextRecordAdvancePatchSite = 0x0045132A;
	static const UInt32 kTESFileNextRecordAdvanceContinueSite = 0x00451342;
	static const UInt32 kTESFileNextRecordAdvanceStopSite = 0x00451350;
	static const UInt32 kTESFileGetNextChunkAdvancePatchSite = 0x0044FECB;
	static const UInt32 kTESFileGetNextChunkAdvanceContinueSite = 0x0044FEF7;
	static const UInt32 kTESFileGetNextChunkAdvanceStopSite = 0x0044FEE4;
	static const UInt32 kTESFileLoadChunkHeaderCompressedTailPatchSite = 0x00450F97;
	static const UInt32 kTESFileLoadChunkHeaderContinueSite = 0x00450FB3;
	static const UInt32 kTESFileLoadChunkHeaderFailSite = 0x00450F65;
	static const UInt32 kTESFileDecompressedRecordSizePatchSite = 0x004505EF;
	static const UInt32 kTESFileDecompressedRecordContinueSite = 0x004505F6;
	static const UInt32 kTESFileDecompressedRecordFailSite = 0x004506F6;
	static const UInt32 kTESFileGetChunkDataTruncatedCopyPatchSite = 0x00450D66;
	static const UInt32 kTESFileGetChunkDataTruncatedCopyContinueSite = 0x00450D84;
	static const UInt32 kTESFileGetChunkDataFullCopyPatchSite = 0x00450EC8;
	static const UInt32 kTESFileGetChunkDataFullCopyContinueSite = 0x00450EED;
	static const UInt32 kTESFileJumpToRecordValidatePatchSite = 0x00451487;
	static const UInt32 kTESFileJumpToRecordSeekSite = 0x004514C0;
	static const UInt32 kTESFileNextRecordAddr = 0x00451310;
	static const UInt32 kTESFileNextGroupPatchSite = 0x00451580;
	static const UInt32 kTESFormCompressRecordTempAllocPatchSite = 0x0046B3E8;
	static const UInt32 kTESFormCompressRecordTempAllocContinueSite = 0x0046B3F9;
	static const UInt32 kTESFormCompressRecordDeflateErrorCleanupPatchSite = 0x0046B427;
	static const UInt32 kTESFormCompressRecordFinalAllocPatchSite = 0x0046B449;
	static const UInt32 kTESFormCompressRecordFinalAllocContinueSite = 0x0046B450;
	static const UInt32 kTESFormCompressRecordFailureReturnSite = 0x0046B430;
	static const UInt32 kTESRecordTypeGRUPAddr = 0x00B05E20;
	static const UInt32 kMaxTESFileDecompressedRecordBytes = 64 * 1024 * 1024;

	static const UInt32 kFormHeapAllocAddr = 0x00401F00;
	static const UInt32 kFormHeapFreeAddr = 0x00401F20;
	static const UInt32 kZlibStreamEndAddr = 0x00743E50;
	static const UInt32 kTESFileGetChunkDataAddr = 0x00450C20;
	static const UInt32 kTESFileJumpToRecordAddr = 0x00451460;
	static const UInt32 kTESWorldSpaceLoadOFSTPatchSite = 0x004F2150;
	static const UInt32 kTESWorldSpaceLoadOFSTContinueSite = 0x004F21B6;
	static const UInt32 kTESWorldSpaceFindCellOffsetPatchSite = 0x004EF55E;
	static const UInt32 kTESWorldSpaceFindCellOffsetSuccessSite = 0x004EF575;
	static const UInt32 kTESWorldSpaceFindCellOffsetNoOffsetSite = 0x004EF580;
	static const UInt32 kTESWorldSpaceFindCellOffsetFallbackSite = 0x004EF58B;
	static const UInt32 kTESWorldSpacePostFixupOffsetWritePatchSite = 0x004F305E;
	static const UInt32 kTESWorldSpacePostFixupOffsetWriteContinueSite = 0x004F3065;
	static const UInt32 kTESWorldSpacePostFixupOffsetCommitPatchSite = 0x004F30A9;
	static const UInt32 kTESWorldSpacePostFixupOffsetCommitContinueSite = 0x004F3268;
	static const UInt32 kTESWorldSpacePostFixupOffsetReadPatchSite = 0x004F3195;
	static const UInt32 kTESWorldSpacePostFixupOffsetNonZeroSite = 0x004F31A6;
	static const UInt32 kTESWorldSpacePostFixupOffsetSkipSite = 0x004F324E;
	static const UInt32 kMaxWorldspaceOFSTBytes = 64 * 1024 * 1024;
	static const UInt32 kWorldspaceOFSTTrackingCapacity = 4096;

	static const UInt32 kLoadingTextureWaitPatchSite = 0x004109EB;
	static const UInt32 kLoadingTextureWaitContinueSite = 0x00410A49;
	static const UInt32 kLoadingTextureWaitPumpAddr = 0x00410390;
	static const UInt32 kLoadingTextureWaitSecondsAddr = 0x00B030AC;
	static const float kStaticLoadingScreenWaitScale = 0.70f;

	static const UInt32 kLoadGameStartTickPatchSite = 0x00459A43;
	static const UInt32 kLoadGameStartTickElapsedSite = 0x00459A63;
	static const UInt32 kLoadGameStartTickGlobalAddr = 0x00B33B08;

	static const UInt32 kWorldspaceMessageDelayPatchSite = 0x005BDD35;
	static const UInt32 kWorldspaceMessageDelayContinueSite = 0x005BDD5E;
	static const UInt32 kWorldspaceMessageDelayStep1Addr = 0x005791A0;
	static const UInt32 kWorldspaceMessageDelayStep2Addr = 0x00579220;
	static const UInt32 kWorldspaceMessageDelayInputGlobalsAddr = 0x00B33398;
	static const UInt32 kWorldspaceMessageDelayInputUpdateAddr = 0x0040D4D0;

	static const UInt32 kGlobalAnimTimerLoadPatchSite = 0x004413DB;
	static const UInt32 kGlobalAnimTimerLoadContinueSite = 0x004413E0;
	static const UInt32 kSaveLoadCreateBufferPatchSite = 0x00453500;
	static const UInt32 kSaveLoadFreeBufferPatchSite = 0x00452230;
	static const UInt32 kSaveLoadLoadDataAddr = 0x004534D0;
	static const UInt32 kSaveLoadRawBlobMap54PatchSite = 0x00458E50;
	static const UInt32 kSaveLoadRawBlobMap58PatchSite = 0x00459020;
	static const UInt32 kSaveLoadRawBlobMap5CPatchSite = 0x004590E0;
	static const UInt32 kSaveLoadRawBlobMap60PatchSite = 0x00459310;
	static const UInt32 kTESSpellListCountPatchSite = 0x0046F9CC;
	static const UInt32 kTESSpellListNodeAllocPatchSite = 0x0046FA4F;
	static const UInt32 kAVCollectionCountPatchSite = 0x0065CBC9;
	static const UInt32 kMiddleHighProcessFormIDCountPatchSite = 0x00656AA4;
	static const UInt32 kMiddleHighProcessFormIDCountLoopSite = 0x00656AB0;
	static const UInt32 kMiddleHighProcessFormIDCountDoneSite = 0x00656B1F;
	static const UInt32 kMiddleHighProcessFormIDNodeAllocPatchSite = 0x00656ADF;
	static const UInt32 kMiddleHighProcessFormIDNodeAllocContinueSite = 0x00656AE9;
	static const UInt32 kMiddleHighProcessFormIDNodeAllocSkipSite = 0x00656B13;
	static const UInt32 kKnownEffectsCountPatchSite = 0x00416EB1;
	static const UInt32 kKnownEffectsCountLoopSite = 0x00416EC0;
	static const UInt32 kKnownEffectsCountDoneSite = 0x00416F12;
	static const UInt32 kActorRelationshipDispositionFirstPatchSite = 0x00602626;
	static const UInt32 kActorRelationshipDispositionFirstContinueSite = 0x006026A2;
	static const UInt32 kActorRelationshipDispositionSecondPatchSite = 0x00602707;
	static const UInt32 kActorRelationshipDispositionSecondContinueSite = 0x00602788;
	static const UInt32 kTESFormLoadDataFromCurrentSaveGameAddr = 0x0046AC80;
	static const UInt32 kTESFormLoadFormIDFromCurrentSaveGameAddr = 0x0046ACA0;
	static const UInt32 kNiTMapGetAtAddr = 0x0055E000;
	static const UInt32 kNiTMapSetAtAddr = 0x00452570;
	static const UInt32 kPrintErrorAddr = 0x00404EC0;
	static const UInt32 kGlobalAnimTimerAddr = 0x00B33A30;
	static const UInt32 kGlobalAnimTimerUpdateDecodeAddr = 0x00442536;
	static const UInt32 kGlobalAnimTimerSceneUpdateDecodeAddr = 0x004425A8;
	static const float kGlobalAnimTimerPrecisionLimit = 131072.0f;

	static const UInt32 kActorAnimRestoreClockWriteSite = 0x00474BB2;
	static const UInt32 kActorAnimUpdatePreSamplePatchSite = 0x00476E86;
	static const UInt32 kActorAnimUpdateClockWriteSite = 0x00476F97;
	static const UInt32 kActorAnimUpdateClockPatchSite = 0x00476FA6;
	static const UInt32 kBSAnimGroupSequenceSampleUpdateAddr = 0x0049F4A0;
	static const UInt32 kBSAnimGroupSequenceSaveStateAddr = 0x0049F570;
	static const UInt32 kBSAnimGroupSequenceLoadStateAddr = 0x0049F5F0;
	static const UInt32 kNiControllerSequenceUpdateAddr = 0x006C5FC0;
	static const UInt32 kActorAnimClockOffset = 0x94;
	static const UInt32 kActorAnimSequenceListOffset = 0xA0;
	static const UInt32 kNiControllerSequenceOffsetOffset = 0x48;
	static const UInt32 kActorAnimSequenceCount = 5;
	static const UInt32 kSaveBufferRangeTrackingCapacity = 256;
	static const float kActorAnimRebaseThreshold = 4096.0f;
	static const float kActorAnimRebaseStep = 2048.0f;
	static const float kSentinelMagnitude = 1.0e30f;

	static const UInt32 kActorGetAttackedPatchSite = 0x005E58C0;
	static const UInt32 kActorGetAttackedContinueSite = 0x005E58C5;
	static const UInt32 kActorAdjacentProcessGuardDecodeAddr = 0x005E58D0;
	static const UInt32 kActorIsTalkingPatchSite = 0x005E0E62;
	static const UInt32 kActorIsTalkingContinueSite = 0x005E0E67;
	static const UInt32 kActorIsTalkingReturnSite = 0x005E0E70;
	static const UInt32 kActorIsTalkingAdjacentGuardDecodeAddr = 0x005E0E80;

	static HMODULE s_module = NULL;
	static HANDLE s_log = INVALID_HANDLE_VALUE;
	static bool s_patchInstalled = false;
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
		volatile UInt32 owner;
		volatile UInt32 base;
		volatile UInt32 end;
		volatile UInt32 size;
	};

	static SaveBufferRangeTrackingEntry s_saveBufferRanges[kSaveBufferRangeTrackingCapacity];

	enum TexturePalettePatchState
	{
		kTexturePalettePatchMismatch = 0,
		kTexturePalettePatchNeedsInstall,
		kTexturePalettePatchAlreadyGuarded
	};

	enum GlobalAnimTimerLoadPatchState
	{
		kGlobalAnimTimerLoadPatchMismatch = 0,
		kGlobalAnimTimerLoadPatchNeedsInstall,
		kGlobalAnimTimerLoadPatchAlreadyGuarded
	};

	enum RendererInitFailurePatchState
	{
		kRendererInitFailurePatchMismatch = 0,
		kRendererInitFailurePatchNeedsInstall,
		kRendererInitFailurePatchAlreadyGuarded
	};

	enum ActorGetAttackedPatchState
	{
		kActorGetAttackedPatchMismatch = 0,
		kActorGetAttackedPatchNeedsInstall,
		kActorGetAttackedPatchAlreadyGuarded
	};

	enum ActorIsTalkingPatchState
	{
		kActorIsTalkingPatchMismatch = 0,
		kActorIsTalkingPatchNeedsInstall,
		kActorIsTalkingPatchAlreadyGuarded
	};

	enum ActorAnimPreSamplePatchState
	{
		kActorAnimPreSamplePatchMismatch = 0,
		kActorAnimPreSamplePatchNeedsInstall,
		kActorAnimPreSamplePatchAlreadyInstalled
	};

	enum ActorAnimClockPatchState
	{
		kActorAnimClockPatchMismatch = 0,
		kActorAnimClockPatchNeedsInstall,
		kActorAnimClockPatchAlreadyInstalled
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

	static const UInt8 kMainThreadHandleExpectedBytes[] =
	{
		0xFF, 0x15, 0xC4, 0x80, 0xA2, 0x00,
		0x53,
		0x53,
		0x53,
		0x57,
		0x53,
		0x50,
		0x53,
		0xFF, 0x15, 0xC0, 0x80, 0xA2, 0x00
	};

	static const UInt8 kTexturePaletteExpectedBytes[] =
	{
		0x8B, 0xF0,             // mov esi, eax
		0x2B, 0xF3,             // sub esi, ebx
		0x83, 0xC4, 0x08        // add esp, 8
	};

	static const UInt8 kTexturePaletteGuardThunkExpectedBytes[] =
	{
		0x8B, 0xF0,                         // mov esi, eax
		0x2B, 0xF3,                         // sub esi, ebx
		0x83, 0xC4, 0x08,                   // add esp, 8
		0x85, 0xC0,                         // test eax, eax
		0x75, 0x06,                         // jnz +6
		0x68, 0xA1, 0x27, 0x4A, 0x00,       // push 004A27A1h
		0xC3,                               // ret
		0x68, 0xFE, 0x26, 0x4A, 0x00,       // push 004A26FEh
		0xC3                                // ret
	};

	static const UInt8 kArchiveRawReadTargetExpectedBytes[] =
	{
		0x8B, 0xBE, 0x58, 0x01, 0x00, 0x00,
		0x03, 0xBE, 0x48, 0x01, 0x00, 0x00,
		0x03, 0xFB
	};

	static const UInt8 kArchiveRawReadCountClampExpectedBytes[] =
	{
		0x8B, 0x96, 0x48, 0x01, 0x00, 0x00,
		0x8B, 0x4C, 0x24, 0x14,
		0x8B, 0x86, 0x50, 0x01, 0x00, 0x00,
		0x55,
		0x8D, 0x2C, 0x0A,
		0x03, 0xEB,
		0x3B, 0xE8,
		0x5D,
		0x76, 0x06,
		0x2B, 0xC2,
		0x2B, 0xC3,
		0x8B, 0xC8
	};

	static const UInt8 kCompressedArchiveRefillExpectedBytes[] =
	{
		0x83, 0x7B, 0x04, 0x00,
		0x75, 0x1B,
		0x8B, 0xCE,
		0xE8, 0x59, 0xB5, 0x31, 0x00,
		0x8B, 0x4E, 0x0C,
		0x8B, 0x56, 0x18,
		0x6A, 0x00,
		0x51,
		0x52,
		0x8B, 0xCE,
		0xE8, 0xF8, 0xFB, 0xFF, 0xFF,
		0x89, 0x46, 0x10,
		0x83, 0x7B, 0x10, 0x00,
		0x75, 0x8F,
		0xEB, 0x25
	};

	static const UInt8 kTESFileNextRecordAdvanceExpectedBytes[] =
	{
		0x8B, 0x86, 0x40, 0x02, 0x00, 0x00,
		0x83, 0xC0, 0x14,
		0x01, 0x86, 0x5C, 0x02, 0x00, 0x00,
		0xEB, 0x07
	};

	static const UInt8 kTESFileGetNextChunkAdvanceExpectedBytes[] =
	{
		0x8B, 0x86, 0x54, 0x02, 0x00, 0x00,
		0x83, 0xC0, 0x06,
		0x01, 0x86, 0x60, 0x02, 0x00, 0x00,
		0x8B, 0x86, 0x60, 0x02, 0x00, 0x00,
		0x3B, 0xC2,
		0x72, 0x13
	};

	static const UInt8 kTESFileLoadChunkHeaderCompressedTailExpectedBytes[] =
	{
		0x3B, 0xBE, 0x18, 0x04, 0x00, 0x00,
		0x73, 0xC6,
		0x8B, 0x0C, 0x38,
		0x03, 0xC7,
		0x89, 0x4C, 0x24, 0x10,
		0x66, 0x8B, 0x50, 0x04,
		0x66, 0x89, 0x54, 0x24, 0x14,
		0xEB, 0x04
	};

	static const UInt8 kTESFileDecompressedRecordSizeExpectedBytes[] =
	{
		0x8B, 0x2F,
		0x55,
		0xC6, 0x04, 0x3B, 0x00
	};

	static const UInt8 kTESFileGetChunkDataTruncatedCopyExpectedBytes[] =
	{
		0x8B, 0x8E, 0x14, 0x04, 0x00, 0x00,
		0x03, 0x4C, 0x24, 0x10,
		0x8D, 0x5F, 0xFF,
		0x53,
		0x51,
		0x55,
		0xE8, 0x45, 0x05, 0x53, 0x00,
		0x83, 0xC4, 0x0C,
		0x89, 0x9E, 0x64, 0x02, 0x00, 0x00
	};

	static const UInt8 kTESFileGetChunkDataFullCopyExpectedBytes[] =
	{
		0x8B, 0x8E, 0x54, 0x02, 0x00, 0x00,
		0x8B, 0x96, 0x14, 0x04, 0x00, 0x00,
		0x51,
		0x03, 0xD7,
		0x52,
		0x55,
		0xE8, 0xE2, 0x03, 0x53, 0x00,
		0x8B, 0x86, 0x54, 0x02, 0x00, 0x00,
		0x83, 0xC4, 0x0C,
		0x89, 0x86, 0x64, 0x02, 0x00, 0x00
	};

	static const UInt8 kTESFileJumpToRecordValidateExpectedBytes[] =
	{
		0x8B, 0x44, 0x24, 0x0C,
		0x3B, 0x86, 0x58, 0x02, 0x00, 0x00,
		0x89, 0x86, 0x5C, 0x02, 0x00, 0x00,
		0x72, 0x27,
		0x33, 0xC0,
		0x89, 0x86, 0x3C, 0x02, 0x00, 0x00,
		0x89, 0x86, 0x40, 0x02, 0x00, 0x00,
		0x89, 0x86, 0x44, 0x02, 0x00, 0x00,
		0x89, 0x86, 0x48, 0x02, 0x00, 0x00,
		0x89, 0x86, 0x4C, 0x02, 0x00, 0x00,
		0x5F,
		0x32, 0xC0,
		0x5E,
		0xC2, 0x04, 0x00
	};

	static const UInt8 kTESFileNextGroupExpectedBytes[] =
	{
		0x8B, 0x91, 0x3C, 0x02, 0x00, 0x00
	};

	static const UInt8 kTESFormCompressRecordTempAllocExpectedBytes[] =
	{
		0x55,
		0x56,
		0x8D, 0x34, 0x1B,
		0x56,
		0xE8, 0x0D, 0x6B, 0xF9, 0xFF,
		0x8B, 0xE8,
		0x8B, 0x44, 0x24, 0x14
	};

	static const UInt8 kTESFormCompressRecordDeflateErrorCleanupExpectedBytes[] =
	{
		0x55,
		0xE8, 0xF3, 0x6A, 0xF9, 0xFF
	};

	static const UInt8 kTESFormCompressRecordFinalAllocExpectedBytes[] =
	{
		0x81, 0x4F, 0x08, 0x00, 0x00, 0x04, 0x00
	};

	static const UInt8 kTESWorldSpaceLoadOFSTExpectedBytes[] =
	{
		0x8B, 0x87, 0x54, 0x02, 0x00, 0x00,
		0x3B, 0xC3,
		0x74, 0x5C,
		0xC1, 0xE8, 0x02,
		0x8B, 0xD8,
		0x8B, 0x86, 0xA8, 0x00, 0x00, 0x00,
		0x85, 0xC0,
		0x74, 0x09,
		0x50,
		0xE8, 0xB1, 0xFD, 0xF0, 0xFF,
		0x83, 0xC4, 0x04,
		0x33, 0xC9,
		0x8B, 0xC3,
		0xBA, 0x04, 0x00, 0x00, 0x00,
		0xF7, 0xE2,
		0x0F, 0x90, 0xC1,
		0xF7, 0xD9,
		0x0B, 0xC8,
		0x51,
		0xE8, 0x76, 0xFD, 0xF0, 0xFF,
		0x83, 0xC4, 0x04,
		0x6A, 0x00,
		0x50,
		0x8B, 0xCF,
		0x89, 0x86, 0xA8, 0x00, 0x00, 0x00,
		0xE8, 0x83, 0xEA, 0xF5, 0xFF,
		0x33, 0xDB,
		0xEB, 0x15
	};

	static const UInt8 kTESWorldSpaceFindCellOffsetExpectedBytes[] =
	{
		0x8B, 0x04, 0x87,
		0x85, 0xC0,
		0x74, 0x1B,
		0x8B, 0x93, 0xBC, 0x00, 0x00, 0x00,
		0x03, 0xD0,
		0x52,
		0x8B, 0xCE,
		0xE8, 0xEB, 0x1E, 0xF6, 0xFF
	};

	static const UInt8 kTESWorldSpacePostFixupOffsetWriteExpectedBytes[] =
	{
		0x8B, 0x54, 0x24, 0x20,
		0x89, 0x0C, 0x82
	};

	static const UInt8 kTESWorldSpacePostFixupOffsetCommitExpectedBytes[] =
	{
		0x8B, 0x5C, 0x24, 0x20,
		0x89, 0x9E, 0xA8, 0x00, 0x00, 0x00
	};

	static const UInt8 kTESWorldSpacePostFixupOffsetReadExpectedBytes[] =
	{
		0x8B, 0x8E, 0xA8, 0x00, 0x00, 0x00,
		0x8B, 0x0C, 0x81,
		0x3B, 0xCF
	};

	static const UInt8 kLoadingTextureWaitExpectedBytes[] =
	{
		0x8B, 0x35, 0xD0, 0x80, 0xA2, 0x00,
		0xFF, 0xD6,
		0x85, 0xC0,
		0x89, 0x44, 0x24, 0x10,
		0xDB, 0x44, 0x24, 0x10,
		0x7D, 0x06,
		0xD8, 0x05, 0x78, 0xFC, 0xA2, 0x00,
		0xD9, 0x05, 0xAC, 0x30, 0xB0, 0x00,
		0xDC, 0x0D, 0x70, 0xFC, 0xA2, 0x00,
		0xD9, 0x7C, 0x24, 0x08,
		0x0F, 0xB7, 0x44, 0x24, 0x08,
		0xDE, 0xC1,
		0x0D, 0x00, 0x0C, 0x00, 0x00,
		0x89, 0x44, 0x24, 0x10,
		0xD9, 0x6C, 0x24, 0x10,
		0xDF, 0x7C, 0x24, 0x10,
		0x8B, 0x7C, 0x24, 0x10,
		0xD9, 0x6C, 0x24, 0x08,
		0xFF, 0xD6,
		0x3B, 0xF8,
		0x76, 0x0E,
		0x6A, 0x01,
		0xE8, 0x4E, 0xF9, 0xFF, 0xFF,
		0x83, 0xC4, 0x04,
		0x84, 0xC0,
		0x75, 0xEC
	};

	static const UInt8 kLoadGameStartTickExpectedBytes[] =
	{
		0x8B, 0x35, 0xD0, 0x80, 0xA2, 0x00,
		0xFF, 0xD6,
		0xA3, 0x08, 0x3B, 0xB3, 0x00,
		0xFF, 0xD6,
		0x8B, 0x0D, 0x08, 0x3B, 0xB3, 0x00,
		0x81, 0xC1, 0xB8, 0x0B, 0x00, 0x00,
		0x3B, 0xC1,
		0x5E,
		0x76, 0x2B
	};

	static const UInt8 kWorldspaceMessageDelayExpectedBytes[] =
	{
		0xFF, 0xD6,
		0x8D, 0xB8, 0xE8, 0x03, 0x00, 0x00,
		0xFF, 0xD6,
		0x3B, 0xC7,
		0x73, 0x1B,
		0xE8, 0x58, 0xB4, 0xFB, 0xFF,
		0xE8, 0xD3, 0xB4, 0xFB, 0xFF,
		0x8B, 0x0D, 0x98, 0x33, 0xB3, 0x00,
		0xE8, 0x78, 0xF7, 0xE4, 0xFF,
		0xFF, 0xD6,
		0x3B, 0xC7,
		0x72, 0xE5
	};

	static const UInt8 kGlobalAnimTimerLoadExpectedBytes[] =
	{
		0xE8, 0xF0, 0x20, 0x01, 0x00
	};

	static const UInt8 kSaveLoadCreateBufferExpectedBytes[] =
	{
		0x8B, 0x44, 0x24, 0x04,
		0x56
	};

	static const UInt8 kSaveLoadFreeBufferExpectedBytes[] =
	{
		0x8B, 0x44, 0x24, 0x04,
		0x56
	};

	static const UInt8 kSaveLoadRawBlobMap54ExpectedBytes[] =
	{
		0x51,
		0x8B, 0x44, 0x24, 0x08
	};

	static const UInt8 kSaveLoadRawBlobMap58ExpectedBytes[] =
	{
		0x8B, 0x44, 0x24, 0x04,
		0x53
	};

	static const UInt8 kSaveLoadRawBlobMap5CExpectedBytes[] =
	{
		0x8B, 0x44, 0x24, 0x04,
		0x53
	};

	static const UInt8 kSaveLoadRawBlobMap60ExpectedBytes[] =
	{
		0x8B, 0x44, 0x24, 0x04,
		0x53
	};

	static const UInt8 kTESSpellListCountExpectedBytes[] =
	{
		0x66, 0x83, 0x7C, 0x24, 0x24, 0x00,
		0xC7, 0x44, 0x24, 0x18, 0x00, 0x00, 0x00, 0x00,
		0x0F, 0x86, 0xEF, 0x00, 0x00, 0x00
	};

	static const UInt8 kTESSpellListNodeAllocExpectedBytes[] =
	{
		0x85, 0xC0,
		0x74, 0x18,
		0x8B, 0x17
	};

	static const UInt8 kAVCollectionCountExpectedBytes[] =
	{
		0x33, 0xFF,
		0x66, 0x39, 0x7C, 0x24, 0x0C,
		0x0F, 0x86, 0xAD, 0x00, 0x00, 0x00
	};

	static const UInt8 kMiddleHighProcessFormIDCountExpectedBytes[] =
	{
		0x33, 0xDB,
		0x66, 0x39, 0x5C, 0x24, 0x64,
		0x76, 0x72
	};

	static const UInt8 kMiddleHighProcessFormIDNodeAllocExpectedBytes[] =
	{
		0x85, 0xC0,
		0x74, 0x11,
		0x8B, 0x8E, 0xA8, 0x00, 0x00, 0x00
	};

	static const UInt8 kKnownEffectsCountExpectedBytes[] =
	{
		0x33, 0xF6,
		0x39, 0x6C, 0x24, 0x14,
		0x7E, 0x59,
		0xBF, 0x00, 0x00, 0x20, 0x00
	};

	static const UInt8 kActorRelationshipDispositionFirstLoopExpectedBytes[] =
	{
		0x33, 0xED,
		0x66, 0x39, 0x6C, 0x24, 0x10,
		0x76, 0x73,
		0x33, 0xDB,
		0x6A, 0x08,
		0xE8, 0xC8, 0xF8, 0xDF, 0xFF,
		0x83, 0xC4, 0x04,
		0x6A, 0x04,
		0x8D, 0x4C, 0x24, 0x20,
		0x51,
		0x8B, 0xCE,
		0x8B, 0xF8,
		0xE8, 0x55, 0x86, 0xE6, 0xFF,
		0x8B, 0x54, 0x24, 0x1C,
		0x6A, 0x04,
		0x57,
		0x8B, 0xCE,
		0x89, 0x57, 0x04,
		0xE8, 0x24, 0x86, 0xE6, 0xFF,
		0x39, 0x9E, 0xA4, 0x00, 0x00, 0x00,
		0x74, 0x2C,
		0x6A, 0x08,
		0xE8, 0x95, 0xF8, 0xDF, 0xFF,
		0x83, 0xC4, 0x04,
		0x3B, 0xC3,
		0x74, 0x0D,
		0x8B, 0x8E, 0xA4, 0x00, 0x00, 0x00,
		0x89, 0x08,
		0x89, 0x58, 0x04,
		0xEB, 0x02,
		0x33, 0xC0,
		0x8B, 0x96, 0xA8, 0x00, 0x00, 0x00,
		0x89, 0x50, 0x04,
		0x89, 0x86, 0xA8, 0x00, 0x00, 0x00,
		0x0F, 0xB7, 0x44, 0x24, 0x10,
		0x83, 0xC5, 0x01,
		0x3B, 0xE8,
		0x89, 0xBE, 0xA4, 0x00, 0x00, 0x00,
		0x72, 0x8F
	};

	static const UInt8 kActorRelationshipDispositionSecondLoopExpectedBytes[] =
	{
		0x33, 0xDB,
		0x66, 0x39, 0x5C, 0x24, 0x28,
		0x76, 0x78,
		0x6A, 0x08,
		0xE8, 0xE9, 0xF7, 0xDF, 0xFF,
		0x83, 0xC4, 0x04,
		0x8B, 0xF8,
		0x6A, 0x04,
		0x8D, 0x44, 0x24, 0x30,
		0x50,
		0x8B, 0xCE,
		0xE8, 0x76, 0x85, 0xE6, 0xFF,
		0x8B, 0x4C, 0x24, 0x2C,
		0x6A, 0x04,
		0x8D, 0x57, 0x04,
		0x89, 0x0F,
		0x52,
		0x8B, 0xCE,
		0xE8, 0x43, 0x85, 0xE6, 0xFF,
		0x83, 0xBE, 0x9C, 0x00, 0x00, 0x00, 0x00,
		0x74, 0x30,
		0x6A, 0x08,
		0xE8, 0xB3, 0xF7, 0xDF, 0xFF,
		0x83, 0xC4, 0x04,
		0x85, 0xC0,
		0x74, 0x11,
		0x8B, 0x8E, 0x9C, 0x00, 0x00, 0x00,
		0x89, 0x08,
		0xC7, 0x40, 0x04, 0x00, 0x00, 0x00, 0x00,
		0xEB, 0x02,
		0x33, 0xC0,
		0x8B, 0x96, 0xA0, 0x00, 0x00, 0x00,
		0x89, 0x50, 0x04,
		0x89, 0x86, 0xA0, 0x00, 0x00, 0x00,
		0x0F, 0xB7, 0x44, 0x24, 0x28,
		0x83, 0xC3, 0x01,
		0x3B, 0xD8,
		0x89, 0xBE, 0x9C, 0x00, 0x00, 0x00,
		0x72, 0x88
	};

	static const UInt8 kTESFormLoadDataFromCurrentSaveGameExpectedBytes[] =
	{
		0x8B, 0x0D, 0x00, 0x3B, 0xB3, 0x00,
		0xE9, 0x45, 0x88, 0xFE, 0xFF
	};

	static const UInt8 kTESFormLoadFormIDFromCurrentSaveGameExpectedBytes[] =
	{
		0x8B, 0x0D, 0x00, 0x3B, 0xB3, 0x00,
		0xE9, 0x55, 0x0D, 0xFF, 0xFF
	};

	static const UInt8 kGlobalAnimTimerUpdateExpectedBytes[] =
	{
		0xD9, 0x05, 0x30, 0x3A, 0xB3, 0x00,
		0x83, 0xC4, 0x08,
		0x85, 0xFF,
		0xD8, 0x44, 0x24, 0x14,
		0x51,
		0xD9, 0x1D, 0x30, 0x3A, 0xB3, 0x00,
		0xD9, 0x05, 0x30, 0x3A, 0xB3, 0x00,
		0xD9, 0x1C, 0x24,
		0x74
	};

	static const UInt8 kGlobalAnimTimerSceneUpdateExpectedBytes[] =
	{
		0xD9, 0x05, 0x30, 0x3A, 0xB3, 0x00,
		0x6A, 0x01,
		0x51,
		0x8B, 0x4E, 0x14,
		0xD9, 0x1C, 0x24,
		0xE8, 0xB4, 0x4D, 0x2C, 0x00
	};

	static const UInt8 kActorAnimClockPatchExpectedBytes[] =
	{
		0x8B, 0xCB,
		0x83, 0xE9, 0x05
	};

	static const UInt8 kActorAnimPreSamplePatchExpectedBytes[] =
	{
		0x33, 0xFF,
		0xEB, 0x06,
		0x8D, 0x9B, 0x00, 0x00, 0x00, 0x00
	};

	static const UInt8 kActorAnimRestoreClockWriteExpectedBytes[] =
	{
		0x66, 0x89, 0x6C, 0x5E, 0x3C,
		0x5F,
		0xD9, 0x9E, 0x94, 0x00, 0x00, 0x00
	};

	static const UInt8 kActorAnimClockWriteExpectedBytes[] =
	{
		0xD9, 0x86, 0x94, 0x00, 0x00, 0x00,
		0xD8, 0x45, 0x0C,
		0xD9, 0x9E, 0x94, 0x00, 0x00, 0x00
	};

	static const UInt8 kBSAnimGroupSequenceSampleUpdateExpectedBytes[] =
	{
		0x8B, 0x41, 0x44,
		0x83, 0xC0, 0xFF,
		0x83, 0xF8, 0x02,
		0x77, 0x1D,
		0xD9, 0x41, 0x48,
		0x6A, 0x01
	};

	static const UInt8 kBSAnimGroupSequenceSaveStateExpectedBytes[] =
	{
		0x56,
		0x8B, 0xF1,
		0xD9, 0x46, 0x48,
		0x8B, 0x0D, 0x00, 0x3B, 0xB3, 0x00,
		0xD8, 0x44, 0x24, 0x08
	};

	static const UInt8 kBSAnimGroupSequenceLoadStateExpectedBytes[] =
	{
		0x51,
		0x56,
		0x57,
		0x6A, 0x04,
		0x8D, 0x44, 0x24, 0x0C,
		0x8B, 0xF1,
		0x8B, 0x0D, 0x00, 0x3B, 0xB3
	};

	static const UInt8 kNiControllerSequenceUpdateExpectedBytes[] =
	{
		0x83, 0xEC, 0x0C,
		0x56,
		0x8B, 0xF1,
		0xD9, 0x46, 0x38,
		0xD9, 0x5C, 0x24, 0x08,
		0xD9, 0x46, 0x34
	};

	static const UInt8 kBinkOpenThreadHandleUseExpectedBytes[] =
	{
		0x74, 0x27,
		0x8B, 0x78, 0x10,
		0xFF, 0x15, 0x8C, 0x80, 0xA2, 0x00,
		0x3B, 0xC7,
		0x74, 0x1A,
		0x8B, 0x15, 0x98, 0x33, 0xB3, 0x00,
		0x8B, 0x42, 0x14,
		0x50,
		0x88, 0x5C, 0x24, 0x17,
		0xFF, 0x15, 0xF4, 0x80, 0xA2
	};

	static const UInt8 kBinkOpenResumeThreadUseExpectedBytes[] =
	{
		0x74, 0x0F,
		0xA1, 0x98, 0x33, 0xB3, 0x00,
		0x8B, 0x48, 0x14,
		0x51,
		0xFF, 0x15, 0xF0, 0x80, 0xA2
	};

	static const UInt8 kRendererInitFailureExpectedBytes[] =
	{
		0x83, 0xCD, 0xFF,
		0x33, 0xDB
	};

	static const UInt8 kActorGetAttackedExpectedBytes[] =
	{
		0x8B, 0x49, 0x58,
		0x8B, 0x01
	};

	static const UInt8 kActorAdjacentProcessGuardExpectedBytes[] =
	{
		0x8B, 0x49, 0x58,
		0x85, 0xC9,
		0x74, 0x0A,
		0x8B, 0x01,
		0x8B, 0x80, 0x9C, 0x03, 0x00, 0x00,
		0xFF, 0xE0
	};

	static const UInt8 kActorIsTalkingExpectedBytes[] =
	{
		0x8B, 0x48, 0x58,
		0x8B, 0x11
	};

	static const UInt8 kActorIsTalkingAdjacentGuardExpectedBytes[] =
	{
		0x83, 0x79, 0x58, 0x00,
		0x74, 0x0D,
		0x8B, 0x49, 0x58,
		0x8B, 0x01,
		0x8B, 0x90, 0x88, 0x03, 0x00, 0x00,
		0xFF, 0xE2,
		0x32, 0xC0,
		0xC3
	};

	struct OBSEInterface
	{
		UInt32 obseVersion;
		UInt32 oblivionVersion;
		UInt32 editorVersion;
		UInt32 isEditor;
		void* RegisterCommand;
		void* SetOpcodeBase;
		void* QueryInterface;
		void* GetPluginHandle;
	};

	struct PluginInfo
	{
		UInt32 infoVersion;
		const char* name;
		UInt32 version;
	};

	static bool GetModuleSiblingPath(const char* extension, char* path, UInt32 pathSize)
	{
		if (!path || !pathSize)
			return false;

		path[0] = 0;
		if (!GetModuleFileNameA(s_module, path, pathSize))
			return false;

		char* dot = std::strrchr(path, '.');
		if (dot)
			strcpy_s(dot, pathSize - (dot - path), extension);
		else
			strncat_s(path, pathSize, extension, _TRUNCATE);

		return true;
	}

	static void OpenLog()
	{
		if (s_log != INVALID_HANDLE_VALUE)
			return;

		char logPath[MAX_PATH] = { 0 };

		if (!GetModuleSiblingPath(".log", logPath, sizeof(logPath)))
		{
			strncpy_s(logPath, sizeof(logPath), "Modern Engine Fixes.log", _TRUNCATE);
		}

		s_log = CreateFileA(logPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}

	static void Log(const char* format, ...)
	{
		OpenLog();
		if (s_log == INVALID_HANDLE_VALUE)
			return;

		char buffer[1024];
		va_list args;
		va_start(args, format);
		int length = _vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
		va_end(args);

		if (length < 0)
			length = (int)std::strlen(buffer);

		DWORD written = 0;
		WriteFile(s_log, buffer, (DWORD)length, &written, NULL);
		WriteFile(s_log, "\r\n", 2, &written, NULL);
		FlushFileBuffers(s_log);
	}

	static bool BytesMatch(UInt32 address, const UInt8* expected, UInt32 length)
	{
		const UInt8* actual = (const UInt8*)address;
		for (UInt32 i = 0; i < length; i++)
		{
			if (actual[i] != expected[i])
				return false;
		}

		return true;
	}

	static bool CanReadMemory(UInt32 address, UInt32 length)
	{
		if (!length)
			return true;

		UInt32 end = address + length;
		if (end < address)
			return false;

		UInt32 current = address;
		while (current < end)
		{
			MEMORY_BASIC_INFORMATION info = { 0 };
			if (!VirtualQuery((const void*)current, &info, sizeof(info)))
				return false;

			if (info.State != MEM_COMMIT)
				return false;

			if (info.Protect & (PAGE_NOACCESS | PAGE_GUARD))
				return false;

			UInt32 regionEnd = (UInt32)info.BaseAddress + (UInt32)info.RegionSize;
			if (regionEnd <= current)
				return false;

			current = regionEnd < end ? regionEnd : end;
		}

		return true;
	}

	static void FormatBytes(const UInt8* bytes, UInt32 length, char* buffer, UInt32 bufferSize)
	{
		if (!bufferSize)
			return;

		buffer[0] = 0;
		UInt32 used = 0;
		for (UInt32 i = 0; i < length && used + 4 < bufferSize; i++)
		{
			int written = _snprintf_s(buffer + used,
				bufferSize - used,
				_TRUNCATE,
				i + 1 < length ? "%02X " : "%02X",
				bytes[i]);

			if (written < 0)
				break;

			used += (UInt32)written;
		}
	}

	static bool WriteRelCall(UInt32 source, UInt32 destination)
	{
		DWORD oldProtect = 0;
		if (!VirtualProtect((void*)source, 5, PAGE_EXECUTE_READWRITE, &oldProtect))
			return false;

		UInt8* code = (UInt8*)source;
		code[0] = 0xE8;
		*(SInt32*)(code + 1) = (SInt32)(destination - source - 5);

		DWORD ignored = 0;
		VirtualProtect((void*)source, 5, oldProtect, &ignored);
		FlushInstructionCache(GetCurrentProcess(), (void*)source, 5);
		return true;
	}

	static bool WriteNop(UInt32 address, UInt32 length)
	{
		if (!length)
			return true;

		DWORD oldProtect = 0;
		if (!VirtualProtect((void*)address, length, PAGE_EXECUTE_READWRITE, &oldProtect))
			return false;

		UInt8* code = (UInt8*)address;
		for (UInt32 i = 0; i < length; i++)
			code[i] = 0x90;

		DWORD ignored = 0;
		VirtualProtect((void*)address, length, oldProtect, &ignored);
		FlushInstructionCache(GetCurrentProcess(), (void*)address, length);
		return true;
	}

	static bool WriteRelJump(UInt32 source, UInt32 destination, UInt32 length)
	{
		if (length < 5)
			return false;

		DWORD oldProtect = 0;
		if (!VirtualProtect((void*)source, length, PAGE_EXECUTE_READWRITE, &oldProtect))
			return false;

		UInt8* code = (UInt8*)source;
		code[0] = 0xE9;
		*(SInt32*)(code + 1) = (SInt32)(destination - source - 5);

		for (UInt32 i = 5; i < length; i++)
			code[i] = 0x90;

		DWORD ignored = 0;
		VirtualProtect((void*)source, length, oldProtect, &ignored);
		FlushInstructionCache(GetCurrentProcess(), (void*)source, length);
		return true;
	}

	static bool ValidateDecodeBytes(const char* name, UInt32 address, const UInt8* expected, UInt32 length)
	{
		if (BytesMatch(address, expected, length))
			return true;

		char expectedText[512];
		char actualText[512];
		FormatBytes(expected, length, expectedText, sizeof(expectedText));
		FormatBytes((const UInt8*)address, length, actualText, sizeof(actualText));

		Log("refusing to patch: unexpected %s bytes at %08X; expected [%s], actual [%s]",
			name,
			address,
			expectedText,
			actualText);
		return false;
	}

	static void ClearSaveBufferRange(UInt32 owner, UInt32 base)
	{
		for (UInt32 i = 0; i < kSaveBufferRangeTrackingCapacity; i++)
		{
			if (s_saveBufferRanges[i].owner == owner &&
				(!base || s_saveBufferRanges[i].base == base))
			{
				s_saveBufferRanges[i].owner = 0;
				s_saveBufferRanges[i].base = 0;
				s_saveBufferRanges[i].end = 0;
				s_saveBufferRanges[i].size = 0;
			}
		}
	}

	static void TrackSaveBufferRange(UInt32 owner, UInt32 base, UInt32 size)
	{
		ClearSaveBufferRange(owner, 0);

		if (!owner || !base)
			return;

		UInt32 end = base + size;
		if (end < base)
		{
			s_saveBufferTrackingFailures++;
			Log("refusing wrapped save buffer range owner=%08X base=%08X size=%u",
				owner,
				base,
				size);
			return;
		}

		for (UInt32 i = 0; i < kSaveBufferRangeTrackingCapacity; i++)
		{
			if (!s_saveBufferRanges[i].owner)
			{
				s_saveBufferRanges[i].base = base;
				s_saveBufferRanges[i].end = end;
				s_saveBufferRanges[i].size = size;
				s_saveBufferRanges[i].owner = owner;
				s_trackedSaveBuffers++;
				return;
			}
		}

		s_saveBufferTrackingFailures++;
		Log("save buffer range tracking table full while tracking owner=%08X base=%08X size=%u",
			owner,
			base,
			size);
	}

	static SaveBufferRangeTrackingEntry* FindSaveBufferRange(UInt32 owner, UInt32 cursor)
	{
		for (UInt32 i = 0; i < kSaveBufferRangeTrackingCapacity; i++)
		{
			if (s_saveBufferRanges[i].owner == owner &&
				cursor >= s_saveBufferRanges[i].base &&
				cursor <= s_saveBufferRanges[i].end)
			{
				return &s_saveBufferRanges[i];
			}
		}

		return NULL;
	}

	static void* __cdecl CreateTrackedSaveBuffer(UInt8* owner, UInt32 size)
	{
		void* buffer = ((FormHeapAllocFn)kFormHeapAllocAddr)(size);
		if (owner)
			*(UInt32*)(owner + 0x14) = (UInt32)buffer;

		if (buffer)
		{
			TrackSaveBufferRange((UInt32)owner, (UInt32)buffer, size);
		}
		else
		{
			ClearSaveBufferRange((UInt32)owner, 0);
			((PrintErrorFn)kPrintErrorAddr)("Could not create save buffer, out of memory.");
		}

		return buffer;
	}

	static void __cdecl FreeTrackedSaveBuffer(UInt8* owner, void* buffer)
	{
		ClearSaveBufferRange((UInt32)owner, (UInt32)buffer);
		((FormHeapFreeFn)kFormHeapFreeAddr)(buffer);
		if (owner)
			*(UInt32*)(owner + 0x14) = 0;
	}

	static UInt32 GetSaveCursor(UInt8* owner)
	{
		return owner ? *(UInt32*)(owner + 0x14) : 0;
	}

	static void SetSaveCursor(UInt8* owner, UInt32 cursor)
	{
		if (owner)
			*(UInt32*)(owner + 0x14) = cursor;
	}

	static bool GetTrackedSaveBufferRemaining(UInt8* owner, UInt32* remainingOut)
	{
		if (!owner || !remainingOut)
			return false;

		UInt32 cursor = GetSaveCursor(owner);
		SaveBufferRangeTrackingEntry* range = FindSaveBufferRange((UInt32)owner, cursor);
		if (!range)
			return false;

		*remainingOut = range->end - cursor;
		return true;
	}

	static void DiscardRawSaveBlobPayload(UInt8* owner, UInt32 length, const char* label)
	{
		if (!owner || !length)
			return;

		UInt32 cursor = GetSaveCursor(owner);
		if (!cursor)
			return;

		UInt32 bytesToAdvance = length;
		SaveBufferRangeTrackingEntry* range = FindSaveBufferRange((UInt32)owner, cursor);
		if (range)
		{
			UInt32 remaining = range->end - cursor;
			if (bytesToAdvance > remaining)
			{
				bytesToAdvance = remaining;
				s_clampedSaveRawBlobCopies++;
				if (s_clampedSaveRawBlobCopies <= 16)
				{
					Log("discarded truncated %s raw save blob from %u to %u bytes at cursor=%08X range=[%08X,%08X)",
						label,
						length,
						bytesToAdvance,
						cursor,
						range->base,
						range->end);
				}
			}
		}
		else
		{
			s_untrackedSaveRawBlobCopies++;
			if (s_untrackedSaveRawBlobCopies <= 16)
			{
				Log("discarding %s raw save blob without tracked range owner=%08X cursor=%08X length=%u",
					label,
					(UInt32)owner,
					cursor,
					length);
			}
		}

		UInt32 next = cursor + bytesToAdvance;
		if (next < cursor)
			next = cursor;

		SetSaveCursor(owner, next);
	}

	static void ClampUInt16SaveCount(UInt16* count, UInt32 minEntryBytes, const char* label)
	{
		if (!count || !minEntryBytes)
			return;

		UInt8* owner = *(UInt8**)0x00B33B00;
		UInt32 remaining = 0;
		if (!GetTrackedSaveBufferRemaining(owner, &remaining))
		{
			s_untrackedSaveRawBlobCopies++;
			if (s_untrackedSaveRawBlobCopies <= 16)
			{
				Log("leaving %s save count unclamped because no tracked range exists owner=%08X",
					label,
					(UInt32)owner);
			}
			return;
		}

		UInt32 maxCount = remaining / minEntryBytes;
		if (*count > maxCount)
		{
			UInt32 original = *count;
			*count = (UInt16)maxCount;
			s_clampedSaveCountLoops++;
			if (s_clampedSaveCountLoops <= 16)
			{
				Log("clamped %s save count from %u to %u using remaining=%u entryBytes=%u",
					label,
					original,
					maxCount,
					remaining,
					minEntryBytes);
			}
		}
	}

	static void __cdecl ClampTESSpellListSaveCount(UInt16* count)
	{
		ClampUInt16SaveCount(count, 4, "TESSpellList");
	}

	static void __cdecl ClampAVCollectionSaveCount(UInt16* count)
	{
		UInt8* owner = *(UInt8**)0x00B33B00;
		UInt32 minEntryBytes = owner && owner[0x7C] >= 0x34 ? 5 : 8;
		ClampUInt16SaveCount(count, minEntryBytes, "AVCollection");
	}

	static void __cdecl ClampMiddleHighProcessFormIDSaveCount(UInt16* count)
	{
		ClampUInt16SaveCount(count, 4, "MiddleHighProcess FormID list");
	}

	static void __cdecl ClampKnownEffectsSaveCount(SInt32* count)
	{
		if (!count || *count <= 0)
			return;

		UInt8* owner = *(UInt8**)0x00B33B00;
		UInt32 remaining = 0;
		if (!GetTrackedSaveBufferRemaining(owner, &remaining))
		{
			s_untrackedSaveRawBlobCopies++;
			if (s_untrackedSaveRawBlobCopies <= 16)
			{
				Log("leaving KnownEffects save count unclamped because no tracked range exists owner=%08X",
					(UInt32)owner);
			}
			return;
		}

		UInt32 maxCount = remaining / 4;
		if ((UInt32)*count > maxCount)
		{
			SInt32 original = *count;
			*count = (SInt32)maxCount;
			s_clampedSaveCountLoops++;
			if (s_clampedSaveCountLoops <= 16)
			{
				Log("clamped KnownEffects save count from %d to %u using remaining=%u entryBytes=4",
					original,
					maxCount,
					remaining);
			}
		}
	}

	static void __cdecl LoadActorRelationshipDispositionLoop(
		UInt8* actor,
		UInt16* count,
		UInt32 currentEntryOffset,
		UInt32 historyEntryOffset,
		UInt32 layout)
	{
		if (!actor || !count)
			return;

		const char* label = layout == kActorRelationshipValueThenForm ?
			"Actor relationship/disposition first list" :
			"Actor relationship/disposition second list";
		ClampUInt16SaveCount(count, 8, label);

		const UInt32 entryCount = *count;
		TESFormLoadCurrentSaveGameFn loadFormID =
			(TESFormLoadCurrentSaveGameFn)kTESFormLoadFormIDFromCurrentSaveGameAddr;
		TESFormLoadCurrentSaveGameFn loadData =
			(TESFormLoadCurrentSaveGameFn)kTESFormLoadDataFromCurrentSaveGameAddr;
		FormHeapAllocFn alloc = (FormHeapAllocFn)kFormHeapAllocAddr;
		FormHeapFreeFn freeEntry = (FormHeapFreeFn)kFormHeapFreeAddr;

		UInt32* currentField = (UInt32*)(actor + currentEntryOffset);
		UInt32* historyField = (UInt32*)(actor + historyEntryOffset);

		for (UInt32 i = 0; i < entryCount; i++)
		{
			UInt32 formID = 0;
			UInt32 value = 0;
			loadFormID(actor, &formID, sizeof(formID));
			loadData(actor, &value, sizeof(value));

			UInt32 oldCurrent = *currentField;
			UInt32 oldHistory = *historyField;
			UInt32* entry = (UInt32*)alloc(8);
			if (!entry)
			{
				++s_skippedActorRelationshipDispositionEntries;
				if (s_skippedActorRelationshipDispositionEntries <= 16)
				{
					Log("skipped %s entry after allocation failure actor=%08X index=%u count=%u",
						label,
						(UInt32)actor,
						i,
						entryCount);
				}
				continue;
			}

			UInt32* history = NULL;
			if (oldCurrent)
			{
				history = (UInt32*)alloc(8);
				if (!history)
				{
					freeEntry(entry);
					++s_skippedActorRelationshipDispositionHistoryNodes;
					if (s_skippedActorRelationshipDispositionHistoryNodes <= 16)
					{
						Log("skipped %s entry after history allocation failure actor=%08X index=%u count=%u",
							label,
							(UInt32)actor,
							i,
							entryCount);
					}
					continue;
				}
			}

			if (layout == kActorRelationshipValueThenForm)
			{
				entry[0] = value;
				entry[1] = formID;
			}
			else
			{
				entry[0] = formID;
				entry[1] = value;
			}

			if (history)
			{
				history[0] = oldCurrent;
				history[1] = oldHistory;
				*historyField = (UInt32)history;
			}

			*currentField = (UInt32)entry;
		}
	}

	static int __cdecl StoreBoundedRawSaveBlob(UInt8* owner,
		UInt32 keyObject,
		UInt32 length,
		UInt32 mapOffset,
		UInt32 zeroKeyWhenExisting,
		const char* label)
	{
		if (!owner)
			return 0;

		if (!keyObject)
		{
			DiscardRawSaveBlobPayload(owner, length, label);
			return 0;
		}

		void* map = *(void**)(owner + mapOffset);
		if (!map)
		{
			DiscardRawSaveBlobPayload(owner, length, label);
			return 0;
		}

		UInt32 key = *(UInt32*)(keyObject + 0x0C);
		if (zeroKeyWhenExisting)
		{
			UInt32 existing = 0;
			if (((NiTMapGetAtFn)kNiTMapGetAtAddr)(map, key, &existing))
				key = 0;
		}

		UInt32 cursor = GetSaveCursor(owner);
		UInt32 bytesToCopy = length;
		UInt32 bytesToAdvance = length;
		SaveBufferRangeTrackingEntry* range = FindSaveBufferRange((UInt32)owner, cursor);
		if (range)
		{
			UInt32 remaining = range->end - cursor;
			if (bytesToCopy > remaining)
			{
				bytesToCopy = remaining;
				bytesToAdvance = remaining;
				s_clampedSaveRawBlobCopies++;
				if (s_clampedSaveRawBlobCopies <= 16)
				{
					Log("clamped %s raw save blob from %u to %u bytes at cursor=%08X range=[%08X,%08X)",
						label,
						length,
						bytesToCopy,
						cursor,
						range->base,
						range->end);
				}
			}
		}
		else
		{
			s_untrackedSaveRawBlobCopies++;
			if (s_untrackedSaveRawBlobCopies <= 16)
			{
				Log("copying %s raw save blob without tracked range owner=%08X cursor=%08X length=%u",
					label,
					(UInt32)owner,
					cursor,
					length);
			}
		}

		if (bytesToCopy && !CanReadMemory(cursor, bytesToCopy))
		{
			if (range)
				bytesToAdvance = range->end - cursor;
			bytesToCopy = 0;
			s_clampedSaveRawBlobCopies++;
			if (s_clampedSaveRawBlobCopies <= 16)
			{
				Log("zero-filled %s raw save blob because cursor is unreadable owner=%08X cursor=%08X length=%u",
					label,
					(UInt32)owner,
					cursor,
					length);
			}
		}

		UInt32 allocationSize = length + 2;
		UInt8* blob = (UInt8*)((FormHeapAllocFn)kFormHeapAllocAddr)(allocationSize);
		if (!blob)
		{
			SetSaveCursor(owner, cursor + bytesToAdvance);
			s_failedSaveRawBlobAllocations++;
			if (s_failedSaveRawBlobAllocations <= 16)
			{
				Log("raw save blob allocation failed for %s length=%u; skipped map insert",
					label,
					length);
			}
			return 0;
		}

		*(UInt16*)blob = (UInt16)length;
		if (bytesToCopy)
			std::memcpy(blob + 2, (const void*)cursor, bytesToCopy);
		if (bytesToCopy < length)
			std::memset(blob + 2 + bytesToCopy, 0, length - bytesToCopy);

		SetSaveCursor(owner, cursor + bytesToAdvance);
		return ((NiTMapSetAtFn)kNiTMapSetAtAddr)(map, key, blob);
	}

	static int __cdecl StoreRawSaveBlobMap54(UInt8* owner, UInt32 keyObject, UInt32 length)
	{
		return StoreBoundedRawSaveBlob(owner, keyObject, length, 0x54, 1, "SaveLoad +0x54");
	}

	static int __cdecl StoreRawSaveBlobMap58(UInt8* owner, UInt32 keyObject, UInt32 length)
	{
		return StoreBoundedRawSaveBlob(owner, keyObject, length, 0x58, 0, "SaveLoad +0x58");
	}

	static int __cdecl StoreRawSaveBlobMap5C(UInt8* owner, UInt32 keyObject, UInt32 length)
	{
		return StoreBoundedRawSaveBlob(owner, keyObject, length, 0x5C, 0, "SaveLoad +0x5C");
	}

	static int __cdecl StoreRawSaveBlobMap60(UInt8* owner, UInt32 keyObject, UInt32 length)
	{
		return StoreBoundedRawSaveBlob(owner, keyObject, length, 0x60, 0, "SaveLoad +0x60");
	}

	static __declspec(naked) void SaveLoadCreateBufferPatch()
	{
		__asm
		{
			push	dword ptr [esp + 4]
			push	ecx
			call	CreateTrackedSaveBuffer
			add		esp, 8
			ret		4
		}
	}

	static __declspec(naked) void SaveLoadFreeBufferPatch()
	{
		__asm
		{
			push	dword ptr [esp + 4]
			push	ecx
			call	FreeTrackedSaveBuffer
			add		esp, 8
			ret		4
		}
	}

	static __declspec(naked) void SaveLoadRawBlobMap54Patch()
	{
		__asm
		{
			movzx	eax, word ptr [esp + 8]
			push	eax
			push	dword ptr [esp + 8]
			push	ecx
			call	StoreRawSaveBlobMap54
			add		esp, 0Ch
			ret		8
		}
	}

	static __declspec(naked) void SaveLoadRawBlobMap58Patch()
	{
		__asm
		{
			movzx	eax, word ptr [esp + 8]
			push	eax
			push	dword ptr [esp + 8]
			push	ecx
			call	StoreRawSaveBlobMap58
			add		esp, 0Ch
			ret		8
		}
	}

	static __declspec(naked) void SaveLoadRawBlobMap5CPatch()
	{
		__asm
		{
			movzx	eax, word ptr [esp + 8]
			push	eax
			push	dword ptr [esp + 8]
			push	ecx
			call	StoreRawSaveBlobMap5C
			add		esp, 0Ch
			ret		8
		}
	}

	static __declspec(naked) void SaveLoadRawBlobMap60Patch()
	{
		__asm
		{
			movzx	eax, word ptr [esp + 8]
			push	eax
			push	dword ptr [esp + 8]
			push	ecx
			call	StoreRawSaveBlobMap60
			add		esp, 0Ch
			ret		8
		}
	}

	static __declspec(naked) void TESSpellListCountPatch()
	{
		__asm
		{
			lea		eax, [esp + 24h]
			push	eax
			call	ClampTESSpellListSaveCount
			add		esp, 4
			mov		dword ptr [esp + 18h], 0
			cmp		word ptr [esp + 24h], 0
			jbe		done
			mov		eax, 0046F9E0h
			jmp		eax

		done:
			mov		eax, 0046FACFh
			jmp		eax
		}
	}

	static __declspec(naked) void TESSpellListNodeAllocPatch()
	{
		__asm
		{
			test	eax, eax
			jz		failed
			mov		edx, [edi]
			mov		ecx, 0046FA55h
			jmp		ecx

		failed:
			inc		s_skippedTESSpellListNodeAllocations
			mov		ecx, 0046FAB7h
			jmp		ecx
		}
	}

	static __declspec(naked) void AVCollectionCountPatch()
	{
		__asm
		{
			lea		eax, [esp + 0Ch]
			push	eax
			call	ClampAVCollectionSaveCount
			add		esp, 4
			xor		edi, edi
			cmp		word ptr [esp + 0Ch], di
			jbe		done
			mov		eax, 0065CBD6h
			jmp		eax

		done:
			mov		eax, 0065CC83h
			jmp		eax
		}
	}

	static __declspec(naked) void MiddleHighProcessFormIDCountPatch()
	{
		__asm
		{
			lea		eax, [esp + 64h]
			push	eax
			call	ClampMiddleHighProcessFormIDSaveCount
			add		esp, 4
			xor		ebx, ebx
			cmp		word ptr [esp + 64h], bx
			jbe		done
			mov		eax, 00656AB0h
			jmp		eax

		done:
			mov		eax, 00656B1Fh
			jmp		eax
		}
	}

	static __declspec(naked) void MiddleHighProcessFormIDNodeAllocPatch()
	{
		__asm
		{
			test	eax, eax
			jz		failed
			mov		ecx, [esi + 0A8h]
			mov		edx, 00656AE9h
			jmp		edx

		failed:
			inc		s_skippedMiddleHighProcessFormIDNodes
			mov		edx, 00656B13h
			jmp		edx
		}
	}

	static __declspec(naked) void KnownEffectsCountPatch()
	{
		__asm
		{
			lea		eax, [esp + 14h]
			push	eax
			call	ClampKnownEffectsSaveCount
			add		esp, 4
			xor		esi, esi
			cmp		dword ptr [esp + 14h], ebp
			jle		done
			mov		edi, 00200000h
			mov		eax, 00416EC0h
			jmp		eax

		done:
			mov		eax, 00416F12h
			jmp		eax
		}
	}

	static __declspec(naked) void ActorRelationshipDispositionFirstLoopPatch()
	{
		__asm
		{
			lea		eax, [esp + 10h]
			push	0
			push	0A8h
			push	0A4h
			push	eax
			push	esi
			call	LoadActorRelationshipDispositionLoop
			add		esp, 14h
			mov		eax, 006026A2h
			jmp		eax
		}
	}

	static __declspec(naked) void ActorRelationshipDispositionSecondLoopPatch()
	{
		__asm
		{
			lea		eax, [esp + 28h]
			push	1
			push	0A0h
			push	09Ch
			push	eax
			push	esi
			call	LoadActorRelationshipDispositionLoop
			add		esp, 14h
			mov		eax, 00602788h
			jmp		eax
		}
	}

	static TexturePalettePatchState GetTexturePalettePatchState()
	{
		if (BytesMatch(kTexturePaletteUnderscorePatchSite,
				kTexturePaletteExpectedBytes,
				sizeof(kTexturePaletteExpectedBytes)))
		{
			return kTexturePalettePatchNeedsInstall;
		}

		const UInt8* site = (const UInt8*)kTexturePaletteUnderscorePatchSite;
		if (site[0] == 0xE9)
		{
			UInt32 target = kTexturePaletteUnderscorePatchSite + 5 + *(const SInt32*)(site + 1);
			if (CanReadMemory(target, sizeof(kTexturePaletteGuardThunkExpectedBytes)) &&
				BytesMatch(target,
					kTexturePaletteGuardThunkExpectedBytes,
					sizeof(kTexturePaletteGuardThunkExpectedBytes)))
			{
				Log("BSTexturePalette strrchr null-result guard already present at %08X via jump target %08X",
					kTexturePaletteUnderscorePatchSite,
					target);
				return kTexturePalettePatchAlreadyGuarded;
			}
		}

		char expectedText[128];
		char actualText[128];
		FormatBytes(kTexturePaletteExpectedBytes,
			sizeof(kTexturePaletteExpectedBytes),
			expectedText,
			sizeof(expectedText));
		FormatBytes(site,
			sizeof(kTexturePaletteExpectedBytes),
			actualText,
			sizeof(actualText));

		Log("refusing to patch: unexpected BSTexturePalette strrchr guard bytes at %08X; expected [%s], actual [%s]",
			kTexturePaletteUnderscorePatchSite,
			expectedText,
			actualText);
		return kTexturePalettePatchMismatch;
	}

	static bool IsEngineBugFixesGlobalAnimTimerResetHelper(UInt32 helper)
	{
		if (!CanReadMemory(helper, 0xB1))
			return false;

		const UInt8* code = (const UInt8*)helper;
		UInt32 globalTimerPointer = 0;

		for (UInt32 i = 0; i + 7 <= 0x50; i++)
		{
			if (code[i] == 0xA1 && code[i + 5] == 0xD9 && code[i + 6] == 0x00)
			{
				UInt32 pointer = *(const UInt32*)(code + i + 1);
				if (CanReadMemory(pointer, 4) && *(const UInt32*)pointer == kGlobalAnimTimerAddr)
				{
					globalTimerPointer = pointer;
					break;
				}
			}
		}

		if (!globalTimerPointer)
			return false;

		for (UInt32 i = 0; i + 20 <= 0xB1; i++)
		{
			if (code[i] == 0x81 &&
				code[i + 1] == 0xFE &&
				*(const UInt32*)(code + i + 2) == 0x48000000 &&
				code[i + 6] == 0x72 &&
				code[i + 8] == 0xD9 &&
				code[i + 9] == 0xEE &&
				code[i + 10] == 0x8B &&
				code[i + 11] == 0x0D &&
				*(const UInt32*)(code + i + 12) == globalTimerPointer)
			{
				for (UInt32 j = i + 16; j + 1 < 0xB1 && j < i + 36; j++)
				{
					if (code[j] == 0xD9 && code[j + 1] == 0x19)
						return true;
				}
			}
		}

		return false;
	}

	static GlobalAnimTimerLoadPatchState GetGlobalAnimTimerLoadPatchState()
	{
		if (BytesMatch(kGlobalAnimTimerLoadPatchSite,
				kGlobalAnimTimerLoadExpectedBytes,
				sizeof(kGlobalAnimTimerLoadExpectedBytes)))
		{
			return kGlobalAnimTimerLoadPatchNeedsInstall;
		}

		const UInt8* site = (const UInt8*)kGlobalAnimTimerLoadPatchSite;
		if (site[0] == 0xE9)
		{
			UInt32 target = kGlobalAnimTimerLoadPatchSite + 5 + *(const SInt32*)(site + 1);
			if (CanReadMemory(target, 17))
			{
				const UInt8* hook = (const UInt8*)target;
				if (hook[0] == 0xFF &&
					hook[1] == 0x15 &&
					hook[6] == 0xE8 &&
					hook[11] == 0xFF &&
					hook[12] == 0x25)
				{
					UInt32 loadPointer = *(const UInt32*)(hook + 2);
					UInt32 resumePointer = *(const UInt32*)(hook + 13);
					UInt32 resetHelper = target + 11 + *(const SInt32*)(hook + 7);

					if (CanReadMemory(loadPointer, 4) &&
						CanReadMemory(resumePointer, 4) &&
						*(const UInt32*)loadPointer == kSaveLoadLoadDataAddr &&
						*(const UInt32*)resumePointer == kGlobalAnimTimerLoadContinueSite &&
						IsEngineBugFixesGlobalAnimTimerResetHelper(resetHelper))
					{
						Log("global animation timer load precision guard already present at %08X via jump target %08X",
							kGlobalAnimTimerLoadPatchSite,
							target);
						return kGlobalAnimTimerLoadPatchAlreadyGuarded;
					}
				}
			}
		}

		char expectedText[128];
		char actualText[128];
		FormatBytes(kGlobalAnimTimerLoadExpectedBytes,
			sizeof(kGlobalAnimTimerLoadExpectedBytes),
			expectedText,
			sizeof(expectedText));
		FormatBytes(site,
			sizeof(kGlobalAnimTimerLoadExpectedBytes),
			actualText,
			sizeof(actualText));

		Log("refusing to patch: unexpected global animation timer load bytes at %08X; expected [%s], actual [%s]",
			kGlobalAnimTimerLoadPatchSite,
			expectedText,
			actualText);
		return kGlobalAnimTimerLoadPatchMismatch;
	}

	static bool InlineTransferTargets(UInt32 base, const UInt8* code, UInt32 length, UInt32 target)
	{
		for (UInt32 i = 0; i < length; i++)
		{
			if (i + 6 <= length && code[i] == 0xFF && code[i + 1] == 0x25)
			{
				UInt32 pointer = *(const UInt32*)(code + i + 2);
				if (CanReadMemory(pointer, 4) && *(const UInt32*)pointer == target)
					return true;
			}

			if (i + 6 <= length && code[i] == 0x68 && code[i + 5] == 0xC3 &&
				*(const UInt32*)(code + i + 1) == target)
			{
				return true;
			}

			if (i + 5 <= length && code[i] == 0xE9)
			{
				UInt32 destination = base + i + 5 + *(const SInt32*)(code + i + 1);
				if (destination == target)
					return true;
			}
		}

		return false;
	}

	static bool IsExistingRendererInitFailureHook(UInt32 hook)
	{
		static const UInt32 kScanLength = 32;

		if (!CanReadMemory(hook, kScanLength))
			return false;

		const UInt8* code = (const UInt8*)hook;
		if (code[0] != 0x84 || code[1] != 0xC0 || code[2] != 0x75)
			return false;

		bool hasOriginalResumeBytes = false;
		for (UInt32 i = 0; i + sizeof(kRendererInitFailureExpectedBytes) <= kScanLength; i++)
		{
			if (BytesMatch(hook + i,
					kRendererInitFailureExpectedBytes,
					sizeof(kRendererInitFailureExpectedBytes)))
			{
				hasOriginalResumeBytes = true;
				break;
			}
		}

		return hasOriginalResumeBytes &&
			InlineTransferTargets(hook, code, kScanLength, kRendererInitFailureReturnSite) &&
			InlineTransferTargets(hook, code, kScanLength, kRendererInitFailureContinueSite);
	}

	static RendererInitFailurePatchState GetRendererInitFailurePatchState()
	{
		if (BytesMatch(kRendererInitFailurePatchSite,
				kRendererInitFailureExpectedBytes,
				sizeof(kRendererInitFailureExpectedBytes)))
		{
			return kRendererInitFailurePatchNeedsInstall;
		}

		const UInt8* site = (const UInt8*)kRendererInitFailurePatchSite;
		if (site[0] == 0xE9)
		{
			UInt32 target = kRendererInitFailurePatchSite + 5 + *(const SInt32*)(site + 1);
			if (IsExistingRendererInitFailureHook(target))
			{
				Log("renderer initialization failure guard already present at %08X via jump target %08X",
					kRendererInitFailurePatchSite,
					target);
				return kRendererInitFailurePatchAlreadyGuarded;
			}
		}

		char expectedText[128];
		char actualText[128];
		FormatBytes(kRendererInitFailureExpectedBytes,
			sizeof(kRendererInitFailureExpectedBytes),
			expectedText,
			sizeof(expectedText));
		FormatBytes(site,
			sizeof(kRendererInitFailureExpectedBytes),
			actualText,
			sizeof(actualText));

		Log("refusing to patch: unexpected renderer initialization failure guard bytes at %08X; expected [%s], actual [%s]",
			kRendererInitFailurePatchSite,
			expectedText,
			actualText);
		return kRendererInitFailurePatchMismatch;
	}

	static bool IsExistingActorGetAttackedProcessGuard(UInt32 hook)
	{
		static const UInt32 kScanLength = 96;

		if (!CanReadMemory(hook, kScanLength))
			return false;

		const UInt8* code = (const UInt8*)hook;
		bool loadsProcessAndChecksNull = false;
		for (UInt32 i = 0; i + 6 <= kScanLength; i++)
		{
			if (((code[i] == 0x8B && code[i + 1] == 0x41 && code[i + 2] == 0x58 &&
						code[i + 3] == 0x85 && code[i + 4] == 0xC0) ||
					(code[i] == 0x8B && code[i + 1] == 0x49 && code[i + 2] == 0x58 &&
						code[i + 3] == 0x85 && code[i + 4] == 0xC9)) &&
				code[i + 5] == 0x74)
			{
				loadsProcessAndChecksNull = true;
				break;
			}
		}

		bool hasOriginalCallSetup = false;
		for (UInt32 i = 0; i + 10 <= kScanLength; i++)
		{
			if (code[i] == 0x8B &&
				code[i + 1] == 0xC8 &&
				code[i + 2] == 0x8B &&
				code[i + 3] == 0x00 &&
				code[i + 4] == 0x8B &&
				code[i + 5] == 0x90 &&
				*(const UInt32*)(code + i + 6) == 0x00000398)
			{
				hasOriginalCallSetup = true;
				break;
			}
		}

		bool returnsFalseOnNull = false;
		for (UInt32 i = 0; i + 3 <= kScanLength; i++)
		{
			if ((code[i] == 0x33 && code[i + 1] == 0xC0 && code[i + 2] == 0xC3) ||
				(code[i] == 0x32 && code[i + 1] == 0xC0 && code[i + 2] == 0xC3))
			{
				returnsFalseOnNull = true;
				break;
			}
		}

		return loadsProcessAndChecksNull &&
			hasOriginalCallSetup &&
			returnsFalseOnNull &&
			InlineTransferTargets(hook, code, kScanLength, kActorGetAttackedContinueSite);
	}

	static ActorGetAttackedPatchState GetActorGetAttackedPatchState()
	{
		if (BytesMatch(kActorGetAttackedPatchSite,
				kActorGetAttackedExpectedBytes,
				sizeof(kActorGetAttackedExpectedBytes)))
		{
			return kActorGetAttackedPatchNeedsInstall;
		}

		const UInt8* site = (const UInt8*)kActorGetAttackedPatchSite;
		if (site[0] == 0xE9)
		{
			UInt32 target = kActorGetAttackedPatchSite + 5 + *(const SInt32*)(site + 1);
			if (IsExistingActorGetAttackedProcessGuard(target))
			{
				Log("Actor::GetAttacked process null guard already present at %08X via jump target %08X",
					kActorGetAttackedPatchSite,
					target);
				return kActorGetAttackedPatchAlreadyGuarded;
			}
		}

		char expectedText[128];
		char actualText[128];
		FormatBytes(kActorGetAttackedExpectedBytes,
			sizeof(kActorGetAttackedExpectedBytes),
			expectedText,
			sizeof(expectedText));
		FormatBytes(site,
			sizeof(kActorGetAttackedExpectedBytes),
			actualText,
			sizeof(actualText));

		Log("refusing to patch: unexpected Actor::GetAttacked process guard bytes at %08X; expected [%s], actual [%s]",
			kActorGetAttackedPatchSite,
			expectedText,
			actualText);
		return kActorGetAttackedPatchMismatch;
	}

	static bool IsExistingActorIsTalkingProcessGuard(UInt32 hook)
	{
		static const UInt32 kScanLength = 96;

		if (!CanReadMemory(hook, kScanLength))
			return false;

		const UInt8* code = (const UInt8*)hook;
		bool loadsProcessAndChecksNull = false;
		for (UInt32 i = 0; i + 6 <= kScanLength; i++)
		{
			if (code[i] == 0x8B &&
				code[i + 1] == 0x48 &&
				code[i + 2] == 0x58 &&
				code[i + 3] == 0x85 &&
				code[i + 4] == 0xC9 &&
				code[i + 5] == 0x74)
			{
				loadsProcessAndChecksNull = true;
				break;
			}
		}

		bool hasOriginalCallSetup = false;
		for (UInt32 i = 0; i + 2 <= kScanLength; i++)
		{
			if (code[i] == 0x8B && code[i + 1] == 0x11)
			{
				hasOriginalCallSetup = true;
				break;
			}
		}

		bool returnsFalseOnNull = false;
		for (UInt32 i = 0; i + 2 <= kScanLength; i++)
		{
			if ((code[i] == 0x33 && code[i + 1] == 0xC0) ||
				(code[i] == 0x32 && code[i + 1] == 0xC0))
			{
				returnsFalseOnNull = true;
				break;
			}
		}

		return loadsProcessAndChecksNull &&
			hasOriginalCallSetup &&
			returnsFalseOnNull &&
			InlineTransferTargets(hook, code, kScanLength, kActorIsTalkingContinueSite) &&
			InlineTransferTargets(hook, code, kScanLength, kActorIsTalkingReturnSite);
	}

	static bool IsExistingActorAnimRebaseHook(UInt32 hook, bool postClockLoop)
	{
		static const UInt32 kScanLength = 16;

		for (UInt32 i = 0; i < 4; i++)
		{
			if (!CanReadMemory(hook, kScanLength))
				return false;

			const UInt8* code = (const UInt8*)hook;
			if (code[0] == 0x60 &&
				code[1] == 0x56 &&
				code[2] == 0xE8 &&
				code[7] == 0x83 &&
				code[8] == 0xC4 &&
				code[9] == 0x04 &&
				code[10] == 0x61)
			{
				if (!postClockLoop &&
					code[11] == 0x33 &&
					code[12] == 0xFF &&
					code[13] == 0xC3)
				{
					return true;
				}

				if (postClockLoop &&
					code[11] == 0x8B &&
					code[12] == 0xCB &&
					code[13] == 0x83 &&
					code[14] == 0xE9 &&
					code[15] == 0x05)
				{
					return true;
				}
			}

			if (code[0] == 0xE8 || code[0] == 0xE9)
			{
				UInt32 redirected = hook + 5 + *(const SInt32*)(code + 1);
				if (redirected == hook)
					return false;
				hook = redirected;
				continue;
			}

			return false;
		}

		return false;
	}

	static ActorAnimPreSamplePatchState GetActorAnimPreSamplePatchState()
	{
		if (BytesMatch(kActorAnimUpdatePreSamplePatchSite,
				kActorAnimPreSamplePatchExpectedBytes,
				sizeof(kActorAnimPreSamplePatchExpectedBytes)))
		{
			return kActorAnimPreSamplePatchNeedsInstall;
		}

		const UInt8* site = (const UInt8*)kActorAnimUpdatePreSamplePatchSite;
		if (site[0] == 0xE8)
		{
			UInt32 target = kActorAnimUpdatePreSamplePatchSite + 5 + *(const SInt32*)(site + 1);
			if (IsExistingActorAnimRebaseHook(target, false))
			{
				Log("ActorAnimData pre-sample loop already present at %08X via call target %08X",
					kActorAnimUpdatePreSamplePatchSite,
					target);
				return kActorAnimPreSamplePatchAlreadyInstalled;
			}

			if (site[5] == 0x90 &&
				site[6] == 0x90 &&
				site[7] == 0x90 &&
				site[8] == 0x90 &&
				site[9] == 0x90)
			{
				Log("ActorAnimData pre-sample loop already present at %08X via hook call at %08X",
					kActorAnimUpdatePreSamplePatchSite,
					target);
				return kActorAnimPreSamplePatchAlreadyInstalled;
			}
		}

		char expectedText[128];
		char actualText[128];
		FormatBytes(kActorAnimPreSamplePatchExpectedBytes,
			sizeof(kActorAnimPreSamplePatchExpectedBytes),
			expectedText,
			sizeof(expectedText));
		FormatBytes(site,
			sizeof(kActorAnimPreSamplePatchExpectedBytes),
			actualText,
			sizeof(actualText));

		Log("refusing to patch: unexpected ActorAnimData pre-sample loop bytes at %08X; expected [%s], actual [%s]",
			kActorAnimUpdatePreSamplePatchSite,
			expectedText,
			actualText);
		return kActorAnimPreSamplePatchMismatch;
	}

	static ActorAnimClockPatchState GetActorAnimClockPatchState()
	{
		if (BytesMatch(kActorAnimUpdateClockPatchSite,
				kActorAnimClockPatchExpectedBytes,
				sizeof(kActorAnimClockPatchExpectedBytes)))
		{
			return kActorAnimClockPatchNeedsInstall;
		}

		const UInt8* site = (const UInt8*)kActorAnimUpdateClockPatchSite;
		if (site[0] == 0xE8)
		{
			UInt32 target = kActorAnimUpdateClockPatchSite + 5 + *(const SInt32*)(site + 1);
			if (IsExistingActorAnimRebaseHook(target, true))
			{
				Log("ActorAnimData post-clock loop already present at %08X via call target %08X",
					kActorAnimUpdateClockPatchSite,
					target);
				return kActorAnimClockPatchAlreadyInstalled;
			}
		}

		char expectedText[128];
		char actualText[128];
		FormatBytes(kActorAnimClockPatchExpectedBytes,
			sizeof(kActorAnimClockPatchExpectedBytes),
			expectedText,
			sizeof(expectedText));
		FormatBytes(site,
			sizeof(kActorAnimClockPatchExpectedBytes),
			actualText,
			sizeof(actualText));

		Log("refusing to patch: unexpected ActorAnimData post-clock loop bytes at %08X; expected [%s], actual [%s]",
			kActorAnimUpdateClockPatchSite,
			expectedText,
			actualText);
		return kActorAnimClockPatchMismatch;
	}

	static ActorIsTalkingPatchState GetActorIsTalkingPatchState()
	{
		if (BytesMatch(kActorIsTalkingPatchSite,
				kActorIsTalkingExpectedBytes,
				sizeof(kActorIsTalkingExpectedBytes)))
		{
			return kActorIsTalkingPatchNeedsInstall;
		}

		const UInt8* site = (const UInt8*)kActorIsTalkingPatchSite;
		if (site[0] == 0xE9)
		{
			UInt32 target = kActorIsTalkingPatchSite + 5 + *(const SInt32*)(site + 1);
			if (IsExistingActorIsTalkingProcessGuard(target))
			{
				Log("Actor::IsTalking process null guard already present at %08X via jump target %08X",
					kActorIsTalkingPatchSite,
					target);
				return kActorIsTalkingPatchAlreadyGuarded;
			}
		}

		char expectedText[128];
		char actualText[128];
		FormatBytes(kActorIsTalkingExpectedBytes,
			sizeof(kActorIsTalkingExpectedBytes),
			expectedText,
			sizeof(expectedText));
		FormatBytes(site,
			sizeof(kActorIsTalkingExpectedBytes),
			actualText,
			sizeof(actualText));

		Log("refusing to patch: unexpected Actor::IsTalking process guard bytes at %08X; expected [%s], actual [%s]",
			kActorIsTalkingPatchSite,
			expectedText,
			actualText);
		return kActorIsTalkingPatchMismatch;
	}

	static TESWorldSpaceOFSTPatchState GetTESWorldSpaceOFSTPatchState()
	{
		if (BytesMatch(kTESWorldSpaceLoadOFSTPatchSite,
				kTESWorldSpaceLoadOFSTExpectedBytes,
				sizeof(kTESWorldSpaceLoadOFSTExpectedBytes)))
		{
			return kTESWorldSpaceOFSTPatchNeedsInstall;
		}

		const UInt8* site = (const UInt8*)kTESWorldSpaceLoadOFSTPatchSite;
		if (site[0] == 0xE9)
		{
			UInt32 target = kTESWorldSpaceLoadOFSTPatchSite + 5 + *(const SInt32*)(site + 1);
			if (target == kTESWorldSpaceLoadOFSTContinueSite &&
				BytesMatch(kTESWorldSpaceLoadOFSTPatchSite + 5,
					kTESWorldSpaceLoadOFSTExpectedBytes + 5,
					sizeof(kTESWorldSpaceLoadOFSTExpectedBytes) - 5))
			{
				Log("TESWorldSpace WRLD OFST load already skipped at %08X; treating WRLD/OFST offset policy as externally owned",
					kTESWorldSpaceLoadOFSTPatchSite);
				return kTESWorldSpaceOFSTPatchExternallySkipped;
			}
		}

		char expectedText[512];
		char actualText[512];
		FormatBytes(kTESWorldSpaceLoadOFSTExpectedBytes,
			sizeof(kTESWorldSpaceLoadOFSTExpectedBytes),
			expectedText,
			sizeof(expectedText));
		FormatBytes(site,
			sizeof(kTESWorldSpaceLoadOFSTExpectedBytes),
			actualText,
			sizeof(actualText));

		Log("refusing to patch: unexpected TESWorldSpace WRLD OFST load bytes at %08X; expected [%s], actual [%s]",
			kTESWorldSpaceLoadOFSTPatchSite,
			expectedText,
			actualText);
		return kTESWorldSpaceOFSTPatchMismatch;
	}

	static __declspec(naked) void RendererInitFailurePatch()
	{
		__asm
		{
			test	al, al
			jnz		rendererModeOk

			push	00498E7Dh
			ret

		rendererModeOk:
			or		ebp, 0FFFFFFFFh
			xor		ebx, ebx
			push	004983C5h
			ret
		}
	}

	static __declspec(naked) void ActorGetAttackedProcessGuardPatch()
	{
		__asm
		{
			mov		ecx, [ecx + 58h]
			test	ecx, ecx
			jz		noProcess

			mov		eax, [ecx]
			push	005E58C5h
			ret

		noProcess:
			xor		eax, eax
			ret
		}
	}

	static __declspec(naked) void ActorIsTalkingProcessGuardPatch()
	{
		__asm
		{
			mov		ecx, [eax + 58h]
			test	ecx, ecx
			jz		noProcess

			mov		edx, [ecx]
			push	005E0E67h
			ret

		noProcess:
			xor		al, al
			push	005E0E70h
			ret
		}
	}

	static __declspec(naked) void TexturePaletteUnderscorePatch()
	{
		__asm
		{
			mov		esi, eax
			sub		esi, ebx
			add		esp, 8

			test	eax, eax
			jnz		notNull

			push	004A27A1h
			ret

		notNull:
			push	004A26FEh
			ret
		}
	}

	static UInt32 HashWorldspaceOFST(UInt32 worldspace)
	{
		return ((worldspace >> 4) ^ (worldspace >> 12)) & (kWorldspaceOFSTTrackingCapacity - 1);
	}

	static bool WorldspaceFloatToCellBound(UInt8* worldspace, UInt32 offset, SInt32* out)
	{
		if (!worldspace || !out)
			return false;

		const float value = *(float*)(worldspace + offset);
		if (!_finite(value) || value < -2147483648.0f || value > 2147483520.0f)
			return false;

		*out = ((SInt32)value) >> 12;
		return true;
	}

	static bool GetRebuiltWorldspaceOFSTCount(UInt8* worldspace, UInt32* count)
	{
		if (count)
			*count = 0;

		SInt32 minX = 0;
		SInt32 minY = 0;
		SInt32 maxX = 0;
		SInt32 maxY = 0;
		if (!WorldspaceFloatToCellBound(worldspace, 0x98, &minX) ||
			!WorldspaceFloatToCellBound(worldspace, 0x9C, &minY) ||
			!WorldspaceFloatToCellBound(worldspace, 0xA0, &maxX) ||
			!WorldspaceFloatToCellBound(worldspace, 0xA4, &maxY))
		{
			return false;
		}

		const SInt32 width = maxX - minX + 1;
		const SInt32 height = maxY - minY + 1;
		if (width <= 0 || height <= 0 || width >= 0x3E8 || height >= 0x3E8)
			return false;

		if (count)
			*count = (UInt32)width * (UInt32)height;
		return true;
	}

	static UInt32 FindWorldspaceOFSTSlot(UInt8* worldspace, bool* found)
	{
		if (found)
			*found = false;

		if (!worldspace)
			return kWorldspaceOFSTTrackingCapacity;

		const UInt32 key = (UInt32)worldspace;
		UInt32 slot = HashWorldspaceOFST(key);
		for (UInt32 probe = 0; probe < kWorldspaceOFSTTrackingCapacity; probe++)
		{
			WorldspaceOFSTTrackingEntry& entry = s_worldspaceOFSTTracking[slot];
			const UInt32 entryWorldspace = entry.worldspace;
			if (entryWorldspace == key)
			{
				if (found)
					*found = true;
				return slot;
			}

			if (!entryWorldspace)
				return slot;

			slot = (slot + 1) & (kWorldspaceOFSTTrackingCapacity - 1);
		}

		return kWorldspaceOFSTTrackingCapacity;
	}

	static void TrackWorldspaceOFST(UInt8* worldspace, UInt32* table, UInt32 count, UInt32 byteLength)
	{
		bool found = false;
		UInt32 slot = FindWorldspaceOFSTSlot(worldspace, &found);
		if (slot >= kWorldspaceOFSTTrackingCapacity)
		{
			++s_worldspaceOFSTTrackingFailures;
			if (s_worldspaceOFSTTrackingFailures <= 16)
				Log("failed to track WRLD OFST table worldspace=%08X table=%08X count=%u bytes=%u",
					(UInt32)worldspace,
					(UInt32)table,
					count,
					byteLength);
			return;
		}

		WorldspaceOFSTTrackingEntry& entry = s_worldspaceOFSTTracking[slot];
		entry.table = 0;
		MemoryBarrier();
		entry.count = count;
		entry.byteLength = byteLength;
		MemoryBarrier();
		entry.table = (UInt32)table;
		MemoryBarrier();
		entry.worldspace = (UInt32)worldspace;
	}

	static void UntrackWorldspaceOFST(UInt8* worldspace, UInt32* table)
	{
		bool found = false;
		UInt32 slot = FindWorldspaceOFSTSlot(worldspace, &found);
		if (!found || slot >= kWorldspaceOFSTTrackingCapacity)
			return;

		WorldspaceOFSTTrackingEntry& entry = s_worldspaceOFSTTracking[slot];
		if (entry.table != (UInt32)table)
			return;

		entry.table = 0;
		MemoryBarrier();
		entry.count = 0;
		entry.byteLength = 0;
	}

	static bool LookupWorldspaceOFSTCount(UInt8* worldspace, UInt32* table, UInt32* count)
	{
		bool found = false;
		UInt32 slot = FindWorldspaceOFSTSlot(worldspace, &found);
		if (!found || slot >= kWorldspaceOFSTTrackingCapacity)
			return false;

		WorldspaceOFSTTrackingEntry& entry = s_worldspaceOFSTTracking[slot];
		const UInt32 trackedTable = entry.table;
		MemoryBarrier();
		if (trackedTable != (UInt32)table)
			return false;

		if (count)
			*count = entry.count;
		return true;
	}

	static void __stdcall LoadWorldspaceOFSTChunk(UInt8* worldspace, UInt8* file)
	{
		if (!worldspace || !file)
			return;

		const UInt32 byteLength = *(UInt32*)(file + 0x254);
		if (!byteLength)
			return;

		UInt32* const oldTable = *(UInt32**)(worldspace + 0xA8);
		if (oldTable)
		{
			UntrackWorldspaceOFST(worldspace, oldTable);
			((FormHeapFreeFn)kFormHeapFreeAddr)(oldTable);
			*(UInt32**)(worldspace + 0xA8) = NULL;
		}

		if (byteLength > kMaxWorldspaceOFSTBytes)
		{
			++s_rejectedWorldspaceOFSTTables;
			if (s_rejectedWorldspaceOFSTTables <= 16)
				Log("rejected oversized WRLD OFST table worldspace=%08X file=%08X bytes=%u",
					(UInt32)worldspace,
					(UInt32)file,
					byteLength);
			return;
		}

		const UInt32 entryCount = byteLength >> 2;
		const UInt32 paddedLength = (byteLength + 3) & ~3U;
		UInt32* const table = (UInt32*)((FormHeapAllocFn)kFormHeapAllocAddr)(paddedLength);
		if (!table)
		{
			++s_rejectedWorldspaceOFSTTables;
			if (s_rejectedWorldspaceOFSTTables <= 16)
				Log("failed to allocate WRLD OFST table worldspace=%08X file=%08X bytes=%u padded=%u",
					(UInt32)worldspace,
					(UInt32)file,
					byteLength,
					paddedLength);
			return;
		}

		std::memset(table, 0, paddedLength);
		if (!((TESFileGetChunkDataFn)kTESFileGetChunkDataAddr)(file, (char*)table, byteLength))
		{
			((FormHeapFreeFn)kFormHeapFreeAddr)(table);
			++s_rejectedWorldspaceOFSTTables;
			if (s_rejectedWorldspaceOFSTTables <= 16)
				Log("failed to read WRLD OFST table worldspace=%08X file=%08X bytes=%u",
					(UInt32)worldspace,
					(UInt32)file,
					byteLength);
			return;
		}

		*(UInt32**)(worldspace + 0xA8) = table;
		TrackWorldspaceOFST(worldspace, table, entryCount, byteLength);
		++s_loadedWorldspaceOFSTTables;

		if ((byteLength & 3) && s_loadedWorldspaceOFSTTables <= 16)
		{
			Log("loaded WRLD OFST table with ignored tail worldspace=%08X file=%08X bytes=%u completeEntries=%u",
				(UInt32)worldspace,
				(UInt32)file,
				byteLength,
				entryCount);
		}
	}

	static UInt32 __stdcall ResolveWorldspaceCellOffsetTarget(
		UInt8* worldspace,
		UInt8* file,
		UInt32* table,
		UInt32 index,
		UInt32* target)
	{
		if (target)
			*target = 0;

		UInt32 count = 0;
		if (!LookupWorldspaceOFSTCount(worldspace, table, &count))
		{
			++s_untrackedWorldspaceOFSTReads;
			if (s_untrackedWorldspaceOFSTReads <= 16)
				Log("falling back for untracked WRLD OFST table worldspace=%08X table=%08X index=%u",
					(UInt32)worldspace,
					(UInt32)table,
					index);
			return 0;
		}

		if (index >= count)
		{
			++s_rejectedWorldspaceOFSTIndexes;
			if (s_rejectedWorldspaceOFSTIndexes <= 16)
				Log("falling back for out-of-range WRLD OFST index worldspace=%08X table=%08X index=%u count=%u",
					(UInt32)worldspace,
					(UInt32)table,
					index,
					count);
			return 0;
		}

		const UInt32 offset = table[index];
		if (!offset)
			return 2;

		if (!file)
			return 0;

		const UInt32 baseOffset = *(UInt32*)(worldspace + 0xBC);
		const UInt32 fileSize = *(UInt32*)(file + 0x258);
		bool valid = baseOffset < fileSize;
		UInt32 resolvedTarget = 0;
		if (valid)
		{
			const UInt32 remaining = fileSize - baseOffset;
			valid = offset < remaining;
			if (valid)
				resolvedTarget = baseOffset + offset;
		}

		if (!valid)
		{
			++s_rejectedWorldspaceOFSTTargets;
			if (s_rejectedWorldspaceOFSTTargets <= 16)
				Log("falling back for invalid WRLD OFST target worldspace=%08X file=%08X base=%u offset=%u fileSize=%u",
					(UInt32)worldspace,
					(UInt32)file,
					baseOffset,
					offset,
					fileSize);
			return 0;
		}

		if (target)
			*target = resolvedTarget;
		return 1;
	}

	static void __stdcall WriteRebuiltWorldspaceOFSTOffset(
		UInt8* worldspace,
		UInt32* table,
		UInt32 index,
		UInt32 cellOffset)
	{
		UInt32 count = 0;
		if (!table || !GetRebuiltWorldspaceOFSTCount(worldspace, &count) || index >= count)
		{
			++s_rejectedRebuiltWorldspaceOFSTWrites;
			if (s_rejectedRebuiltWorldspaceOFSTWrites <= 16)
				Log("skipped rebuilt WRLD OFST write worldspace=%08X table=%08X index=%u count=%u offset=%u",
					(UInt32)worldspace,
					(UInt32)table,
					index,
					count,
					cellOffset);
			return;
		}

		table[index] = cellOffset;
	}

	static void __stdcall TrackRebuiltWorldspaceOFST(UInt8* worldspace, UInt32* table)
	{
		UInt32 count = 0;
		if (!table || !GetRebuiltWorldspaceOFSTCount(worldspace, &count))
			return;

		TrackWorldspaceOFST(worldspace, table, count, count * sizeof(UInt32));
		++s_trackedRebuiltWorldspaceOFSTTables;
	}

	static bool __stdcall ReadTrackedWorldspaceCellOffset(UInt8* worldspace, UInt32 index, UInt32* offset)
	{
		if (offset)
			*offset = 0;

		if (!worldspace)
			return false;

		UInt32* const table = *(UInt32**)(worldspace + 0xA8);
		UInt32 count = 0;
		if (!LookupWorldspaceOFSTCount(worldspace, table, &count))
			return false;

		if (index >= count)
		{
			++s_rejectedWorldspaceOFSTIndexes;
			return false;
		}

		if (offset)
			*offset = table[index];
		return true;
	}

	static __declspec(naked) void TESWorldSpaceLoadOFSTPatch()
	{
		__asm
		{
			push	edi
			push	esi
			call	LoadWorldspaceOFSTChunk
			xor		ebx, ebx
			mov		eax, 004F21B6h
			jmp		eax
		}
	}

	static __declspec(naked) void TESWorldSpaceFindCellOffsetPatch()
	{
		__asm
		{
			sub		esp, 4
			mov		dword ptr [esp], 0
			lea		ecx, [esp]
			push	ecx
			push	eax
			push	edi
			push	esi
			push	ebx
			call	ResolveWorldspaceCellOffsetTarget
			cmp		eax, 1
			jz		direct
			cmp		eax, 2
			jz		noOffset

			add		esp, 4
			mov		eax, 004EF58Bh
			jmp		eax

		direct:
			mov		edx, [esp]
			add		esp, 4
			push	edx
			mov		ecx, esi
			mov		eax, 00451460h
			call	eax
			mov		eax, 004EF575h
			jmp		eax

		noOffset:
			add		esp, 4
			mov		eax, 004EF580h
			jmp		eax
		}
	}

	static __declspec(naked) void TESWorldSpacePostFixupOffsetWritePatch()
	{
		__asm
		{
			mov		edx, [esp + 20h]
			push	ecx
			push	eax
			push	edx
			push	esi
			call	WriteRebuiltWorldspaceOFSTOffset
			mov		eax, 004F3065h
			jmp		eax
		}
	}

	static __declspec(naked) void TESWorldSpacePostFixupOffsetCommitPatch()
	{
		__asm
		{
			mov		ebx, [esp + 20h]
			push	ebx
			push	esi
			call	TrackRebuiltWorldspaceOFST
			mov		[esi + 0A8h], ebx
			mov		eax, 004F3268h
			jmp		eax
		}
	}

	static __declspec(naked) void TESWorldSpacePostFixupOffsetReadPatch()
	{
		__asm
		{
			sub		esp, 4
			mov		dword ptr [esp], 0
			lea		ecx, [esp]
			push	ecx
			push	eax
			push	esi
			call	ReadTrackedWorldspaceCellOffset
			test	al, al
			jz		skip

			mov		ecx, [esp]
			add		esp, 4
			test	ecx, ecx
			jz		skipNoLocal

			mov		eax, 004F31A6h
			jmp		eax

		skip:
			add		esp, 4

		skipNoLocal:
			mov		eax, 004F324Eh
			jmp		eax
		}
	}

	static bool __stdcall CheckedTESFileNextRecordAdvance(UInt8* file)
	{
		if (!file)
			return false;

		const UInt32 length = *(UInt32*)(file + 0x240);
		const UInt32 fileSize = *(UInt32*)(file + 0x258);
		const UInt32 currentOffset = *(UInt32*)(file + 0x25C);

		bool valid = currentOffset <= fileSize;
		UInt32 nextOffset = fileSize;
		if (valid)
		{
			const UInt32 remaining = fileSize - currentOffset;
			valid = remaining >= 0x14 && length <= remaining - 0x14;
			if (valid)
			{
				nextOffset = currentOffset + length + 0x14;
				valid = nextOffset >= currentOffset;
			}
		}

		if (!valid)
		{
			*(UInt32*)(file + 0x25C) = fileSize;
			++s_rejectedTESFileRecordAdvances;
			if (s_rejectedTESFileRecordAdvances <= 16)
			{
				Log("rejected TESFile_NextRecord advance file=%08X current=%u length=%u fileSize=%u",
					(UInt32)file,
					currentOffset,
					length,
					fileSize);
			}
			return false;
		}

		*(UInt32*)(file + 0x25C) = nextOffset;
		return true;
	}

	static bool __stdcall CheckedTESFileGetNextChunkAdvance(UInt8* file, UInt32 recordLength)
	{
		if (!file)
			return false;

		const UInt32 chunkLength = *(UInt32*)(file + 0x254);
		const UInt32 currentOffset = *(UInt32*)(file + 0x260);

		if (currentOffset == recordLength)
			return false;

		bool valid = currentOffset < recordLength;
		UInt32 nextOffset = recordLength;
		if (valid)
		{
			const UInt32 remaining = recordLength - currentOffset;
			valid = remaining >= 6 && chunkLength <= remaining - 6;
			if (valid)
			{
				nextOffset = currentOffset + chunkLength + 6;
				valid = nextOffset >= currentOffset;
			}
		}

		if (!valid)
		{
			*(UInt32*)(file + 0x260) = recordLength;
			++s_rejectedTESFileChunkAdvances;
			if (s_rejectedTESFileChunkAdvances <= 16)
			{
				Log("rejected TESFile_GetNextChunk advance file=%08X current=%u chunkLength=%u recordLength=%u",
					(UInt32)file,
					currentOffset,
					chunkLength,
					recordLength);
			}
			return false;
		}

		*(UInt32*)(file + 0x260) = nextOffset;
		return nextOffset < recordLength;
	}

	static bool __stdcall ValidateTESFileDecompressedRecordHeader(UInt8* compressedBuffer, UInt32 compressedLength)
	{
		if (!compressedBuffer || compressedLength < sizeof(UInt32))
		{
			++s_rejectedTESFileCompressedRecords;
			if (s_rejectedTESFileCompressedRecords <= 16)
			{
				Log("rejected compressed TES record header buffer=%08X length=%u",
					(UInt32)compressedBuffer,
					compressedLength);
			}
			return false;
		}

		const UInt32 decompressedLength = *(UInt32*)compressedBuffer;
		if (!decompressedLength || decompressedLength > kMaxTESFileDecompressedRecordBytes)
		{
			++s_rejectedTESFileCompressedRecords;
			if (s_rejectedTESFileCompressedRecords <= 16)
			{
				Log("rejected compressed TES record advertisedLength=%u compressedLength=%u",
					decompressedLength,
					compressedLength);
			}
			return false;
		}

		return true;
	}

	static void __stdcall SafeCopyTESFileCompressedChunkData(UInt8* file, void* dst, UInt32 requested)
	{
		if (!file)
			return;

		UInt32 copied = 0;
		UInt8* const dcBuffer = *(UInt8**)(file + 0x414);
		const UInt32 dcLength = *(UInt32*)(file + 0x418);
		const UInt32 chunkOffset = *(UInt32*)(file + 0x260);

		bool valid = dcBuffer != NULL && chunkOffset <= dcLength;
		UInt32 sourceOffset = 0;
		if (valid)
		{
			sourceOffset = chunkOffset + 6;
			valid = sourceOffset >= chunkOffset && sourceOffset <= dcLength;
		}

		if (valid)
		{
			const UInt32 available = dcLength - sourceOffset;
			copied = requested <= available ? requested : available;
			if (dst && copied)
				std::memcpy(dst, dcBuffer + sourceOffset, copied);
		}

		if (dst && copied < requested)
			std::memset((UInt8*)dst + copied, 0, requested - copied);

		*(UInt32*)(file + 0x264) = copied;

		if (copied != requested)
		{
			++s_clippedTESFileCompressedChunkCopies;
			if (s_clippedTESFileCompressedChunkCopies <= 16)
			{
				Log("clipped compressed TES chunk copy file=%08X chunkOffset=%u dcLength=%u requested=%u copied=%u",
					(UInt32)file,
					chunkOffset,
					dcLength,
					requested,
					copied);
			}
		}
	}

	static void __cdecl SkipTESFormRecordCompression(void* zlibStream, void* tempBuffer, const char* reason)
	{
		if (zlibStream)
			((ZlibStreamEndFn)kZlibStreamEndAddr)(zlibStream);

		if (tempBuffer)
			((FormHeapFreeFn)kFormHeapFreeAddr)(tempBuffer);

		++s_skippedTESFormRecordCompressions;
		if (s_skippedTESFormRecordCompressions <= 16)
		{
			Log("left TESForm record uncompressed because %s stream=%08X temp=%08X",
				reason,
				(UInt32)zlibStream,
				(UInt32)tempBuffer);
		}
	}

	static void __cdecl SkipTESFormRecordCompressionTempBuffer(void* zlibStream)
	{
		SkipTESFormRecordCompression(zlibStream, NULL, "temp compression buffer allocation failed or overflowed");
	}

	static void __cdecl SkipTESFormRecordCompressionDeflateError(void* zlibStream, void* tempBuffer)
	{
		SkipTESFormRecordCompression(zlibStream, tempBuffer, "zlib deflate failed");
	}

	static void __cdecl SkipTESFormRecordCompressionFinalBuffer(void* zlibStream, void* tempBuffer)
	{
		SkipTESFormRecordCompression(zlibStream, tempBuffer, "final compressed record allocation failed");
	}

	static char __fastcall TESFileNextGroupPatch(UInt8* file, void*)
	{
		if (!file)
			return 0;

		const UInt32 grupType = *(UInt32*)kTESRecordTypeGRUPAddr;
		if (*(UInt32*)(file + 0x23C) != grupType)
			return 0;

		UInt32* const length = (UInt32*)(file + 0x240);
		if (*length < 0x14)
		{
			const UInt32 originalLength = *length;
			*(UInt32*)(file + 0x23C) = 0;
			*length = 0;
			++s_rejectedTESFileGroupAdvances;
			if (s_rejectedTESFileGroupAdvances <= 16)
			{
				Log("rejected short TES GRUP advance file=%08X length=%u",
					(UInt32)file,
					originalLength);
			}
			return 0;
		}

		*(UInt32*)(file + 0x23C) = 0;
		*length -= 0x14;
		return ((TESFileNextRecordFn)kTESFileNextRecordAddr)(file);
	}

	static __declspec(naked) void TESFormCompressRecordTempAllocPatch()
	{
		__asm
		{
			push	ebp
			push	esi
			test	ebx, 80000000h
			jnz		failedNoSize

			lea		esi, [ebx + ebx]
			push	esi
			mov		eax, 00401F00h
			call	eax
			mov		ebp, eax
			test	ebp, ebp
			jz		failedWithSize

			mov		eax, [esp + 14h]
			mov		ecx, 0046B3F9h
			jmp		ecx

		failedWithSize:
			lea		eax, [esp + 1Ch]
			push	eax
			call	SkipTESFormRecordCompressionTempBuffer
			add		esp, 8
			mov		eax, 0046B430h
			jmp		eax

		failedNoSize:
			lea		eax, [esp + 18h]
			push	eax
			call	SkipTESFormRecordCompressionTempBuffer
			add		esp, 4
			mov		eax, 0046B430h
			jmp		eax
		}
	}

	static __declspec(naked) void TESFormCompressRecordDeflateErrorCleanupPatch()
	{
		__asm
		{
			lea		eax, [esp + 1Ch]
			push	ebp
			push	eax
			call	SkipTESFormRecordCompressionDeflateError
			add		esp, 0Ch
			mov		eax, 0046B430h
			jmp		eax
		}
	}

	static __declspec(naked) void TESFormCompressRecordFinalAllocPatch()
	{
		__asm
		{
			test	eax, eax
			jz		failed

			or		dword ptr [edi + 8], 00040000h
			mov		ecx, 0046B450h
			jmp		ecx

		failed:
			lea		ecx, [esp + 1Ch]
			push	ebp
			push	ecx
			call	SkipTESFormRecordCompressionFinalBuffer
			add		esp, 0Ch
			mov		ecx, 0046B430h
			jmp		ecx
		}
	}

	static __declspec(naked) void TESFileNextRecordAdvancePatch()
	{
		__asm
		{
			push	esi
			call	CheckedTESFileNextRecordAdvance
			test	al, al
			jz		stop

			mov		eax, 00451342h
			jmp		eax

		stop:
			mov		eax, 00451350h
			jmp		eax
		}
	}

	static __declspec(naked) void TESFileGetNextChunkAdvancePatch()
	{
		__asm
		{
			push	ecx
			push	edx
			push	edi
			push	edx
			push	esi
			call	CheckedTESFileGetNextChunkAdvance
			test	al, al
			pop		edi
			pop		edx
			pop		ecx
			jz		stop

			mov		eax, [esi + 260h]
			mov		edx, 0044FEF7h
			jmp		edx

		stop:
			mov		eax, 0044FEE4h
			jmp		eax
		}
	}

	static __declspec(naked) void TESFileDecompressedRecordSizePatch()
	{
		__asm
		{
			push	ebx
			push	edi
			call	ValidateTESFileDecompressedRecordHeader
			test	al, al
			jz		invalid

			mov		ebp, [edi]
			push	ebp
			mov		byte ptr [ebx + edi], 0
			mov		eax, 004505F6h
			jmp		eax

		invalid:
			mov		ecx, esi
			mov		eax, 004506F6h
			jmp		eax
		}
	}

	static __declspec(naked) void TESFileLoadChunkHeaderCompressedTailPatch()
	{
		__asm
		{
			mov		edx, [esi + 418h]
			mov		ecx, edi
			add		ecx, 6
			jc		invalid
			cmp		ecx, edx
			ja		invalid

			mov		ecx, [eax + edi]
			mov		[esp + 10h], ecx
			mov		dx, [eax + edi + 4]
			mov		[esp + 14h], dx
			mov		ecx, 00450FB3h
			jmp		ecx

		invalid:
			inc		s_rejectedTESFileCompressedChunkHeaders
			mov		ecx, 00450F65h
			jmp		ecx
		}
	}

	static __declspec(naked) void TESFileGetChunkDataTruncatedCopyPatch()
	{
		__asm
		{
			lea		eax, [edi - 1]
			push	eax
			push	ebp
			push	esi
			call	SafeCopyTESFileCompressedChunkData
			mov		eax, 00450D84h
			jmp		eax
		}
	}

	static __declspec(naked) void TESFileGetChunkDataFullCopyPatch()
	{
		__asm
		{
			mov		eax, [esi + 254h]
			push	eax
			push	ebp
			push	esi
			call	SafeCopyTESFileCompressedChunkData
			mov		eax, 00450EEDh
			jmp		eax
		}
	}

	static __declspec(naked) void TESFileJumpToRecordValidatePatch()
	{
		__asm
		{
			mov		eax, [esp + 0Ch]
			cmp		eax, [esi + 258h]
			jae		invalid

			mov		[esi + 25Ch], eax
			mov		ecx, 004514C0h
			jmp		ecx

		invalid:
			inc		s_rejectedTESFileJumpTargets
			xor		eax, eax
			mov		[esi + 23Ch], eax
			mov		[esi + 240h], eax
			mov		[esi + 244h], eax
			mov		[esi + 248h], eax
			mov		[esi + 24Ch], eax
			pop		edi
			xor		al, al
			pop		esi
			ret		4
		}
	}

	static __declspec(naked) void ArchiveRawReadTargetGuardPatch()
	{
		__asm
		{
			mov		edi, [esi + 158h]
			mov		edx, [esi + 148h]
			mov		ecx, [esi + 150h]
			cmp		edx, ecx
			ja		invalid

			mov		eax, ecx
			sub		eax, edx
			cmp		ebx, eax
			ja		invalid

			add		edi, edx
			jc		invalid
			add		edi, ebx
			jc		invalid
			cmp		edi, 0FFFFFFFFh
			jz		invalid

			mov		eax, [esi + 154h]
			mov		ecx, 0042C3FDh
			jmp		ecx

		invalid:
			inc		s_rejectedArchiveRawReadTargets
			pop		edi
			pop		esi
			xor		eax, eax
			pop		ebx
			ret		0Ch
		}
	}

	static __declspec(naked) void ArchiveRawReadCountClampPatch()
	{
		__asm
		{
			mov		edx, [esi + 148h]
			mov		ecx, [esp + 14h]
			mov		eax, [esi + 150h]
			push	ebp
			cmp		edx, eax
			ja		invalid

			mov		ebp, eax
			sub		ebp, edx
			cmp		ebx, ebp
			ja		invalid

			sub		ebp, ebx
			cmp		ecx, ebp
			jbe		done

			mov		ecx, ebp
			inc		s_clampedArchiveRawReads
			jmp		done

		invalid:
			xor		ecx, ecx
			inc		s_clampedArchiveRawReads

		done:
			pop		ebp
			mov		eax, 0042C45Ah
			jmp		eax
		}
	}

	static __declspec(naked) void CompressedArchiveRefillPatch()
	{
		__asm
		{
			cmp		dword ptr [ebx + 4], 0
			jnz		checkOutput

			mov		ecx, esi
			mov		eax, 00747D30h
			call	eax
			mov		ecx, [esi + 0Ch]
			mov		edx, [esi + 18h]
			push	0
			push	ecx
			push	edx
			mov		ecx, esi
			mov		eax, 0042C3E0h
			call	eax
			mov		[esi + 10h], eax
			test	eax, eax
			jnz		checkOutput

			cmp		dword ptr [ebx + 10h], 0
			jnz		refillFailed
			mov		ecx, 0042C818h
			jmp		ecx

		checkOutput:
			cmp		dword ptr [ebx + 10h], 0
			jnz		continueLoop
			mov		ecx, 0042C818h
			jmp		ecx

		continueLoop:
			mov		ecx, 0042C780h
			jmp		ecx

		refillFailed:
			inc		s_rejectedCompressedArchiveRefills
			mov		ecx, 0042C7F3h
			jmp		ecx
		}
	}

	static __declspec(naked) void LoadGameStartTickPatch()
	{
		__asm
		{
			mov		esi, ds:0A280D0h
			call	esi
			mov		ds:0B33B08h, eax
			mov		ecx, eax
			call	esi
			sub		eax, ecx
			cmp		eax, 0BB8h
			pop		esi
			ja		elapsed
			ret

		elapsed:
			push	00459A63h
			ret
		}
	}

	static void __cdecl DuplicateMainThreadHandle(HANDLE* targetHandle)
	{
		if (!targetHandle)
			return;

		HANDLE duplicated = NULL;
		HANDLE currentProcess = GetCurrentProcess();
		HANDLE currentThread = GetCurrentThread();
		if (DuplicateHandle(currentProcess,
				currentThread,
				currentProcess,
				&duplicated,
				0,
				FALSE,
				DUPLICATE_SAME_ACCESS))
		{
			*targetHandle = duplicated;
		}
		else
		{
			*targetHandle = NULL;
			Log("failed to duplicate main thread handle: %lu", GetLastError());
		}
	}

	static __declspec(naked) void MainThreadHandleDuplicatePatch()
	{
		__asm
		{
			push	edi
			call	DuplicateMainThreadHandle
			add		esp, 4
			push	00404A68h
			ret
		}
	}

	typedef char (__cdecl* PumpWaitInputFn)(char pumpInput);
	typedef void (__cdecl* NoArgFn)();
	typedef void (__thiscall* InputGlobalUpdateFn)(void* inputGlobals);
	typedef void (__thiscall* SaveLoadLoadDataFn)(void* saveLoad, void* dst, UInt32 size);

	static void __cdecl LoadingTextureWaitPatch()
	{
		float seconds = *(float*)kLoadingTextureWaitSecondsAddr * kStaticLoadingScreenWaitScale;

		double milliseconds = (double)seconds * 1000.0;
		if (!(milliseconds > 0.0) || milliseconds >= 2147483647.0)
			return;

		SInt32 duration = (SInt32)milliseconds;
		DWORD start = GetTickCount();
		PumpWaitInputFn pump = (PumpWaitInputFn)kLoadingTextureWaitPumpAddr;

		while ((SInt32)(GetTickCount() - start) < duration)
		{
			if (!pump(1))
				break;
		}
	}

	static void __cdecl WorldspaceMessageDelayPatch()
	{
		NoArgFn step1 = (NoArgFn)kWorldspaceMessageDelayStep1Addr;
		NoArgFn step2 = (NoArgFn)kWorldspaceMessageDelayStep2Addr;
		InputGlobalUpdateFn updateInputGlobals = (InputGlobalUpdateFn)kWorldspaceMessageDelayInputUpdateAddr;
		DWORD start = GetTickCount();

		while ((SInt32)(GetTickCount() - start) < 1000)
		{
			step1();
			step2();
			updateInputGlobals(*(void**)kWorldspaceMessageDelayInputGlobalsAddr);
		}
	}

	static void __cdecl LoadGlobalAnimTimerAndClamp(void* saveLoad, void* dst, UInt32 size)
	{
		((SaveLoadLoadDataFn)kSaveLoadLoadDataAddr)(saveLoad, dst, size);

		if (dst != (void*)kGlobalAnimTimerAddr || size != sizeof(float))
			return;

		float& timer = *(float*)kGlobalAnimTimerAddr;
		if (timer >= kGlobalAnimTimerPrecisionLimit)
		{
			Log("reset loaded global animation timer from %.3f to 0.000", timer);
			timer = 0.0f;
		}
	}

	static __declspec(naked) void GlobalAnimTimerLoadPatch()
	{
		__asm
		{
			mov		eax, [esp + 8]
			mov		edx, [esp + 4]
			push	eax
			push	edx
			push	ecx
			call	LoadGlobalAnimTimerAndClamp
			add		esp, 0Ch
			ret		8
		}
	}

	static bool IsRebasableTime(float value)
	{
		return _finite(value) && value > -kSentinelMagnitude && value < kSentinelMagnitude;
	}

	static float& ActorAnimClock(void* animData)
	{
		return *(float*)((UInt8*)animData + kActorAnimClockOffset);
	}

	static void* ActorAnimSequence(void* animData, UInt32 index)
	{
		return *(void**)((UInt8*)animData + kActorAnimSequenceListOffset + index * sizeof(void*));
	}

	static float& SequenceOffset(void* sequence)
	{
		return *(float*)((UInt8*)sequence + kNiControllerSequenceOffsetOffset);
	}

	static void __cdecl RebaseActorAnimData(void* animData)
	{
		if (!animData)
			return;

		float& clock = ActorAnimClock(animData);
		if (!IsRebasableTime(clock) || clock < kActorAnimRebaseThreshold)
			return;

		UInt32 chunks = (UInt32)(clock / kActorAnimRebaseStep);
		if (!chunks)
			return;

		float shift = (float)chunks * kActorAnimRebaseStep;
		if (shift <= 0.0f || shift > clock)
			return;

		for (UInt32 i = 0; i < kActorAnimSequenceCount; i++)
		{
			void* sequence = ActorAnimSequence(animData, i);
			if (!sequence)
				continue;

			float& offset = SequenceOffset(sequence);
			if (IsRebasableTime(offset))
				offset += shift;
		}

		float oldClock = clock;
		clock -= shift;

		if (s_actorAnimRebaseLogCount < 32)
		{
			Log("rebased ActorAnimData %08X clock from %.3f to %.3f, shifted active sequence offsets by %.3f",
				(UInt32)animData,
				oldClock,
				clock,
				shift);
			s_actorAnimRebaseLogCount++;
		}
	}

	static __declspec(naked) void ActorAnimClockPatch()
	{
		__asm
		{
			pushad
			push	esi
			call	RebaseActorAnimData
			add		esp, 4
			popad

			mov		ecx, ebx
			sub		ecx, 5
			ret
		}
	}

	static __declspec(naked) void ActorAnimPreSamplePatch()
	{
		__asm
		{
			pushad
			push	esi
			call	RebaseActorAnimData
			add		esp, 4
			popad

			xor		edi, edi
			ret
		}
	}

	static bool ValidateDecode(ActorAnimClockPatchState actorAnimClockPatchState,
		TESWorldSpaceOFSTPatchState worldspaceOFSTPatchState)
	{
		return ValidateDecodeBytes("OSGlobals main thread handle duplicate",
				kMainThreadHandlePatchSite,
				kMainThreadHandleExpectedBytes,
				sizeof(kMainThreadHandleExpectedBytes)) &&
			ValidateDecodeBytes("BinkOpen main thread suspend use",
				kBinkOpenThreadHandleUseDecodeAddr,
				kBinkOpenThreadHandleUseExpectedBytes,
				sizeof(kBinkOpenThreadHandleUseExpectedBytes)) &&
			ValidateDecodeBytes("BinkOpen main thread resume use",
				kBinkOpenResumeThreadUseDecodeAddr,
				kBinkOpenResumeThreadUseExpectedBytes,
				sizeof(kBinkOpenResumeThreadUseExpectedBytes)) &&
			ValidateDecodeBytes("SaveLoad create-buffer tracker",
				kSaveLoadCreateBufferPatchSite,
				kSaveLoadCreateBufferExpectedBytes,
				sizeof(kSaveLoadCreateBufferExpectedBytes)) &&
			ValidateDecodeBytes("SaveLoad free-buffer tracker",
				kSaveLoadFreeBufferPatchSite,
				kSaveLoadFreeBufferExpectedBytes,
				sizeof(kSaveLoadFreeBufferExpectedBytes)) &&
			ValidateDecodeBytes("SaveLoad +0x54 raw blob helper",
				kSaveLoadRawBlobMap54PatchSite,
				kSaveLoadRawBlobMap54ExpectedBytes,
				sizeof(kSaveLoadRawBlobMap54ExpectedBytes)) &&
			ValidateDecodeBytes("SaveLoad +0x58 raw blob helper",
				kSaveLoadRawBlobMap58PatchSite,
				kSaveLoadRawBlobMap58ExpectedBytes,
				sizeof(kSaveLoadRawBlobMap58ExpectedBytes)) &&
			ValidateDecodeBytes("SaveLoad +0x5C raw blob helper",
				kSaveLoadRawBlobMap5CPatchSite,
				kSaveLoadRawBlobMap5CExpectedBytes,
				sizeof(kSaveLoadRawBlobMap5CExpectedBytes)) &&
			ValidateDecodeBytes("SaveLoad +0x60 raw blob helper",
				kSaveLoadRawBlobMap60PatchSite,
				kSaveLoadRawBlobMap60ExpectedBytes,
				sizeof(kSaveLoadRawBlobMap60ExpectedBytes)) &&
			ValidateDecodeBytes("TESSpellList saved count",
				kTESSpellListCountPatchSite,
				kTESSpellListCountExpectedBytes,
				sizeof(kTESSpellListCountExpectedBytes)) &&
			ValidateDecodeBytes("TESSpellList list-node allocation",
				kTESSpellListNodeAllocPatchSite,
				kTESSpellListNodeAllocExpectedBytes,
				sizeof(kTESSpellListNodeAllocExpectedBytes)) &&
			ValidateDecodeBytes("AVCollection saved count",
				kAVCollectionCountPatchSite,
				kAVCollectionCountExpectedBytes,
				sizeof(kAVCollectionCountExpectedBytes)) &&
			ValidateDecodeBytes("MiddleHighProcess FormID saved count",
				kMiddleHighProcessFormIDCountPatchSite,
				kMiddleHighProcessFormIDCountExpectedBytes,
				sizeof(kMiddleHighProcessFormIDCountExpectedBytes)) &&
			ValidateDecodeBytes("MiddleHighProcess FormID list-node allocation",
				kMiddleHighProcessFormIDNodeAllocPatchSite,
				kMiddleHighProcessFormIDNodeAllocExpectedBytes,
				sizeof(kMiddleHighProcessFormIDNodeAllocExpectedBytes)) &&
			ValidateDecodeBytes("KnownEffects saved count",
				kKnownEffectsCountPatchSite,
				kKnownEffectsCountExpectedBytes,
				sizeof(kKnownEffectsCountExpectedBytes)) &&
			ValidateDecodeBytes("Actor relationship/disposition first loop",
				kActorRelationshipDispositionFirstPatchSite,
				kActorRelationshipDispositionFirstLoopExpectedBytes,
				sizeof(kActorRelationshipDispositionFirstLoopExpectedBytes)) &&
			ValidateDecodeBytes("Actor relationship/disposition second loop",
				kActorRelationshipDispositionSecondPatchSite,
				kActorRelationshipDispositionSecondLoopExpectedBytes,
				sizeof(kActorRelationshipDispositionSecondLoopExpectedBytes)) &&
			ValidateDecodeBytes("TESForm current-save data wrapper",
				kTESFormLoadDataFromCurrentSaveGameAddr,
				kTESFormLoadDataFromCurrentSaveGameExpectedBytes,
				sizeof(kTESFormLoadDataFromCurrentSaveGameExpectedBytes)) &&
			ValidateDecodeBytes("TESForm current-save FormID wrapper",
				kTESFormLoadFormIDFromCurrentSaveGameAddr,
				kTESFormLoadFormIDFromCurrentSaveGameExpectedBytes,
				sizeof(kTESFormLoadFormIDFromCurrentSaveGameExpectedBytes)) &&
			(!kInstallArchiveStreamingGuards ||
				(ValidateDecodeBytes("archive raw read target guard",
					kArchiveRawReadTargetPatchSite,
					kArchiveRawReadTargetExpectedBytes,
					sizeof(kArchiveRawReadTargetExpectedBytes)) &&
				ValidateDecodeBytes("archive raw read count clamp",
					kArchiveRawReadCountClampPatchSite,
					kArchiveRawReadCountClampExpectedBytes,
					sizeof(kArchiveRawReadCountClampExpectedBytes)) &&
				ValidateDecodeBytes("compressed archive refill",
					kCompressedArchiveRefillPatchSite,
					kCompressedArchiveRefillExpectedBytes,
					sizeof(kCompressedArchiveRefillExpectedBytes)))) &&
			ValidateDecodeBytes("TESFile_NextRecord advance",
				kTESFileNextRecordAdvancePatchSite,
				kTESFileNextRecordAdvanceExpectedBytes,
				sizeof(kTESFileNextRecordAdvanceExpectedBytes)) &&
			ValidateDecodeBytes("TESFile_GetNextChunk advance",
				kTESFileGetNextChunkAdvancePatchSite,
				kTESFileGetNextChunkAdvanceExpectedBytes,
				sizeof(kTESFileGetNextChunkAdvanceExpectedBytes)) &&
			ValidateDecodeBytes("TESFile compressed record size",
				kTESFileDecompressedRecordSizePatchSite,
				kTESFileDecompressedRecordSizeExpectedBytes,
				sizeof(kTESFileDecompressedRecordSizeExpectedBytes)) &&
			ValidateDecodeBytes("TESFile compressed chunk header tail",
				kTESFileLoadChunkHeaderCompressedTailPatchSite,
				kTESFileLoadChunkHeaderCompressedTailExpectedBytes,
				sizeof(kTESFileLoadChunkHeaderCompressedTailExpectedBytes)) &&
			ValidateDecodeBytes("TESFile truncated compressed chunk copy",
				kTESFileGetChunkDataTruncatedCopyPatchSite,
				kTESFileGetChunkDataTruncatedCopyExpectedBytes,
				sizeof(kTESFileGetChunkDataTruncatedCopyExpectedBytes)) &&
			ValidateDecodeBytes("TESFile full compressed chunk copy",
				kTESFileGetChunkDataFullCopyPatchSite,
				kTESFileGetChunkDataFullCopyExpectedBytes,
				sizeof(kTESFileGetChunkDataFullCopyExpectedBytes)) &&
			ValidateDecodeBytes("TESFile JumpToRecord target",
				kTESFileJumpToRecordValidatePatchSite,
				kTESFileJumpToRecordValidateExpectedBytes,
				sizeof(kTESFileJumpToRecordValidateExpectedBytes)) &&
			ValidateDecodeBytes("TESFile NextGroup",
				kTESFileNextGroupPatchSite,
				kTESFileNextGroupExpectedBytes,
				sizeof(kTESFileNextGroupExpectedBytes)) &&
			ValidateDecodeBytes("TESForm record compression temp allocation",
				kTESFormCompressRecordTempAllocPatchSite,
				kTESFormCompressRecordTempAllocExpectedBytes,
				sizeof(kTESFormCompressRecordTempAllocExpectedBytes)) &&
			ValidateDecodeBytes("TESForm record compression deflate-error cleanup",
				kTESFormCompressRecordDeflateErrorCleanupPatchSite,
				kTESFormCompressRecordDeflateErrorCleanupExpectedBytes,
				sizeof(kTESFormCompressRecordDeflateErrorCleanupExpectedBytes)) &&
			ValidateDecodeBytes("TESForm record compression final allocation",
				kTESFormCompressRecordFinalAllocPatchSite,
				kTESFormCompressRecordFinalAllocExpectedBytes,
				sizeof(kTESFormCompressRecordFinalAllocExpectedBytes)) &&
			(worldspaceOFSTPatchState != kTESWorldSpaceOFSTPatchNeedsInstall ||
				(ValidateDecodeBytes("TESWorldSpace WRLD OFST load",
					kTESWorldSpaceLoadOFSTPatchSite,
					kTESWorldSpaceLoadOFSTExpectedBytes,
					sizeof(kTESWorldSpaceLoadOFSTExpectedBytes)) &&
				ValidateDecodeBytes("TESWorldSpace exterior cell offset lookup",
					kTESWorldSpaceFindCellOffsetPatchSite,
					kTESWorldSpaceFindCellOffsetExpectedBytes,
					sizeof(kTESWorldSpaceFindCellOffsetExpectedBytes)) &&
				ValidateDecodeBytes("TESWorldSpace post-fixup rebuilt offset write",
					kTESWorldSpacePostFixupOffsetWritePatchSite,
					kTESWorldSpacePostFixupOffsetWriteExpectedBytes,
					sizeof(kTESWorldSpacePostFixupOffsetWriteExpectedBytes)) &&
				ValidateDecodeBytes("TESWorldSpace post-fixup rebuilt offset commit",
					kTESWorldSpacePostFixupOffsetCommitPatchSite,
					kTESWorldSpacePostFixupOffsetCommitExpectedBytes,
					sizeof(kTESWorldSpacePostFixupOffsetCommitExpectedBytes)) &&
				ValidateDecodeBytes("TESWorldSpace post-fixup offset validation read",
					kTESWorldSpacePostFixupOffsetReadPatchSite,
					kTESWorldSpacePostFixupOffsetReadExpectedBytes,
					sizeof(kTESWorldSpacePostFixupOffsetReadExpectedBytes)))) &&
			ValidateDecodeBytes("loading texture wait",
				kLoadingTextureWaitPatchSite,
				kLoadingTextureWaitExpectedBytes,
				sizeof(kLoadingTextureWaitExpectedBytes)) &&
			ValidateDecodeBytes("load-game start tick",
				kLoadGameStartTickPatchSite,
				kLoadGameStartTickExpectedBytes,
				sizeof(kLoadGameStartTickExpectedBytes)) &&
			ValidateDecodeBytes("worldspace message delay",
				kWorldspaceMessageDelayPatchSite,
				kWorldspaceMessageDelayExpectedBytes,
				sizeof(kWorldspaceMessageDelayExpectedBytes)) &&
			ValidateDecodeBytes("global animation timer update",
				kGlobalAnimTimerUpdateDecodeAddr,
				kGlobalAnimTimerUpdateExpectedBytes,
				sizeof(kGlobalAnimTimerUpdateExpectedBytes)) &&
			ValidateDecodeBytes("global animation timer scene update",
				kGlobalAnimTimerSceneUpdateDecodeAddr,
				kGlobalAnimTimerSceneUpdateExpectedBytes,
				sizeof(kGlobalAnimTimerSceneUpdateExpectedBytes)) &&
			ValidateDecodeBytes("ActorAnimData restore clock write",
				kActorAnimRestoreClockWriteSite,
				kActorAnimRestoreClockWriteExpectedBytes,
				sizeof(kActorAnimRestoreClockWriteExpectedBytes)) &&
			ValidateDecodeBytes("ActorAnimData clock write",
				kActorAnimUpdateClockWriteSite,
				kActorAnimClockWriteExpectedBytes,
				sizeof(kActorAnimClockWriteExpectedBytes)) &&
			(actorAnimClockPatchState != kActorAnimClockPatchNeedsInstall ||
				ValidateDecodeBytes("ActorAnimData post-clock loop",
					kActorAnimUpdateClockPatchSite,
					kActorAnimClockPatchExpectedBytes,
					sizeof(kActorAnimClockPatchExpectedBytes))) &&
			ValidateDecodeBytes("BSAnimGroupSequence sample update",
				kBSAnimGroupSequenceSampleUpdateAddr,
				kBSAnimGroupSequenceSampleUpdateExpectedBytes,
				sizeof(kBSAnimGroupSequenceSampleUpdateExpectedBytes)) &&
			ValidateDecodeBytes("BSAnimGroupSequence save state",
				kBSAnimGroupSequenceSaveStateAddr,
				kBSAnimGroupSequenceSaveStateExpectedBytes,
				sizeof(kBSAnimGroupSequenceSaveStateExpectedBytes)) &&
			ValidateDecodeBytes("BSAnimGroupSequence load state",
				kBSAnimGroupSequenceLoadStateAddr,
				kBSAnimGroupSequenceLoadStateExpectedBytes,
				sizeof(kBSAnimGroupSequenceLoadStateExpectedBytes)) &&
			ValidateDecodeBytes("NiControllerSequence update",
				kNiControllerSequenceUpdateAddr,
				kNiControllerSequenceUpdateExpectedBytes,
				sizeof(kNiControllerSequenceUpdateExpectedBytes)) &&
			ValidateDecodeBytes("Actor adjacent process guard",
				kActorAdjacentProcessGuardDecodeAddr,
				kActorAdjacentProcessGuardExpectedBytes,
				sizeof(kActorAdjacentProcessGuardExpectedBytes)) &&
			ValidateDecodeBytes("Actor::IsTalking adjacent process guard",
				kActorIsTalkingAdjacentGuardDecodeAddr,
				kActorIsTalkingAdjacentGuardExpectedBytes,
				sizeof(kActorIsTalkingAdjacentGuardExpectedBytes));
	}

	static bool InstallPatches()
	{
		if (s_patchInstalled)
			return true;

		TexturePalettePatchState texturePaletteState = GetTexturePalettePatchState();
		if (texturePaletteState == kTexturePalettePatchMismatch)
			return false;

		GlobalAnimTimerLoadPatchState globalAnimTimerLoadState = GetGlobalAnimTimerLoadPatchState();
		if (globalAnimTimerLoadState == kGlobalAnimTimerLoadPatchMismatch)
			return false;

		RendererInitFailurePatchState rendererInitFailureState = GetRendererInitFailurePatchState();
		if (rendererInitFailureState == kRendererInitFailurePatchMismatch)
			return false;

		ActorGetAttackedPatchState actorGetAttackedState = GetActorGetAttackedPatchState();
		if (actorGetAttackedState == kActorGetAttackedPatchMismatch)
			return false;

		ActorIsTalkingPatchState actorIsTalkingState = GetActorIsTalkingPatchState();
		if (actorIsTalkingState == kActorIsTalkingPatchMismatch)
			return false;

		ActorAnimPreSamplePatchState actorAnimPreSampleState = GetActorAnimPreSamplePatchState();
		if (actorAnimPreSampleState == kActorAnimPreSamplePatchMismatch)
			return false;

		ActorAnimClockPatchState actorAnimClockPatchState = GetActorAnimClockPatchState();
		if (actorAnimClockPatchState == kActorAnimClockPatchMismatch)
			return false;

		TESWorldSpaceOFSTPatchState worldspaceOFSTPatchState = GetTESWorldSpaceOFSTPatchState();
		if (worldspaceOFSTPatchState == kTESWorldSpaceOFSTPatchMismatch)
			return false;

		if (!ValidateDecode(actorAnimClockPatchState, worldspaceOFSTPatchState))
			return false;

		if (!WriteRelJump(kMainThreadHandlePatchSite, (UInt32)&MainThreadHandleDuplicatePatch, sizeof(kMainThreadHandleExpectedBytes)))
		{
			Log("failed to write OSGlobals main thread handle patch at %08X", kMainThreadHandlePatchSite);
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

		if (!WriteRelJump(kTESFileDecompressedRecordSizePatchSite,
				(UInt32)&TESFileDecompressedRecordSizePatch,
				sizeof(kTESFileDecompressedRecordSizeExpectedBytes)) ||
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
			Log("failed to write TESFile compressed/seek guards at %08X/%08X/%08X/%08X/%08X/%08X",
				kTESFileDecompressedRecordSizePatchSite,
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

		if (globalAnimTimerLoadState == kGlobalAnimTimerLoadPatchNeedsInstall &&
			!WriteRelCall(kGlobalAnimTimerLoadPatchSite, (UInt32)&GlobalAnimTimerLoadPatch))
		{
			Log("failed to write global animation timer load patch at %08X", kGlobalAnimTimerLoadPatchSite);
			return false;
		}

		if (rendererInitFailureState == kRendererInitFailurePatchNeedsInstall &&
			!WriteRelJump(kRendererInitFailurePatchSite,
				(UInt32)&RendererInitFailurePatch,
				sizeof(kRendererInitFailureExpectedBytes)))
		{
			Log("failed to write renderer initialization failure guard at %08X", kRendererInitFailurePatchSite);
			return false;
		}

		if (actorGetAttackedState == kActorGetAttackedPatchNeedsInstall &&
			!WriteRelJump(kActorGetAttackedPatchSite,
				(UInt32)&ActorGetAttackedProcessGuardPatch,
				sizeof(kActorGetAttackedExpectedBytes)))
		{
			Log("failed to write Actor::GetAttacked process null guard at %08X", kActorGetAttackedPatchSite);
			return false;
		}

		if (actorIsTalkingState == kActorIsTalkingPatchNeedsInstall &&
			!WriteRelJump(kActorIsTalkingPatchSite,
				(UInt32)&ActorIsTalkingProcessGuardPatch,
				sizeof(kActorIsTalkingExpectedBytes)))
		{
			Log("failed to write Actor::IsTalking process null guard at %08X", kActorIsTalkingPatchSite);
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

		s_patchInstalled = true;
		Log("installed OSGlobals main thread handle duplicate fix at %08X", kMainThreadHandlePatchSite);
		if (texturePaletteState == kTexturePalettePatchNeedsInstall)
			Log("installed BSTexturePalette strrchr null-result guard at %08X", kTexturePaletteUnderscorePatchSite);
		else
			Log("accepted existing BSTexturePalette strrchr null-result guard at %08X", kTexturePaletteUnderscorePatchSite);
		Log("installed SaveLoad save-buffer range tracker at %08X and %08X",
			kSaveLoadCreateBufferPatchSite,
			kSaveLoadFreeBufferPatchSite);
		Log("installed SaveLoad raw blob guards at %08X/%08X/%08X/%08X",
			kSaveLoadRawBlobMap54PatchSite,
			kSaveLoadRawBlobMap58PatchSite,
			kSaveLoadRawBlobMap5CPatchSite,
			kSaveLoadRawBlobMap60PatchSite);
		Log("installed TESSpellList save count and list-node guards at %08X/%08X",
			kTESSpellListCountPatchSite,
			kTESSpellListNodeAllocPatchSite);
		Log("installed AVCollection save count guard at %08X", kAVCollectionCountPatchSite);
		Log("installed MiddleHighProcess FormID count and list-node guards at %08X/%08X",
			kMiddleHighProcessFormIDCountPatchSite,
			kMiddleHighProcessFormIDNodeAllocPatchSite);
		Log("installed KnownEffects signed count guard at %08X", kKnownEffectsCountPatchSite);
		Log("installed Actor relationship/disposition paired loop replacement at %08X/%08X",
			kActorRelationshipDispositionFirstPatchSite,
			kActorRelationshipDispositionSecondPatchSite);
		if (kInstallArchiveStreamingGuards)
		{
			Log("installed archive raw-read target guard at %08X", kArchiveRawReadTargetPatchSite);
			Log("installed archive raw-read count clamp at %08X", kArchiveRawReadCountClampPatchSite);
			Log("installed compressed archive zero-refill guard at %08X", kCompressedArchiveRefillPatchSite);
		}
		else
		{
			Log("skipped archive streaming guards at %08X/%08X/%08X after WER crash trace at 0042C7EB/0042C859",
				kArchiveRawReadTargetPatchSite,
				kArchiveRawReadCountClampPatchSite,
				kCompressedArchiveRefillPatchSite);
		}
		Log("installed TESFile_NextRecord advance guard at %08X", kTESFileNextRecordAdvancePatchSite);
		Log("installed TESFile_GetNextChunk advance guard at %08X", kTESFileGetNextChunkAdvancePatchSite);
		Log("installed TESFile compressed record size guard at %08X", kTESFileDecompressedRecordSizePatchSite);
		Log("installed TESFile compressed chunk-header tail guard at %08X", kTESFileLoadChunkHeaderCompressedTailPatchSite);
		Log("installed TESFile compressed chunk copy guards at %08X and %08X",
			kTESFileGetChunkDataTruncatedCopyPatchSite,
			kTESFileGetChunkDataFullCopyPatchSite);
		Log("installed TESFile JumpToRecord target guard at %08X", kTESFileJumpToRecordValidatePatchSite);
		Log("installed TESFile NextGroup short-GRUP guard at %08X", kTESFileNextGroupPatchSite);
		Log("installed TESForm record compression memory guards at %08X/%08X/%08X",
			kTESFormCompressRecordTempAllocPatchSite,
			kTESFormCompressRecordDeflateErrorCleanupPatchSite,
			kTESFormCompressRecordFinalAllocPatchSite);
		if (worldspaceOFSTPatchState == kTESWorldSpaceOFSTPatchNeedsInstall)
		{
			Log("installed TESWorldSpace WRLD OFST load guard at %08X", kTESWorldSpaceLoadOFSTPatchSite);
			Log("installed TESWorldSpace exterior cell offset lookup guard at %08X", kTESWorldSpaceFindCellOffsetPatchSite);
			Log("installed TESWorldSpace post-fixup rebuilt offset write guard at %08X", kTESWorldSpacePostFixupOffsetWritePatchSite);
			Log("installed TESWorldSpace post-fixup rebuilt offset tracking guard at %08X", kTESWorldSpacePostFixupOffsetCommitPatchSite);
			Log("installed TESWorldSpace post-fixup offset validation guard at %08X", kTESWorldSpacePostFixupOffsetReadPatchSite);
		}
		else
		{
			Log("accepted externally owned TESWorldSpace WRLD/OFST offset policy at %08X; skipped MEF WRLD/OFST guards",
				kTESWorldSpaceLoadOFSTPatchSite);
		}
		Log("installed loading texture GetTickCount wrap-safe wait at %08X", kLoadingTextureWaitPatchSite);
		Log("installed load-game start GetTickCount wrap-safe guard at %08X", kLoadGameStartTickPatchSite);
		Log("installed worldspace message GetTickCount wrap-safe delay at %08X", kWorldspaceMessageDelayPatchSite);
		if (globalAnimTimerLoadState == kGlobalAnimTimerLoadPatchNeedsInstall)
			Log("installed global animation timer load precision guard at %08X", kGlobalAnimTimerLoadPatchSite);
		else
			Log("accepted existing global animation timer load precision guard at %08X", kGlobalAnimTimerLoadPatchSite);
		if (rendererInitFailureState == kRendererInitFailurePatchNeedsInstall)
			Log("installed renderer initialization failure guard at %08X", kRendererInitFailurePatchSite);
		else
			Log("accepted existing renderer initialization failure guard at %08X", kRendererInitFailurePatchSite);
		if (actorGetAttackedState == kActorGetAttackedPatchNeedsInstall)
			Log("installed Actor::GetAttacked process null guard at %08X", kActorGetAttackedPatchSite);
		else
			Log("accepted existing Actor::GetAttacked process null guard at %08X", kActorGetAttackedPatchSite);
		if (actorIsTalkingState == kActorIsTalkingPatchNeedsInstall)
			Log("installed Actor::IsTalking process null guard at %08X", kActorIsTalkingPatchSite);
		else
			Log("accepted existing Actor::IsTalking process null guard at %08X", kActorIsTalkingPatchSite);
		if (actorAnimPreSampleState == kActorAnimPreSamplePatchNeedsInstall)
			Log("installed ActorAnimData pre-sample loop hook at %08X", kActorAnimUpdatePreSamplePatchSite);
		else
			Log("accepted existing ActorAnimData pre-sample loop hook at %08X", kActorAnimUpdatePreSamplePatchSite);
		if (actorAnimClockPatchState == kActorAnimClockPatchNeedsInstall)
			Log("installed ActorAnimData post-clock loop hook at %08X", kActorAnimUpdateClockPatchSite);
		else
			Log("accepted existing ActorAnimData post-clock loop hook at %08X", kActorAnimUpdateClockPatchSite);
		Log("installed ActorAnimData clock rebase hooks at %08X and %08X",
			kActorAnimUpdatePreSamplePatchSite,
			kActorAnimUpdateClockPatchSite);
		return true;
	}
}

extern "C"
{
	__declspec(dllexport) bool OBSEPlugin_Query(const OBSEInterface* obse, PluginInfo* info)
	{
		if (info)
		{
			info->infoVersion = kPluginInfoVersion;
			info->name = "Modern Engine Fixes";
			info->version = kPluginVersion;
		}

		Log("Modern Engine Fixes query");

		if (!obse)
		{
			Log("query failed: null OBSE interface");
			return false;
		}

		if (obse->isEditor)
		{
			Log("query failed: editor is not supported");
			return false;
		}

		if (obse->oblivionVersion != kOblivionVersion_1_2_0_416)
		{
			Log("query failed: Oblivion version %08X, expected %08X",
				obse->oblivionVersion,
				kOblivionVersion_1_2_0_416);
			return false;
		}

		return true;
	}

	__declspec(dllexport) bool OBSEPlugin_Load(const OBSEInterface* obse)
	{
		if (!obse || obse->isEditor || obse->oblivionVersion != kOblivionVersion_1_2_0_416)
			return false;

		return InstallPatches();
	}
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		s_module = instance;
		DisableThreadLibraryCalls(instance);
		OpenLog();
		Log("Modern Engine Fixes %u initializing", kPluginVersion);
		Log("static loading screen wait uses %.0f%% of Oblivion's configured duration",
			(double)(kStaticLoadingScreenWaitScale * 100.0f));
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		if (s_log != INVALID_HANDLE_VALUE)
		{
			Log("Modern Engine Fixes shutting down");
			CloseHandle(s_log);
			s_log = INVALID_HANDLE_VALUE;
		}
	}

	return TRUE;
}
