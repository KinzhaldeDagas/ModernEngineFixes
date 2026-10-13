	static void ClearSaveBufferRange(UInt8* owner, UInt32 base)
	{
		for (UInt32 i = 0; i < kSaveBufferRangeTrackingCapacity; i++)
		{
			if (s_saveBufferRanges[i].owner == owner &&
				(!base || s_saveBufferRanges[i].base == base))
			{
				if (s_saveBufferRanges[i].scopeOverflowDepth)
					continue;
				if (!s_saveBufferRanges[i].scopeTop && !s_saveBufferRanges[i].scopeOverflowDepth)
					s_saveBufferRanges[i].owner = 0;
				s_saveBufferRanges[i].base = 0;
				s_saveBufferRanges[i].end = 0;
				s_saveBufferRanges[i].size = 0;
			}
		}
	}

	static SaveBufferRangeTrackingEntry* FindActiveSaveBufferRange(UInt8* owner)
	{
		if (!owner)
			return NULL;
		for (UInt32 i = 0; i < kSaveBufferRangeTrackingCapacity; i++)
			if (s_saveBufferRanges[i].owner == owner)
				return &s_saveBufferRanges[i];
		return NULL;
	}

	static SaveBufferRangeTrackingEntry* GetSaveBufferOwnerSlot(UInt8* owner)
	{
		SaveBufferRangeTrackingEntry* entry = FindActiveSaveBufferRange(owner);
		if (entry || !owner)
			return entry;
		for (UInt32 i = 0; i < kSaveBufferRangeTrackingCapacity; i++)
		{
			if (!s_saveBufferRanges[i].owner)
			{
				s_saveBufferRanges[i].owner = owner;
				return &s_saveBufferRanges[i];
			}
		}
		return NULL;
	}

	static void TrackSaveBufferRange(UInt8* owner, UInt32 base, UInt32 size)
	{
		SaveBufferRangeTrackingEntry* entry = GetSaveBufferOwnerSlot(owner);
		if (!entry)
		{
			++s_saveBufferTrackingFailures;
			return;
		}
		if (entry->scopeOverflowDepth)
			return;
		entry->base = 0;
		entry->end = 0;
		entry->size = 0;
		if (!base)
			return;

		UInt32 end = base + size;
		if (end < base)
		{
			s_saveBufferTrackingFailures++;
			Log("refusing wrapped save buffer range owner=%p base=%08X size=%u",
				static_cast<void*>(owner),
				base,
				size);
			return;
		}

		entry->base = base;
		entry->end = end;
		entry->size = size;
		++s_trackedSaveBuffers;
	}

	static UInt32 GetSaveBufferRangeRemaining(const SaveBufferRangeTrackingEntry* range, UInt32 cursor)
	{
		if (!range || !range->base || range->scopeOverflowDepth ||
			cursor < range->base || cursor > range->end)
			return 0;
		return range->end - cursor;
	}

	static void __stdcall ActivateDeferredSaveBuffer(UInt8* owner, UInt8* buffer)
	{
		// Both engine producers allocate payloadLength+4 and write this header
		// before publishing the owned buffer into the deferred form map.
		const UInt32 size = buffer ? (UInt32)*(UInt16*)buffer + 4 : 0;
		TrackSaveBufferRange(owner, (UInt32)buffer, size);
		mef::interop::WriteSaveCursor(owner, static_cast<UInt8*>(buffer));
	}

	static void __stdcall BeginRawSaveBufferScope(UInt8* owner, UInt8* buffer)
	{
		SaveBufferRangeTrackingEntry* entry = GetSaveBufferOwnerSlot(owner);
		mef::interop::WriteSaveCursor(owner, static_cast<UInt8*>(buffer));
		if (!entry)
			return;
		if (entry->scopeOverflowDepth)
		{
			++entry->scopeOverflowDepth;
			return;
		}
		for (UInt32 i = 0; i < kSaveBufferRangeTrackingCapacity; ++i)
		{
			SaveBufferScopeTrackingEntry& scope = s_saveBufferScopes[i];
			if (scope.used)
				continue;
			scope.used = true;
			scope.parent = entry->scopeTop;
			scope.base = entry->base;
			scope.end = entry->end;
			scope.size = entry->size;
			entry->scopeTop = i + 1;
			// The raw-map producer owns a UInt16 length followed by that payload.
			TrackSaveBufferRange(owner, (UInt32)buffer,
				buffer ? (UInt32)*(UInt16*)buffer + 2 : 0);
			return;
		}
		// Keep the suspended parent intact. All guarded reads return zero until
		// the matching overflowed scopes have unwound to that parent.
		entry->scopeOverflowDepth = 1;
		++s_saveBufferTrackingFailures;
	}

	static void __stdcall EndRawSaveBufferScope(UInt8* owner, UInt32 restoredCursor)
	{
		mef::interop::WriteSaveCursor(owner, reinterpret_cast<UInt8*>(restoredCursor));
		SaveBufferRangeTrackingEntry* entry = FindActiveSaveBufferRange(owner);
		if (!entry)
			return;
		if (entry->scopeOverflowDepth)
		{
			--entry->scopeOverflowDepth;
			return;
		}
		if (!entry->scopeTop)
			return;
		SaveBufferScopeTrackingEntry& scope = s_saveBufferScopes[entry->scopeTop - 1];
		entry->base = scope.base;
		entry->end = scope.end;
		entry->size = scope.size;
		entry->scopeTop = scope.parent;
		scope.used = false;
		if (!entry->base && !entry->scopeTop)
			entry->owner = 0;
	}

	// Allocation is not read activation. These native producers write or queue
	// buffers; the owner can subsequently point at an unrelated private block.
	// Deferred buffers activate at 463876; the four raw readers have scoped entry.
	static const UInt32 kSaveBufferExpiredProducerReturn = 0x00461D52;
	static const UInt32 kSaveBufferUnloadProducerReturn = 0x00463CF7;
	static const UInt32 kSaveBufferImmediateReadReturn = 0x0046666E;
	static const UInt32 kSaveBufferDeferredProducerReturn = 0x004667B4;
	static const UInt32 kSaveBufferPlayerInitialProducerReturn = 0x006632EE;
	static bool IsInactiveSaveBufferProducer(UInt32 caller)
	{
		return caller == kSaveBufferExpiredProducerReturn || caller == kSaveBufferUnloadProducerReturn ||
			caller == kSaveBufferDeferredProducerReturn || caller == kSaveBufferPlayerInitialProducerReturn;
	}
	static void PublishAllocatedSaveBuffer(UInt8* owner, void* buffer, UInt32 size, UInt32 caller)
	{
		if (owner)
			mef::interop::WriteSaveCursor(owner, static_cast<UInt8*>(buffer));
		if (buffer)
		{
			// Preserve a genuinely active parent range during nested production.
			// Known writers do not install a new range or destroy the parent's.
			// The immediate LoadGame reader and unclassified callers retain the
			// previous allocation-tracking contract.
			if (!IsInactiveSaveBufferProducer(caller))
				TrackSaveBufferRange(owner, (UInt32)buffer, size);
		}
		else
			ClearSaveBufferRange(owner, 0);
	}
	static void* __cdecl CreateTrackedSaveBuffer(UInt8* owner, UInt32 size, UInt32 caller)
	{
		void* buffer = ((FormHeapAllocFn)kFormHeapAllocAddr)(size);
		PublishAllocatedSaveBuffer(owner, buffer, size, caller);
		if (!buffer)
			((PrintErrorFn)kPrintErrorAddr)("Could not create save buffer, out of memory.");
		return buffer;
	}

	static void __cdecl FreeTrackedSaveBuffer(UInt8* owner, void* buffer)
	{
		ClearSaveBufferRange(owner, (UInt32)buffer);
		((FormHeapFreeFn)kFormHeapFreeAddr)(buffer);
		if (owner)
			mef::interop::WriteSaveCursor(owner, nullptr);
	}

	static UInt32 GetSaveCursor(UInt8* owner)
	{
		// Integer addresses are confined to checked range arithmetic below.
		return owner ? reinterpret_cast<UInt32>(mef::interop::ReadSaveCursor(owner)) : 0;
	}

	static void SetSaveCursor(UInt8* owner, UInt32 cursor)
	{
		if (owner)
			mef::interop::WriteSaveCursor(owner, reinterpret_cast<UInt8*>(cursor));
	}

	static bool GetTrackedSaveBufferRemaining(UInt8* owner, UInt32* remainingOut)
	{
		if (!owner || !remainingOut)
			return false;

		UInt32 cursor = GetSaveCursor(owner);
		SaveBufferRangeTrackingEntry* range = FindActiveSaveBufferRange(owner);
		if (!range)
			return false;

		*remainingOut = GetSaveBufferRangeRemaining(range, cursor);
		return true;
	}

	static const UInt32 kSkyLoadTraceBegin = 0x005437C0;
	static const UInt32 kSkyLoadTraceEnd = 0x00543A00;
	static UInt32 s_skySaveReadTraceCount = 0;
	static UInt32 s_boundedReadClampTraceCount = 0;
	static void TraceBoundedSaveRead(UInt32 caller, UInt8* owner, void* destination,
		UInt32 cursor, UInt32 requested, UInt32 copied, const SaveBufferRangeTrackingEntry* range)
	{
		const bool sky = caller >= kSkyLoadTraceBegin && caller < kSkyLoadTraceEnd;
		if (copied == requested && !s_traceSaveReads) return;
		if (sky) { if (s_skySaveReadTraceCount++ >= 32) return; }
		else if (copied == requested || s_boundedReadClampTraceCount++ >= 16) return;
		UInt32 value = 0;
		if (destination && requested == sizeof(value)) std::memcpy(&value, destination, sizeof(value));
		Log("save-read trace sky=%u caller=%08X owner=%08X cursor=%08X requested=%u copied=%u tracked=%u range=[%08X,%08X) scope=%u overflow=%u valueBits=%08X",
			sky ? 1u : 0u, caller, (UInt32)owner, cursor, requested, copied, range ? 1u : 0u,
			range ? range->base : 0, range ? range->end : 0, range ? range->scopeTop : 0,
			range ? range->scopeOverflowDepth : 0, value);
	}
	static void* __cdecl LoadSaveDataBounded(UInt8* owner, void* destination, UInt32 requested, UInt32 caller = 0)
	{
		const UInt32 cursor = GetSaveCursor(owner);
		SaveBufferRangeTrackingEntry* range = FindActiveSaveBufferRange(owner);
		if (!range)
		{
			// SaveLoad_LoadData also serves unrelated callers with private buffers.
			// Preserve their vanilla contract; raw-blob and count helpers separately
			// reject unknown ranges. Engine-owned form/blob buffers activate above.
			std::memcpy(destination, (const void*)cursor, requested);
			SetSaveCursor(owner, cursor + requested);
			TraceBoundedSaveRead(caller, owner, destination, cursor, requested, requested, NULL);
			return destination;
		}
		const UInt32 remaining = GetSaveBufferRangeRemaining(range, cursor);
		const UInt32 copied = requested < remaining ? requested : remaining;
		if (destination)
		{
			if (copied)
				std::memcpy(destination, (const void*)cursor, copied);
			if (copied < requested)
				std::memset((UInt8*)destination + copied, 0, requested - copied);
		}
		UInt32 next = cursor + copied;
		if (cursor < range->base || cursor > range->end)
			next = range->end;
		SetSaveCursor(owner, next);
		TraceBoundedSaveRead(caller, owner, destination, cursor, requested, copied, range);
		return destination;
	}

	static bool WriteBytes(UInt32 address, const UInt8* replacement, UInt32 length)
	{
		return StageSelectedWrite(address, replacement, length);
	}

	static void DiscardRawSaveBlobPayload(UInt8* owner, UInt32 length, const char* label)
	{
		if (!owner || !length)
			return;

		UInt32 cursor = GetSaveCursor(owner);
		if (!cursor)
			return;

		UInt32 bytesToAdvance = length;
		SaveBufferRangeTrackingEntry* range = FindActiveSaveBufferRange(owner);
		if (range)
		{
			UInt32 remaining = GetSaveBufferRangeRemaining(range, cursor);
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
				Log("skipping %s raw save blob without an active range; cursor unchanged owner=%08X cursor=%08X length=%u",
					label,
					(UInt32)owner,
					cursor,
					length);
			}
			return;
		}

		UInt32 next = cursor + bytesToAdvance;
		if (cursor < range->base || cursor > range->end)
			next = range->end;

		SetSaveCursor(owner, next);
	}

	static void ClampUInt16SaveCountForOwner(UInt8* owner, UInt16* count,
		UInt32 minEntryBytes, const char* label)
	{
		if (!count || !minEntryBytes)
			return;

		UInt32 remaining = 0;
		if (!GetTrackedSaveBufferRemaining(owner, &remaining))
		{
			s_untrackedSaveRawBlobCopies++;
			if (s_untrackedSaveRawBlobCopies <= 16)
			{
				Log("clearing %s save count because no active range exists owner=%08X",
					label,
					(UInt32)owner);
			}
			*count = 0;
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

	static void ClampUInt16SaveCount(UInt16* count, UInt32 minEntryBytes, const char* label)
	{
		ClampUInt16SaveCountForOwner(*(UInt8**)0x00B33B00, count, minEntryBytes, label);
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

	static void ClampKnownEffectsSaveCountForOwner(UInt8* owner, SInt32* count)
	{
		if (!count || *count <= 0)
			return;

		UInt32 remaining = 0;
		if (!GetTrackedSaveBufferRemaining(owner, &remaining))
		{
			s_untrackedSaveRawBlobCopies++;
			if (s_untrackedSaveRawBlobCopies <= 16)
			{
				Log("clearing KnownEffects save count because no active range exists owner=%08X",
					(UInt32)owner);
			}
			*count = 0;
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

	static void __cdecl ClampKnownEffectsSaveCount(SInt32* count)
	{
		ClampKnownEffectsSaveCountForOwner(*(UInt8**)0x00B33B00, count);
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

	struct RawSaveBlobMapNode
	{
		RawSaveBlobMapNode* next;
		UInt32 key;
		void* value;
	};

	struct RawSaveBlobMap
	{
		void** vtable;
		UInt32 bucketCount;
		RawSaveBlobMapNode** buckets;
		UInt32 count;
	};

	typedef UInt32 (__thiscall* SaveBlobMapHashFn)(RawSaveBlobMap*, UInt32);
	typedef char (__thiscall* SaveBlobMapEqualFn)(RawSaveBlobMap*, UInt32, UInt32);
	typedef void (__thiscall* SaveBlobMapSetFn)(RawSaveBlobMap*, RawSaveBlobMapNode*, UInt32, void*);
	typedef void (__thiscall* SaveBlobMapClearFn)(RawSaveBlobMap*, RawSaveBlobMapNode*);
	typedef RawSaveBlobMapNode* (__thiscall* SaveBlobMapAllocateFn)(RawSaveBlobMap*);

	#include "perf_actor_maps.h"

	static bool TryStoreRawSaveBlobMap(RawSaveBlobMap* map, UInt32 key,
		bool zeroKeyWhenExisting, void* blob)
	{
		bool indexedResult;
		if (PerfTryStoreSaveMap(map, key, zeroKeyWhenExisting, blob, &indexedResult)) return indexedResult;
		if (!map || !map->vtable || !map->bucketCount || !map->buckets)
			return false;
		SaveBlobMapHashFn hash = (SaveBlobMapHashFn)map->vtable[1];
		SaveBlobMapEqualFn equal = (SaveBlobMapEqualFn)map->vtable[2];
		SaveBlobMapSetFn set = (SaveBlobMapSetFn)map->vtable[3];
		SaveBlobMapClearFn clear = (SaveBlobMapClearFn)map->vtable[4];
		SaveBlobMapAllocateFn allocate = (SaveBlobMapAllocateFn)map->vtable[5];
		UInt32 bucket = hash(map, key);
		if (bucket >= map->bucketCount)
			return false;
		RawSaveBlobMapNode* entry = map->buckets[bucket];
		while (entry && !equal(map, key, entry->key))
			entry = entry->next;
		if (entry && zeroKeyWhenExisting)
		{
			key = 0;
			bucket = hash(map, key);
			if (bucket >= map->bucketCount)
				return false;
			entry = map->buckets[bucket];
			while (entry && !equal(map, key, entry->key))
				entry = entry->next;
		}
		if (entry)
		{
			// Replay the engine's virtual cleanup/assignment policy on replacement.
			clear(map, entry);
			set(map, entry, key, blob);
			return true;
		}
		entry = allocate(map);
		if (!entry)
			return false;
		// No map mutation or ownership transfer occurs until a node exists.
		set(map, entry, key, blob);
		entry->next = map->buckets[bucket];
		map->buckets[bucket] = entry;
		++map->count;
		return true;
	}

	static int __cdecl StoreBoundedRawSaveBlob(UInt8* owner,
		UInt32 keyObject,
		UInt32 length,
		UInt32 mapOffset,
		UInt32 zeroKeyWhenExisting,
		const char* label,
		FormHeapAllocFn allocate = (FormHeapAllocFn)kFormHeapAllocAddr,
		FormHeapFreeFn release = (FormHeapFreeFn)kFormHeapFreeAddr)
	{
		if (!owner)
			return 0;

		if (!keyObject)
		{
			DiscardRawSaveBlobPayload(owner, length, label);
			return 0;
		}

		RawSaveBlobMap* map = *(RawSaveBlobMap**)(owner + mapOffset);
		if (!map)
		{
			DiscardRawSaveBlobPayload(owner, length, label);
			return 0;
		}

		UInt32 key = *(UInt32*)(keyObject + 0x0C);

		UInt32 cursor = GetSaveCursor(owner);
		UInt32 bytesToCopy = length;
		UInt32 bytesToAdvance = length;
		SaveBufferRangeTrackingEntry* range = FindActiveSaveBufferRange(owner);
		if (range)
		{
			UInt32 remaining = GetSaveBufferRangeRemaining(range, cursor);
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
				Log("skipping unsupported %s raw save blob without an active range; cursor unchanged owner=%08X cursor=%08X length=%u",
					label,
					(UInt32)owner,
					cursor,
					length);
			}
			return 0;
		}

		if (bytesToCopy && !CanReadMemory(cursor, bytesToCopy))
		{
			bytesToAdvance = GetSaveBufferRangeRemaining(range, cursor);
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

		// The four naked entries zero-extend a UInt16. Keep this contract explicit
		// here so helpers cannot accidentally wrap allocationSize if reused.
		if (length > 0xFFFF)
			return 0;
		UInt32 nextCursor = cursor + bytesToAdvance;
		if (cursor < range->base || cursor > range->end)
			nextCursor = range->end;
		UInt32 allocationSize = length + 2;
		UInt8* blob = (UInt8*)allocate(allocationSize);
		if (!blob)
		{
			SetSaveCursor(owner, nextCursor);
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

		SetSaveCursor(owner, nextCursor);
		if (!TryStoreRawSaveBlobMap(map, key, zeroKeyWhenExisting != 0, blob))
		{
			release(blob);
			++s_failedSaveRawBlobAllocations;
			if (s_failedSaveRawBlobAllocations <= 16)
				Log("raw save blob map insertion failed for %s; released unowned payload", label);
			return 0;
		}
		return 1;
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
			push	dword ptr [esp] // native allocation caller distinguishes producers from readers
			push	dword ptr [esp + 8]
			push	ecx
			call	CreateTrackedSaveBuffer
			add		esp, 0Ch
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

	static __declspec(naked) void SaveLoadBoundedLoadDataPatch()
	{
		__asm
		{
			push dword ptr [esp] // original engine return address, for bounded diagnostics
			push dword ptr [esp + 0Ch]
			push dword ptr [esp + 0Ch]
			push ecx
			call LoadSaveDataBounded
			add esp, 10h
			ret 8
		}
	}

	static __declspec(naked) void SaveLoadActivateDeferredBufferPatch()
	{
		__asm
		{
			pushfd
			pushad
			push ebx
			push esi
			call ActivateDeferredSaveBuffer
			popad
			popfd
			mov ecx, dword ptr ds:[00B33B00h]
			push 00463876h
			ret
		}
	}

	static __declspec(naked) void SaveLoadAnimationBufferBeginPatch()
	{
		__asm
		{
			pushfd
			pushad
			push ebp
			push edi
			call BeginRawSaveBufferScope
			popad
			popfd
			mov ecx, dword ptr ds:[00B33B00h]
			push 00458F8Ch
			ret
		}
	}

	static __declspec(naked) void SaveLoadAnimationBufferEndPatch()
	{
		__asm
		{
			mov ecx, [esp + 18h]
			pushfd
			pushad
			push ecx
			push edi
			call EndRawSaveBufferScope
			popad
			popfd
			// ZF from the original test esi,esi selects the animation tail.
			push 00458FD5h
			ret
		}
	}

	static __declspec(naked) void SaveLoadAttachedAnimationBufferBeginPatch()
	{
		__asm
		{
			pushfd
			pushad
			push edi
			push esi
			call BeginRawSaveBufferScope
			popad
			popfd
			mov ecx, dword ptr ds:[00B33B00h]
			push 00459096h
			ret
		}
	}

	static __declspec(naked) void SaveLoadAttachedAnimationBufferEndPatch()
	{
		__asm
		{
			pushfd
			pushad
			push ebp
			push esi
			call EndRawSaveBufferScope
			popad
			popfd
			pop esi
			pop ebp
			push 004590D1h
			ret
		}
	}

	static __declspec(naked) void SaveLoadCharacterBufferBeginPatch()
	{
		__asm
		{
			mov [esp + 28h], ecx
			pushfd
			pushad
			push edx
			push edi
			call BeginRawSaveBufferScope
			popad
			popfd
			push 004591FAh
			ret
		}
	}

	static __declspec(naked) void SaveLoadCharacterBufferEndPatch()
	{
		__asm
		{
			mov ecx, [esp + 28h]
			pushfd
			pushad
			push ecx
			push edi
			call EndRawSaveBufferScope
			popad
			popfd
			// Other paths enter 4592C1 directly; do not overwrite that target.
			push 004592C1h
			ret
		}
	}

	static __declspec(naked) void SaveLoadHavokBufferBeginPatch()
	{
		__asm
		{
			pushfd
			pushad
			push edi
			push esi
			call BeginRawSaveBufferScope
			popad
			popfd
			mov ecx, dword ptr ds:[00B33B00h]
			push 00459386h
			ret
		}
	}

	static __declspec(naked) void SaveLoadHavokBufferEndPatch()
	{
		__asm
		{
			pushfd
			pushad
			push ebp
			push esi
			call EndRawSaveBufferScope
			popad
			popfd
			pop esi
			pop ebp
			push 004593C9h
			ret
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

	static __declspec(naked) void EffectSettingFilteredListHeadAllocPatch()
	{
		__asm
		{
			push	dword ptr [esp + 4]
			mov		eax, 00401F00h
			call	eax
			add		esp, 4
			test	eax, eax
			jnz		allocationSucceeded

			// The head has not been published and EDI has not yet been saved.
			// Discard the pending size, restore ESI/EBP through the clean epilogue,
			// and return a null list head through EBP.
			xor		ebp, ebp
			mov		dword ptr [esp], 004171C5h
			ret		4

		allocationSucceeded:
			ret
		}
	}

	static __declspec(naked) void SaveLoadExpiredCellEntryAllocPatch()
	{
		__asm
		{
			push	dword ptr [esp + 4]
			mov		eax, 00401F00h
			call	eax
			add		esp, 4
			test	eax, eax
			jnz		allocationSucceeded

			mov		dword ptr [esp], 00461A0Dh
			ret		4

		allocationSucceeded:
			ret
		}
	}

	static __declspec(naked) void SaveLoadExpiredCellListAllocPatch()
	{
		__asm
		{
			push	dword ptr [esp + 4]
			mov		eax, 00401F00h
			call	eax
			add		esp, 4
			test	eax, eax
			jnz		allocationSucceeded

			// At replacement-CALL entry the already allocated coordinate pair is
			// the vanilla local [ESP+2Ch], shifted by return+size to [ESP+34h].
			push	dword ptr [esp + 34h]
			mov		eax, 00401F20h
			call	eax
			add		esp, 4
			mov		dword ptr [esp], 00461A0Dh
			ret		4

		allocationSucceeded:
			ret
		}
	}

	static __declspec(naked) void ContainerExtraDataListNodeAllocPatch()
	{
		__asm
		{
			cmp		eax, ebp
			jz		allocationFailed

			mov		ecx, [esi]
			push	0049390Eh
			ret

		allocationFailed:
			push	0049392Fh
			ret
		}
	}

	static __declspec(naked) void ExtraDataRegionListBufferAllocPatch()
	{
		__asm
		{
			push	dword ptr [esp + 4]
			mov		eax, 00401F00h
			call	eax
			add		esp, 4
			test	eax, eax
			jnz		allocationSucceeded

			// The XCLR region-list object was constructed with mode 0 and is still
			// empty here, so direct FormHeapFree is its complete failure cleanup.
			push	dword ptr [esp + 44h]
			mov		eax, 00401F20h
			call	eax
			add		esp, 4
			mov		dword ptr [esp], 004260C1h
			xor		eax, eax
			ret		4

		allocationSucceeded:
			ret
		}
	}

	static __declspec(naked) void ExtraDataLockDataAllocPatch()
	{
		__asm
		{
			test	eax, eax
			jz		allocationFailed
			mov		[eax + 4], esi
			push	00425AABh
			ret

		allocationFailed:
			push	00426251h
			ret
		}
	}

	static __declspec(naked) void ExtraDataLockWrapperAllocPatch()
	{
		__asm
		{
			test	eax, eax
			jz		allocationFailed
			mov		dword ptr [esp + 34h], 0
			push	00425ACCh
			ret

		allocationFailed:
			push	esi
			mov		eax, 00401F20h
			call	eax
			add		esp, 4
			push	00426251h
			ret
		}
	}

	static __declspec(naked) void ExtraDataMapMarkerAllocPatch()
	{
		__asm
		{
			test	eax, eax
			jz		allocationFailed
			mov		ecx, eax
			mov		eax, 0042B3F0h
			call	eax
			push	00425DC1h
			ret

		allocationFailed:
			mov		dword ptr [esp + 34h], 0FFFFFFFFh
			push	00426251h
			ret
		}
	}

	static __declspec(naked) void ExtraDataTeleportAllocPatch()
	{
		__asm
		{
			test	eax, eax
			jz		allocationFailed
			mov		ecx, eax
			mov		eax, 0042B590h
			call	eax
			push	00425EA5h
			ret

		allocationFailed:
			mov		dword ptr [esp + 34h], 0FFFFFFFFh
			push	00426251h
			ret
		}
	}

	static __declspec(naked) void ExtraDataRagdollAllocPatch()
	{
		__asm
		{
			test	eax, eax
			jz		allocationFailed
			mov		ecx, eax
			mov		eax, 0042A300h
			call	eax
			push	00425C7Eh
			ret

		allocationFailed:
			mov		dword ptr [esp + 34h], 0FFFFFFFFh
			push	00426251h
			ret
		}
	}

	static __declspec(naked) void ExtraDataRagdollPayloadAllocPatch()
	{
		__asm
		{
			test	eax, eax
			jz		allocationFailed
			mov		ecx, eax
			mov		eax, 00497210h
			call	eax
			push	00425CADh
			ret

		allocationFailed:
			push	esi
			mov		eax, 00401F20h
			call	eax
			add		esp, 4
			mov		dword ptr [esp + 34h], 0FFFFFFFFh
			push	00426251h
			ret
		}
	}

	static __declspec(naked) void ExtraDataRegionListAllocPatch()
	{
		__asm
		{
			test	eax, eax
			jz		allocationFailed
			push	0
			mov		ecx, eax
			mov		eax, 004A62D0h
			call	eax
			push	0042602Ah
			ret

		allocationFailed:
			mov		dword ptr [esp + 34h], 0FFFFFFFFh
			push	00426251h
			ret
		}
	}

	static UInt8* __cdecl AllocateSentinelBuffer(UInt32 size)
	{
		if (size == 0xFFFFFFFF)
			return NULL;

		UInt8* const buffer = (UInt8*)((FormHeapAllocFn)kFormHeapAllocAddr)(size + 1);
		if (buffer)
			buffer[size] = 0;
		return buffer;
	}

	static UInt8* __cdecl AllocateDoubleSentinelBuffer(UInt32 size)
	{
		if (size >= 0xFFFFFFFE)
			return NULL;

		UInt8* const buffer = (UInt8*)((FormHeapAllocFn)kFormHeapAllocAddr)(size + 2);
		if (buffer)
		{
			buffer[size] = 0;
			buffer[size + 1] = 0;
		}
		return buffer;
	}

	static __declspec(naked) void AnimationChunkBufferAllocPatch()
	{
		__asm
		{
			push	dword ptr [esp + 4]
			call	AllocateDoubleSentinelBuffer
			add		esp, 4
			test	eax, eax
			jnz		allocationSucceeded

			mov		dword ptr [esp], 00468940h
			ret		4

		allocationSucceeded:
			ret
		}
	}

	static __declspec(naked) void TESCreatureNIFZBufferAllocPatch()
	{
		__asm
		{
			push	dword ptr [esp + 4]
			call	AllocateDoubleSentinelBuffer
			add		esp, 4
			test	eax, eax
			jnz		allocationSucceeded

			// Leave the saved EBX at the top of the creature-loader frame and
			// enter the register-restoring epilogue after its buffer-free call.
			mov		dword ptr [esp], 0046E05Eh
			ret		4

		allocationSucceeded:
			ret
		}
	}

	static __declspec(naked) void ScriptEventListOptionalDataAllocPatch()
	{
		__asm
		{
			add		esp, 4
			test	eax, eax
			jz		allocationFailed

			push	8
			mov		[edi + 10h], eax
			push	004FB91Ch
			ret

		allocationFailed:
			mov		dword ptr [edi + 10h], 0
			sub		esp, 8
			push	8
			lea		eax, [esp + 4]
			push	eax
			mov		ecx, dword ptr ds:[00B33B00h]
			mov		eax, 004534D0h
			call	eax
			add		esp, 8
			push	004FB928h
			ret
		}
	}

	#include "performance_fixes.h"
	#include "perf_archive_model.h"
	#include "perf_array_sort.h"
	#include "perf_light_lists.h"
	#include "perf_property_order.h"
	#include "perf_light_count.h"
	#include "save_read_preflight.h"
	#include "save_read_runtime.h"
	#include "perf_save_queue.h"
	#include "perf_save_exterior.h"
	#include "perf_runtime.h"
