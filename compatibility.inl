	static ActorAnimPreSamplePatchState GetActorAnimPreSamplePatchState()
	{
		if (!RequireReadableSignature("actor pre-sample state", kActorAnimUpdatePreSamplePatchSite,
			sizeof(kActorAnimPreSamplePatchExpectedBytes))) return kActorAnimPreSamplePatchMismatch;
		if (BytesMatch(kActorAnimUpdatePreSamplePatchSite,
				kActorAnimPreSamplePatchExpectedBytes,
				sizeof(kActorAnimPreSamplePatchExpectedBytes)))
		{
			return kActorAnimPreSamplePatchNeedsInstall;
		}

		const UInt8* site = (const UInt8*)kActorAnimUpdatePreSamplePatchSite;

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
		if (!RequireReadableSignature("actor clock state", kActorAnimUpdateClockPatchSite,
			sizeof(kActorAnimClockPatchExpectedBytes))) return kActorAnimClockPatchMismatch;
		if (BytesMatch(kActorAnimUpdateClockPatchSite,
				kActorAnimClockPatchExpectedBytes,
				sizeof(kActorAnimClockPatchExpectedBytes)))
		{
			return kActorAnimClockPatchNeedsInstall;
		}

		const UInt8* site = (const UInt8*)kActorAnimUpdateClockPatchSite;

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

	static TESWorldSpaceOFSTPatchState GetTESWorldSpaceOFSTPatchState()
	{
		if (!RequireReadableSignature("worldspace OFST state", kTESWorldSpaceLoadOFSTPatchSite,
			sizeof(kTESWorldSpaceLoadOFSTExpectedBytes))) return kTESWorldSpaceOFSTPatchMismatch;
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
