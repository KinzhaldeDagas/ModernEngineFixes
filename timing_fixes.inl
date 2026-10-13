	static __declspec(naked) void LoadGameStartTickPatch()
	{
		__asm
		{
			mov		esi, ds:0A280D0h
			call	esi
			mov		ds:0B33B08h, eax
			call	esi
			mov		ecx, ds:0B33B08h
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

		// Stay in floating point: finite saved clocks can exceed the UInt32
		// chunk-count range. The power-of-two step keeps ordinary rebases exact.
		float shift = clock - (float)std::fmod((double)clock, (double)kActorAnimRebaseStep);
		if (shift <= 0.0f || shift > clock)
			return;

		// Active slots can refer to the same sequence. Stage one adjusted value
		// per object so aliases receive the shift only once, and so a rejected
		// sentinel crossing leaves every sequence and the actor clock unchanged.
		void* sequences[kActorAnimSequenceCount] = { 0 };
		float adjustedOffsets[kActorAnimSequenceCount] = { 0 };
		UInt32 sequenceCount = 0;
		for (UInt32 i = 0; i < kActorAnimSequenceCount; i++)
		{
			void* sequence = ActorAnimSequence(animData, i);
			if (!sequence)
				continue;
			UInt32 previous = 0;
			while (previous < sequenceCount && sequences[previous] != sequence)
				++previous;
			if (previous != sequenceCount || !IsRebasableTime(SequenceOffset(sequence)))
				continue;
			float adjusted = SequenceOffset(sequence) + shift;
			if (!IsRebasableTime(adjusted))
				return;
			sequences[sequenceCount] = sequence;
			adjustedOffsets[sequenceCount++] = adjusted;
		}
		for (UInt32 i = 0; i < sequenceCount; ++i)
			SequenceOffset(sequences[i]) = adjustedOffsets[i];

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

	// IDA: native 6007F0 jumps directly to 600F6D. The intervening death paths
	// are offline evidence only; Blood.dll's call at 600B61 is outside this hook.
