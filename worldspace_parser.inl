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
			// Both the empty-record path and the completed enumeration arrive
			// here with EBX holding the table. The latter reloads EBX at 4F30A9.
			push	ebx
			push	esi
			call	TrackRebuiltWorldspaceOFST
			mov		[esi + 0A8h], ebx
			jmp		dword ptr [kTESWorldSpacePostFixupOffsetCommitContinueSite]
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

	static bool __stdcall ValidateTESFileCompressedInputBounds(UInt8* file)
	{
		if (!file)
			return false;

		const UInt32 length = *(UInt32*)(file + 0x240);
		const UInt32 fileSize = *(UInt32*)(file + 0x258);
		const UInt32 recordOffset = *(UInt32*)(file + 0x25C);
		// The stream is positioned after the 20-byte record header. Subtraction
		// proves both the input span and the allocator's subsequent length + 1.
		if (length >= sizeof(UInt32) && recordOffset <= fileSize &&
			fileSize - recordOffset >= 0x14 && length <= fileSize - recordOffset - 0x14)
			return true;

		++s_rejectedTESFileCompressedRecords;
		if (s_rejectedTESFileCompressedRecords <= 16)
		{
			Log("rejected compressed TES record input file=%08X offset=%u length=%u fileSize=%u",
				(UInt32)file, recordOffset, length, fileSize);
		}
		return false;
	}

	static bool __stdcall ValidateTESFileInflatedSize(UInt8* file, UInt32 actualLength)
	{
		if (!file)
			return false;

		const UInt32 advertisedLength = *(UInt32*)(file + 0x418);
		if (*(UInt8**)(file + 0x414) && advertisedLength && actualLength == advertisedLength)
			return true;

		// A completed zlib stream can produce fewer bytes than the record's
		// advertised length. Reject it before parsers can read the unwritten tail.
		// The caller still owns both buffers and routes to its normal cleanup.
		++s_rejectedTESFileCompressedRecords;
		if (s_rejectedTESFileCompressedRecords <= 16)
		{
			Log("rejected compressed TES record output file=%08X advertised=%u actual=%u",
				(UInt32)file, advertisedLength, actualLength);
		}
		return false;
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

	static __declspec(naked) void TESFileCompressedInputBoundsPatch()
	{
		__asm
		{
			push	esi
			call	ValidateTESFileCompressedInputBounds
			test	al, al
			jz		invalid

			mov		eax, [esi + 240h]
			jmp		dword ptr [kTESFileCompressedInputContinueSite]

		invalid:
			// Only ESI is saved at this point; no buffer has been allocated.
			jmp		dword ptr [kTESFileCompressedInputFailSite]
		}
	}

	static __declspec(naked) void TESFileInflatedSizePatch()
	{
		__asm
		{
			// z_stream starts at original ESP+10h; total_out is field +14h.
			push	dword ptr [esp + 24h]
			push	esi
			call	ValidateTESFileInflatedSize
			test	al, al
			jz		invalid

			lea		edx, [esp + 10h]
			push	edx
			jmp		dword ptr [kTESFileInflatedSizeContinueSite]

		invalid:
			// This path calls inflateEnd, then clears output and frees input.
			jmp		dword ptr [kTESFileInflatedSizeFailSite]
		}
	}

	static __declspec(naked) void TESWorldSpaceRebuildAllocPatch()
	{
		__asm
		{
			test	eax, eax
			jz		failed

			mov		ebx, eax
			lea		eax, [ebp * 4]
			jmp		dword ptr [kTESWorldSpaceRebuildAllocContinueSite]

		failed:
			// Three PrintError arguments and one allocation size are still
			// pending. No table exists or has been published on this branch.
			add		esp, 10h
			inc		s_rejectedWorldspaceOFSTTables
			jmp		dword ptr [kTESWorldSpaceRebuildAllocFailSite]
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

	static __declspec(naked) void TESFileDecompressedRecordAllocPatch()
	{
		__asm
		{
			// Replay the cdecl allocation while preserving the original pending
			// size argument for the vanilla zlib setup's add esp, 10h.
			push	dword ptr [esp + 4]
			mov		eax, 0040FD70h
			call	eax
			add		esp, 4
			test	eax, eax
			jnz		allocationSucceeded

			// No decompressed buffer has been stored yet. Discard the pending
			// allocation size and reuse the existing TESFile cleanup path.
			mov		ecx, esi
			mov		dword ptr [esp], 004506F6h
			ret		4

		allocationSucceeded:
			ret
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
