	typedef UInt32 (__thiscall* ArchiveReadBytesFn)(UInt8*, void*, UInt32);

	// The constructor is still private. Earlier folder/file metadata must already
	// be usable; this helper does not validate or repair those preceding stages.
	static bool TryBuildCachedArchiveNames(UInt8* archive,
		FormHeapAllocFn allocate = (FormHeapAllocFn)kFormHeapAllocAddr,
		FormHeapFreeFn release = (FormHeapFreeFn)kFormHeapFreeAddr,
		ArchiveReadBytesFn read = (ArchiveReadBytesFn)kArchiveReadBytesAddr)
	{
		if (!archive || *(void**)(archive + 0x1A0) || *(void**)(archive + 0x1A4) ||
			(archive[0x194] & 0x24))
			return false;
		const UInt32 folderCount = *(UInt32*)(archive + 0x164);
		const UInt32 byteCount = *(UInt32*)(archive + 0x170);
		const UInt8* folders = *(UInt8**)(archive + 0x178);
		if (folderCount > 0xFFFFFFFFu / 16 || (folderCount && !folders))
			return false;
		for (UInt32 i = 0; i < folderCount; ++i)
			if (*(const UInt32*)(folders + i * 16 + 8) > 0xFFFFFFFFu / sizeof(UInt32))
				return false;

		UInt8* buffer = NULL;
		UInt32** offsets = NULL;
		UInt32 builtFolders = 0;
		bool complete = false;
		do
		{
			if (byteCount)
			{
				buffer = (UInt8*)allocate(byteCount);
				if (!buffer || read(archive, buffer, byteCount) != byteCount)
					break;
			}
			if (folderCount)
			{
				offsets = (UInt32**)allocate(folderCount * sizeof(UInt32*));
				if (!offsets)
					break;
			}
			UInt32 cursor = 0;
			bool scanComplete = true;
			for (; builtFolders < folderCount; ++builtFolders)
			{
				const UInt32 count = *(const UInt32*)(folders + builtFolders * 16 + 8);
				// Each entry requires at least its terminator. Check before allocation.
				if (count > byteCount - cursor)
					break;
				UInt32* table = count ? (UInt32*)allocate(count * sizeof(UInt32)) : NULL;
				if (count && !table)
					break;
				offsets[builtFolders] = table;
				UInt32 entry = 0;
				for (; entry < count; ++entry)
				{
					const UInt8* terminator = (const UInt8*)std::memchr(buffer + cursor, 0, byteCount - cursor);
					if (!terminator)
						break;
					table[entry] = cursor;
					cursor = (UInt32)(terminator - buffer) + 1;
				}
				if (entry != count)
				{
					scanComplete = false;
					// Include this allocated table in the initialized cleanup prefix.
					++builtFolders;
					break;
				}
			}
			// builtFolders alone is not a success result: the final folder can fail
			// after its allocation, so validate the completed scan independently.
			if (builtFolders != folderCount || !scanComplete)
				break;
			complete = true;
		} while (false);
		if (!complete)
		{
			for (UInt32 i = 0; i < builtFolders; ++i)
				if (offsets[i]) release(offsets[i]);
			if (offsets) release(offsets);
			if (buffer) release(buffer);
			return false;
		}
		*(UInt8**)(archive + 0x1A0) = buffer;
		*(UInt32***)(archive + 0x1A4) = offsets;
		archive[0x194] |= 0x20;
		return true;
	}

	struct ArchiveRegistrationNode
	{
		UInt8* archive;
		ArchiveRegistrationNode* next;
	};
	static_assert(sizeof(ArchiveRegistrationNode) == 8, "engine list node ABI");

	static ArchiveRegistrationNode* ArchiveRegistrationTail(ArchiveRegistrationNode* node)
	{
		while (node && node->next) node = node->next;
		return node;
	}

	static bool TryRegisterArchive(UInt8* archive, ArchiveRegistrationNode** list,
		FormHeapAllocFn allocate = (FormHeapAllocFn)kFormHeapAllocAddr,
		FormHeapFreeFn release = (FormHeapFreeFn)kFormHeapFreeAddr)
	{
		if (!archive || !list) return false;
		ArchiveRegistrationNode* tail = ArchiveRegistrationTail(*list);
		if (tail && !tail->archive)
		{
			tail->archive = archive;
			return true;
		}
		ArchiveRegistrationNode* prepared = (ArchiveRegistrationNode*)allocate(sizeof(ArchiveRegistrationNode));
		// Allocation recovery can mutate the global list. Never reuse the old tail.
		tail = ArchiveRegistrationTail(*list);
		if (tail && !tail->archive)
		{
			tail->archive = archive;
			if (prepared) release(prepared);
			return true;
		}
		if (!prepared) return false;
		prepared->archive = archive;
		prepared->next = NULL;
		if (tail) tail->next = prepared;
		else *list = prepared;
		return true;
	}

	static bool __stdcall BuildCachedArchiveNames(UInt8* archive)
	{
		return TryBuildCachedArchiveNames(archive);
	}
	static bool __stdcall RegisterArchive(UInt8* archive)
	{
		return TryRegisterArchive(archive, (ArchiveRegistrationNode**)kArchiveRegistrationListAddr);
	}
	static __declspec(naked) void ArchiveCachedNamesBuildPatch()
	{
		__asm {
			pushad
			push esi
			call BuildCachedArchiveNames
			test al, al
			popad
			jz rejected
			push kArchiveCachedNamesBuildContinueSite
			ret
		rejected:
			push kArchiveCachedNamesInvalidSite
			ret
		}
	}
	static __declspec(naked) void ArchiveRegistrationPatch()
	{
		__asm {
			pushad
			push esi
			call RegisterArchive
			test al, al
			popad
			jz rejected
			push kArchiveRegistrationContinueSite
			ret
		rejected:
			push kArchiveRegistrationFailureSite
			ret
		}
	}

	static char s_emptyArchiveFolderName[] = "";

	static __declspec(naked) void ArchiveFolderNameAllocPatch()
	{
		__asm
		{
			push	dword ptr [esp + 4]
			call	AllocateSentinelBuffer
			add		esp, 4
			test	eax, eax
			jnz		allocationSucceeded

			mov		eax, offset s_emptyArchiveFolderName
			mov		dword ptr [esp], 0042CBEEh
			ret		4

		allocationSucceeded:
			ret
		}
	}

	static __declspec(naked) void BSFileWriteSwappedAllocPatch()
	{
		__asm
		{
			push	dword ptr [esp + 4]
			mov		eax, 00401F00h
			call	eax
			add		esp, 4
			test	eax, eax
			jnz		allocationSucceeded

			pop		ecx
			add		esp, 4
			pop		esi
			xor		eax, eax
			pop		edi
			ret

		allocationSucceeded:
			ret
		}
	}

	static __declspec(naked) void BSFileOpenBufferAllocPatch()
	{
		__asm
		{
			add		esp, 4
			test	eax, eax
			jz		allocationFailed

			test	bl, bl
			push	0042FF54h
			ret

		allocationFailed:
			mov		[esi + 18h], eax
			mov		byte ptr [esi + 24h], 0
			push	0042FF74h
			ret
		}
	}

	static __declspec(naked) void ModelWildcardPathAllocPatch()
	{
		__asm
		{
			mov		ecx, [esp + 4]
			cmp		ecx, edi
			jbe		allocationFailed

			push	ecx
			mov		eax, 00401F00h
			call	eax
			add		esp, 4
			test	eax, eax
			jz		allocationFailed
			ret

		allocationFailed:
			mov		dword ptr [esp], 00431AD8h
			ret		4
		}
	}

	static __declspec(naked) void KFListHeadAllocPatch()
	{
		__asm
		{
			push	dword ptr [esp + 4]
			mov		eax, 00401F00h
			call	eax
			add		esp, 4
			test	eax, eax
			jnz		allocationSucceeded

			xor		ebp, ebp
			mov		dword ptr [esp], 00435920h
			ret		4

		allocationSucceeded:
			ret
		}
	}

	static __declspec(naked) void KFListStringAllocPatch()
	{
		__asm
		{
			push	dword ptr [esp + 4]
			mov		eax, 00401F00h
			call	eax
			add		esp, 4
			test	eax, eax
			jnz		allocationSucceeded

			mov		dword ptr [esp], 00435911h
			ret		4

		allocationSucceeded:
			ret
		}
	}

	static __declspec(naked) void KFListNodeAllocPatch()
	{
		__asm
		{
			test	eax, eax
			jz		allocationFailed
			mov		ecx, [ebp]
			push	004358F8h
			ret

		allocationFailed:
			push	edi
			mov		eax, 00401F20h
			call	eax
			add		esp, 4
			push	00435911h
			ret
		}
	}

	static __declspec(naked) void TESFileUpdateGroupsAllocPatch()
	{
		__asm
		{
			push	dword ptr [esp + 4]
			mov		eax, 00401F00h
			call	eax
			add		esp, 4
			test	eax, eax
			jnz		allocationSucceeded

			mov		dword ptr [esp], 00450427h
			ret		4

		allocationSucceeded:
			ret
		}
	}

	static __declspec(naked) void TESFileLoadedMastersAllocPatch()
	{
		__asm
		{
			push	dword ptr [esp + 4]
			mov		eax, 00401F00h
			call	eax
			add		esp, 4
			test	eax, eax
			jnz		allocationSucceeded

			pop		ecx
			add		esp, 4
			pop		edi
			pop		esi
			pop		ebp
			add		esp, 4
			xor		eax, eax
			ret		8

		allocationSucceeded:
			ret
		}
	}

	static __declspec(naked) void ArchiveMasterListAllocPatch()
	{
		__asm
		{
			test	eax, eax
			jz		allocationFailed

			mov		ecx, dword ptr ds:[00B04458h]
			push	0042F73Ah
			ret

		allocationFailed:
			mov		byte ptr [esp + 13h], 0
			push	0042F9BEh
			ret
		}
	}

	static bool __cdecl AppendArchiveNameBounded(char* destination, const char* name)
	{
		if (!destination || !name)
			return false;

		UInt32 destinationLength = 0;
		while (destinationLength < 0x8000 && destination[destinationLength])
			++destinationLength;
		if (destinationLength == 0x8000)
			return false;

		UInt32 nameLength = 0;
		while (nameLength < 0x104 && name[nameLength])
			++nameLength;
		if (nameLength == 0x104)
			return false;

		if (destinationLength > 0x8000 - 3 - nameLength)
			return false;

		destination[destinationLength++] = ',';
		destination[destinationLength++] = ' ';
		for (UInt32 i = 0; i <= nameLength; ++i)
			destination[destinationLength + i] = name[i];
		return true;
	}

	static __declspec(naked) void ArchiveMasterListAppendPatch()
	{
		__asm
		{
			lea		eax, [esp + 4Ch]
			push	eax
			push	ebp
			call	AppendArchiveNameBounded
			add		esp, 8
			test	al, al
			jnz		appendComplete
			mov		byte ptr [esp + 13h], 0

		appendComplete:
			lea		eax, [esp + 28h]
			push	eax
			push	ebx
			mov		eax, 0042F90Eh
			jmp		eax
		}
	}

	static __declspec(naked) void ArchivePluginsFileClosePatch()
	{
		__asm
		{
			test	esi, esi
			jz		fileClosed
			push	esi
			mov		eax, 00982179h
			call	eax
			add		esp, 4

		fileClosed:
			push	00A319FCh
			push	ebp
			push	0042F939h
			ret
		}
	}

	static __declspec(naked) void ArchiveSearchHandleClosePatch()
	{
		__asm
		{
			test	eax, eax
			jz		haveNextEntry

			push	ebx
			call	FindClose
			push	0042F91Eh
			ret

		haveNextEntry:
			push	0042F880h
			ret
		}
	}

	static __declspec(naked) void ArchiveMasterListNewlinePatch()
	{
		__asm
		{
			cmp		eax, 2
			jb		listTooShort

			cmp		byte ptr [eax + ebp - 2], 0Ah
			push	0042F895h
			ret

		listTooShort:
			push	0042F8AFh
			ret
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
