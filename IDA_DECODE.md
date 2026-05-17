# Modern Windows Compatibility Decode

Target binary: `Oblivion.exe` 1.2.0.416, IDA instance `agb0`.

## Verified Hotspots

### OSGlobals main-thread handle duplication bug

`InitializeOSGlobals` stores the main thread id at `OSGlobals +0x10` and intends
to store a real duplicate of the current thread handle at `OSGlobals +0x14`.
The raw code at `0x00404A55` calls `GetCurrentThread`, then passes null process
handles into `DuplicateHandle`:

```asm
00404A55  call    ds:GetCurrentThread
00404A5B  push    ebx        ; dwOptions = 0
00404A5C  push    ebx        ; bInheritHandle = FALSE
00404A5D  push    ebx        ; dwDesiredAccess = 0
00404A5E  push    edi        ; lpTargetHandle = OSGlobals +0x14
00404A5F  push    ebx        ; hTargetProcessHandle = NULL
00404A60  push    eax        ; hSourceHandle = pseudo current thread
00404A61  push    ebx        ; hSourceProcessHandle = NULL
00404A62  call    ds:DuplicateHandle
00404A68  mov     ecx, [esi+10h]
```

This is a Windows API argument bug, not a renamed gameplay behavior. A useful
thread handle requires valid source/target process handles. The decoded consumer
is the Bink open path in `sub_410160`: when a video is opened from a non-main
thread, the engine compares `GetCurrentThreadId()` with `OSGlobals +0x10`, then
calls `SuspendThread(OSGlobals +0x14)` and later
`ResumeThread(OSGlobals +0x14)`.

```asm
0041022B  mov     edi, [eax+10h]
00410235  call    ds:GetCurrentThreadId
0041023C  cmp     eax, edi
00410242  mov     bl, 1
00410246  call    ds:SuspendThread
...
00410295  mov     ecx, [eax+14h]
0041029B  call    ds:ResumeThread
```

Patch contract:

- Validate the 19 original bytes at `0x00404A55`.
- Validate the Bink suspend/resume use blocks at `0x00410229` and `0x0041028E`
  before installing.
- Replace only `0x00404A55..0x00404A67`.
- Duplicate the current thread with
  `DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
  &OSGlobals->mainThreadHandle, 0, FALSE, DUPLICATE_SAME_ACCESS)`.
- Continue at `0x00404A68`.

No Bink playback flags, thread-id checks, or `SuspendThread`/`ResumeThread`
callsites are changed.

### BSTexturePalette high-address path bug

This verified modern-Windows-sensitive hotspot is `sub_4A25F0`, located between
`BSTexturePalette` constructor/destructor symbols. Its callers are:

- `0x43FC89` from `sub_43FC20`, a TES cleanup/streaming critical-section path.
- `0x4A2879` from `sub_4A2850`, which calls this pass and then reports texture
  count problems.
- `0x590414` from `sub_5903E0`, when `InterfaceManager + 0xB8` requests the pass.

At `0x4A26F2`, the function calls `_strrchr(Str, '_')` while processing entries
whose record count field at `entry + 4` equals `2`.

IDA-observed sequence:

```asm
004A26E3  mov     ebx, [esp+Str]
004A26E7  push    5Fh
004A26E9  push    ebx
004A26F2  call    _strrchr
004A26F7  mov     esi, eax
004A26F9  sub     esi, ebx
004A26FB  add     esp, 8
004A26FE  test    esi, esi
004A2700  jle     004A27A1
```

`_strrchr` returns either the last underscore pointer or `NULL`. The vanilla code
does not test `eax` before subtracting the source pointer. If `eax == 0` and the
source string address has the high bit set, `0 - source` can become a positive
signed value. The later path uses that value as a `strncpy` count:

```asm
004A2706  push    esi
004A270D  call    _strncpy
```

The fallback path at `0x4A27A1` removes the original string from the texture
palette path:

```asm
004A27A1  mov     ecx, [esp+this]
004A27A5  push    ebx
004A27A6  call    sub_4A1A10
```

`sub_4A1A10` normalizes and hashes the path, tries `ArchiveManager_LazyFileLookup`,
and removes the entry from one of the two maps. That is the path already taken by
the vanilla `jle` when the underscore position is not positive.

Patch contract:

- Validate the seven original bytes at `0x004A26F7`: `8B F0 2B F3 83 C4 08`.
- Preserve `mov esi, eax; sub esi, ebx; add esp, 8`.
- If `eax == 0`, jump to `0x004A27A1`, the existing fallback path.
- If `eax != 0`, jump to `0x004A26FE`, where vanilla still tests `esi`.
- If another loaded plugin has already replaced `0x004A26F7` with a relative
  jump to the exact same guard thunk shape, accept that site as already fixed
  and do not install a duplicate jump.

No texture ownership, map, archive, or path semantics are changed.

The local load-order mismatch was traced to `BA_EngineFixes.dll`. Its public
source and the deployed binary both show one patch for this same improper
`strrchr` return check: it writes a seven-byte jump at `0x004A26F7`, preserves
the original three instructions, tests `eax`, sends null returns to
`0x004A27A1`, and resumes non-null returns at `0x004A26FE`. Modern Engine Fixes
therefore recognizes only that exact already-guarded machine-code shape.

### Loading texture wait tick bug

This verified hotspot is in `sub_410840`, the loading texture render/wait
path. The function builds a `Data\Vid\...` path, loads it through
`D3DXCreateTextureFromFileA`, renders it, and then waits for the configured
duration stored at `0x00B030AC` while calling `sub_410390(1)` to pump messages
and input.

IDA-observed wait block:

```asm
004109EB  mov     esi, ds:GetTickCount
004109F1  call    esi
004109F9  fild    [esp+10h]
004109FD  jge     00410A05
004109FF  fadd    ds:flt_A2FC78
00410A05  fld     ds:flt_B030AC
00410A0B  fmul    ds:dbl_A2FC70
00410A29  fistp   [esp+10h]
00410A2D  mov     edi, [esp+10h]
00410A35  call    esi
00410A37  cmp     edi, eax
00410A39  jbe     00410A49
00410A3B  push    1
00410A3D  call    sub_410390
00410A47  jnz     00410A35
```

The intended behavior is:

```text
wait until the configured duration has elapsed, unless sub_410390(1) returns 0
```

The vanilla implementation computes an absolute future tick target and compares
it with `GetTickCount()`. The preceding x87 conversion treats high-bit tick
values as unsigned by adding `4294967296.0`, then stores back through a signed
32-bit `fistp`. On long system uptimes, and at the 32-bit tick wrap boundary,
the absolute target path can become invalid or compare as already elapsed.

Patch contract:

- Validate the 94 original bytes at `0x004109EB`.
- Replace `0x004109EB..0x00410A48` with `call LoadingTextureWaitPatch`,
  `jmp 0x00410A49`, and NOPs.
- Read the same `0x00B030AC` duration and preserve the same `sub_410390(1)` pump.
- Use `(SInt32)(GetTickCount() - start) < duration` instead of an absolute target.

### Loading screen/movie speed trace

The loading-screen presentation path was traced separately from save/load I/O.
The static `.dds` wait above is reached from one caller:

```asm
00410DE5  push    esi
00410DE6  call    sub_410840
```

`sub_410D10` copies a comma-separated sequence string, tokenizes it with
`,` from `0x00A319FC`, pumps `sub_410390(1)` before each token, and dispatches
by extension:

```asm
00410D79  push    1
00410D7B  call    sub_410390
...
00410D9F  push    offset aBik
00410DA5  call    __mbsicmp
00410DC9  call    sub_410BA0      ; .bik token
...
00410DD3  push    offset aDds_0
00410DD9  call    __mbsicmp
00410DE6  call    sub_410840      ; .dds token
```

The wait duration for static `.dds` loading screens is not a hidden loader
stall. It is the `fStaticScreenWaitTime:General` setting at `0x00A31B2C`, whose
runtime value is stored at `0x00B030AC`. The IDA database default bytes for that
runtime value are `00 00 40 40` (`3.0f`). `sub_410840` reads that value, converts
seconds to milliseconds, and waits after drawing the static texture.

`sub_410390` is a message/input pump, not a file loader:

```asm
004103A7  call    PeekMessageA
004103C5  call    TranslateMessage
004103CC  call    DispatchMessageA
00410457  call    InputGlobals::PollAndUpdateInputState
00410462  call    InputGlobals::QueryControlState
00410471  call    InputGlobals::QueryControlState
00410490  call    GetExitCodeThread
```

This proves only that static loading screens include a configured post-draw
display interval. Modern Engine Fixes keeps that interval and fixes only the
`GetTickCount` absolute-target arithmetic. Removing or shortening the wait would
be a presentation/configuration change, not a decoded modern-Windows engine bug.

The optional implementation reduction is intentionally documented as a
presentation tweak. The value is built into the DLL rather than read from an
external INI: the wait helper uses `fStaticScreenWaitTime:General * 0.70` before
converting seconds to milliseconds. This decreases the verified artificial
static-screen interval by 30%. The same `sub_410390(1)` pump and wrap-safe
elapsed tick arithmetic are still used.

Bink playback follows a separate timed-movie path. `sub_410BA0` creates or reuses
a small movie state object and calls `sub_410A70`; `sub_410A70` loops
`VideoPass` until playback completes or the pump exits:

```asm
00410AC5  push    ebx
00410AC6  push    ebp
00410AC9  call    VideoPass
00410ACE  test    al, al
00410AD0  jnz     00410AC5
```

`VideoPass` calls the same pump, checks `BinkWait`, advances frames with
`BinkNextFrame`, and sleeps for `1` ms only when Bink reports the next frame is
not ready:

```asm
004106C8  call    sub_410390
004106E6  call    BinkWait
00410726  call    BinkNextFrame
004107F7  push    1
004107F9  call    Sleep
```

The verified startup/movie callsites are:

```asm
0040EAEA  call    sub_410E40      ; starts MoviePlayer thread for sequence string
0040F0BE  call    sub_410BA0      ; plays main-menu intro movie string
00410DC9  call    sub_410BA0      ; .bik token in comma-separated sequence
0066F2D1  call    sub_410BA0      ; cell-position transition loading movie path
```

Skipping those calls would skip movies or configured presentation assets. That
may make startup or transitions appear faster, but the IDA trace does not show a
modern-Windows compatibility failure in those Bink callsites. No Bink skip patch
is included.

Finally, the load-game transition guard below is separate from both presentation
paths. IDA shows its single code reference at `0x00465BF7`, immediately before
the save-load subroutine at `0x00465C4D`. The patch corrects the tick rollover
comparison before save data processing begins; it does not speed up file reads
or form reconstruction.

### Load-game start/menu transition tick bug

The third verified hotspot is `sub_459A10`, called once from
`TESSaveLoadGame_LoadGame` at `0x465BF7`. The function performs menu and loading
state transitions, then records `dword_B33B08 = GetTickCount()` and immediately
tests whether more than `3000` ms has elapsed.

IDA-observed block:

```asm
00459A43  mov     esi, ds:GetTickCount
00459A49  call    esi
00459A4B  mov     ds:0B33B08h, eax
00459A50  call    esi
00459A52  mov     ecx, ds:0B33B08h
00459A58  add     ecx, 0BB8h
00459A5E  cmp     eax, ecx
00459A60  pop     esi
00459A61  jbe     00459A8E
00459A63  call    sub_57BAC0
```

Normally the branch at `0x459A61` returns immediately because the second tick is
not later than `start + 3000`. At the 32-bit tick wrap boundary, `start + 3000`
can wrap low, making the absolute comparison fire immediately and run the
post-threshold path.

Patch contract:

- Validate the 32 original bytes at `0x00459A43`.
- Preserve both `GetTickCount` calls and the write to `0x00B33B08`.
- Replace `now > start + 3000` with `now - start > 3000`.
- Return normally for the not-elapsed path, or continue at `0x00459A63` for the
  elapsed path.

No caller, menu, save/load, or travel-update behavior is changed outside the
tick comparison.

### Worldspace message delay tick bug

The fourth verified hotspot is `sub_5BDCD0`. IDA shows it gating on
`sub_5DDCD0()`, current worldspace, no current interior cell, `sub_4EF2D0(..., 4)`,
and `byte_B02D70`. In that path it calls `ShowUIMessageBox` with `dword_B38C00`,
then waits about one second while pumping three engine/UI update helpers.

IDA-observed wait block:

```asm
005BDD35  call    esi
005BDD37  lea     edi, [eax+3E8h]
005BDD3D  call    esi
005BDD3F  cmp     eax, edi
005BDD41  jnb     005BDD5E
005BDD43  call    sub_5791A0
005BDD48  call    sub_579220
005BDD4D  mov     ecx, ds:0B33398h
005BDD53  call    sub_40D4D0
005BDD58  call    esi
005BDD5A  cmp     eax, edi
005BDD5C  jb      005BDD43
```

The intended behavior is:

```text
after the message box, pump sub_5791A0, sub_579220, and sub_40D4D0(OSGlobals)
for roughly 1000 ms, then continue at 0x005BDD5E
```

This is another absolute `GetTickCount() + duration` target. If `start + 1000`
wraps below the current tick value, the first unsigned compare at `0x5BDD41`
treats the wait as already complete and skips the pump.

Patch contract:

- Validate the 41 original bytes at `0x005BDD35`.
- Replace the block with `call WorldspaceMessageDelayPatch`, `jmp 0x005BDD5E`,
  and NOPs.
- Preserve the same three helper calls in the same order.
- Use `(SInt32)(GetTickCount() - start) < 1000` instead of `now < start + 1000`.

No attempt is made to rename the condition or infer gameplay semantics beyond
what the IDA block proves.

### Global visual animation timer precision bug

The next verified hotspot is the global scene animation timer stored at
`0x00B33A30`. This is distinct from the per-actor `ActorAnimData + 0x94` clock
handled by the integrated actor-animation clock fix later in this file.

`sub_4424D0` advances the global timer:

```asm
00442536  fld     ds:flt_B33A30
00442541  fadd    [esp+10h+arg_0]
00442546  fstp    ds:flt_B33A30
0044254C  fld     ds:flt_B33A30
```

The caller in the main frame loop passes `arg_0 = fAnimationMult:General *
frameDelta` outside menu mode:

```asm
0040DE83  fld     flt_B06530
0040DE89  fmul    flt_B33E9C
0040DEA1  call    sub_4424D0
```

IDA xrefs to `sub_4424D0` show two other engine callers:

```asm
00461089  fldz
00461095  call    sub_4424D0

0066F248  fld     ds:flt_B33E9C
0066F25B  call    sub_4424D0
```

The first caller passes `0.0` during a flagged world/cell update path in
`sub_461030`. The second caller is in `PlayerCharacter_ChangeCellAndPosition`
after the cell-position transition and uses `flt_B33E9C`. These calls do not
add fire-specific semantics; they only confirm that `sub_4424D0` is a shared
TES scene update path.

`sub_4424D0` then passes the updated absolute timer into the scene root:

```asm
004425A8  fld     ds:flt_B33A30
004425AE  push    1
004425B1  mov     ecx, [esi+14h]
004425B7  call    NiAVObject_UpdateNiAVObject
```

This is a broad visual-controller timing path. It is not decoded as a
fire-specific routine; fire-like symptoms are plausible only because texture,
particle, and other scene controllers consume this absolute time through the
scene graph. The patch does not assume a particular asset or effect name.

The timer is saved and loaded in the TES save block:

```asm
004411AD  push    4
004411AF  push    offset flt_B33A30
004411B4  call    SaveLoad_SaveData

004413D4  push    4
004413D6  push    offset flt_B33A30
004413DB  call    SaveLoad_LoadData
```

Because `flt_B33A30` is a 32-bit float, once it reaches `131072.0` seconds the
float spacing is `0.015625` seconds. That is close to a 60 FPS frame delta and
larger than many modern high-refresh frame deltas, so small controller time
increments can quantize or advance unevenly after loading an old save.

Patch contract:

- Validate the original call bytes at `0x004413DB`: `E8 F0 20 01 00`.
- Also validate the decoded update/use blocks at `0x00442536` and `0x004425A8`
  before installing the load hook.
- Replace only the `SaveLoad_LoadData` call at `0x004413DB`.
- The hook calls the original `SaveLoad_LoadData(this, &flt_B33A30, 4)`.
- If the loaded timer is `>= 131072.0`, write `0.0` to `flt_B33A30`.
- If EngineBugFixes has already replaced `0x004413DB` with its
  `GlobalAnimTimerFix` hook, accept only the verified hook shape that calls
  `0x004534D0`, calls a reset helper, and jumps back to `0x004413E0`. The reset
  helper must read/write the `0x00B33A30` global timer pointer and compare the
  loaded float bits against `0x48000000`.

No live scene-update rebasing is attempted. Resetting the absolute timer while
controllers are already active would require a separate per-controller phase
preservation proof; this patch only normalizes an oversized saved timer before
the loaded scene resumes.

The local load-order conflict was traced to EngineBugFixes v2.22. Its partial
source `Patches/GlobalAnimTimerFix.cpp` installs a hook at `0x004413DB`, calls
the original load routine, then resets the same global timer when the loaded raw
float value is at or above `0x48000000` (`131072.0f`). The deployed binary was
checked for the same hook and helper shape before allowing Modern Engine Fixes
to skip its duplicate hook.

### Merged TragicEngineFix actor animation clock precision bug

The requested `TragicEngineFix` source folder is not present in the project root.
The local build metadata does contain a stale
`C:\src\EngineTweak\TragicEngineFix\...` path, and the available local plugin
matching that lineage is `ActorAnimClockFix`. Its IDA-backed patch is merged
here after re-validating the same Oblivion 1.2.0.416 byte chain.

This path is distinct from the global visual timer above. It targets the
per-actor animation clock at `ActorAnimData + 0x94`.

`ActorAnimData::Update` at `0x00476D10` samples active animation slots before
the per-frame increment:

```asm
00476E86  xor     edi, edi
...
00476F43  fld     dword ptr [esi+94h]
00476F4D  call    BSAnimGroupSequence_SampleUpdate
```

At `0x00476F97` the same function advances the actor-local clock:

```asm
00476F97  fld     dword ptr [esi+94h]
00476F9D  fadd    [ebp+arg_4]
00476FA0  fstp    dword ptr [esi+94h]
```

IDA shows `ESI` is the current actor animation data object in this function.
The local xOBSE structure definition places active `BSAnimGroupSequence*`
pointers at `ActorAnimData + 0xA0`, five entries total.

`BSAnimGroupSequence_SampleUpdate` at `0x0049F4A0` consumes the actor clock by
adding it to `BSAnimGroupSequence + 0x48`:

```c
if ((sequence->state - 1) <= 2)
    NiControllerSequence_Update(sequence, sequence->offset48 + actorClock, 1);
```

`NiControllerSequence_Update` at `0x006C5FC0` then computes deltas from that
input time and, when the third argument is nonzero, stores the latest input time
and scaled time in the controller sequence.

Save/load preserves the sum rather than only the offset:

```asm
0049F570  ; save: sequence +0x48 plus ActorAnimData +0x94
0049F5F0  ; load: sequence +0x48 = saved summed time - ActorAnimData +0x94
```

`ActorAnimData_RestorePlaySavedSlot` writes the saved actor clock back to
`ActorAnimData + 0x94`:

```asm
00474BB2  mov     [esi+ebx*2+3Ch], bp
00474BB8  fstp    dword ptr [esi+94h]
```

Patch contract:

- Validate the restored clock write at `0x00474BB2`.
- Validate the active-slot sample loop at `0x00476E86`.
- Validate the clock increment at `0x00476F97`.
- Validate the post-increment hook site at `0x00476FA6`.
- Validate the sample/save/load/controller functions at `0x0049F4A0`,
  `0x0049F570`, `0x0049F5F0`, and `0x006C5FC0`.
- Replace `0x00476E86` with a call before active-slot sampling and NOP the
  remaining five bytes.
- Replace `0x00476FA6` with a call immediately after the vanilla clock write.
- Rebase only finite, non-sentinel time values.

The rebase preserves the sampled total:

```text
oldTotal = oldOffset + oldClock
newClock = oldClock - shift
newOffset = oldOffset + shift
newTotal = newOffset + newClock = oldTotal
```

The merged threshold remains `4096.0` seconds and the step remains `2048.0`
seconds. This is an early precision guard for the actor-local single-precision
accumulator; it is not decoded as a fire-specific path and does not rename the
underlying animation state semantics.

### Renderer initialization failure-message path

EngineBugFixes' `InitRenderer` patch was used only as a naming/candidate hint.
The accepted behavior was verified in Oblivion's IDA database.

`CreateWindowAndInitialize` calls `sub_4980D0(1)` before constructing the
Gamebryo renderer:

```asm
004983B8  call    sub_4980D0
004983BD  add     esp, 4
004983C0  or      ebp, 0FFFFFFFFh
004983C3  xor     ebx, ebx
004983C5  test    al, al
004983C7  jz      loc_498566
```

`sub_4980D0` is the adapter/render-mode validation pass. Its failure exits write
specific text into `byte_B34FC8`, including:

```text
Windowed mode not supported on this Adapter.
Desired render mode not found on Adapter.
Pixel and Vertex Shader versions incorrect.  Requires a Geforce4 4400 or Radeon 8500 or better.
Hardware T&L required but not supported by Adapter.
Bad Adapter Number or Adapter not found.
No D3D Device description found.
```

The caller at `0x004052F0` uses `CreateWindowAndInitialize`'s return value to
show the final startup error:

```asm
0040530E  call    CreateWindowAndInitialize
00405316  test    eax, eax
00405318  jnz     short loc_40534D
0040531A  push    offset byte_B34FC8
00405323  push    offset aFailedToInit_0 ; "Failed to initialize renderer.\n%s"
0040533F  call    ds:MessageBoxA
00405347  call    ds:ExitProcess
```

On validation failure, the original branch jumps into the later renderer path at
`0x00498566`. If no renderer instance is available, that path writes
`"Unknown error creating the Gamebryo Renderer."` at `0x004985F9`, replacing the
more precise adapter/render-mode reason from `sub_4980D0`.

Accepted patch contract:

- At `0x004983C0`, test `AL`, the return value from `sub_4980D0(1)`, before
  replaying the overwritten `or ebp, -1; xor ebx, ebx` bytes.
- If validation failed, return through `0x00498E7D` so the caller reports the
  already-written `byte_B34FC8` text.
- If validation passed, resume at `0x004983C5` and let Oblivion's original
  renderer initialization continue unchanged.

This is a fidelity/diagnostic guard. It preserves the engine's own unsupported
adapter/render-mode result; it does not reinterpret the mode list or make an
unsupported display mode valid.

If EngineBugFixes v2.22 has already installed the same `test al, al` guard with
the original resume bytes and transfers to `0x004983C5`/`0x00498E7D`, Modern
Engine Fixes accepts the site as already guarded and does not overwrite it.

### Actor GetAttacked and IsTalking null process pointer

EngineBugFixes' `ActorWithoutProcessCTD` patch was used only to identify a
candidate address. The accepted target was verified directly in Oblivion IDA.

`Actor::GetAttacked` is a short tail-call wrapper:

```asm
005E58C0  mov     ecx, [ecx+58h]
005E58C3  mov     eax, [ecx]
005E58C5  mov     edx, [eax+398h]
005E58CB  jmp     edx
```

If `Actor +0x58` is null, `0x005E58C3` dereferences null before the call can
reach the lower object. The neighboring wrapper at `0x005E58D0` accesses the
same actor offset but checks it before using the vtable:

```asm
005E58D0  mov     ecx, [ecx+58h]
005E58D3  test    ecx, ecx
005E58D5  jz      short locret_5E58E1
005E58D7  mov     eax, [ecx]
005E58D9  mov     eax, [eax+39Ch]
005E58DF  jmp     eax
005E58E1  retn    4
```

Accepted patch contract:

- Validate the original five bytes at `0x005E58C0`: `8B 49 58 8B 01`.
- Validate the adjacent guarded wrapper at `0x005E58D0` as a same-offset
  contrast point; this proves only the null-check pattern, not additional
  gameplay semantics.
- Replace only the first five bytes of `Actor::GetAttacked`.
- If `Actor +0x58` is non-null, replay the original setup and continue at
  `0x005E58C5`.
- If `Actor +0x58` is null, return `0` directly. No actor state is created or
  modified.
- If EngineBugFixes has already installed a hook that loads `Actor +0x58`, tests
  it, preserves the original vtable call setup, and returns zero on null, Modern
  Engine Fixes accepts the site as already guarded.

The same pass accepted `Actor::IsTalking`, another short actor-process wrapper:

```asm
005E0E60  mov     eax, ecx
005E0E62  mov     ecx, [eax+58h]
005E0E65  mov     edx, [ecx]
005E0E67  push    eax
005E0E68  mov     eax, [edx+368h]
005E0E6E  call    eax
005E0E70  retn
```

The local contrast point is again adjacent and explicit:

```asm
005E0E80  cmp     dword ptr [ecx+58h], 0
005E0E84  jz      short loc_5E0E93
005E0E86  mov     ecx, [ecx+58h]
005E0E89  mov     eax, [ecx]
005E0E8B  mov     edx, [eax+388h]
005E0E91  jmp     edx
005E0E93  xor     al, al
005E0E95  retn
```

Accepted patch contract:

- Validate the original five bytes at `0x005E0E62`: `8B 48 58 8B 11`.
- Validate the adjacent guarded wrapper at `0x005E0E80`.
- If `Actor +0x58` is non-null, replay the original setup and continue at
  `0x005E0E67`.
- If `Actor +0x58` is null, return through `0x005E0E70` with `AL = 0`.
- If EngineBugFixes has already installed a hook with the same null check,
  original setup, and false-return path, Modern Engine Fixes accepts it.

## Fidelity Pass Notes

The Windows-facing and engine-bug scan covered:

- `OSGlobals` main-thread handle: accepted the invalid `DuplicateHandle` argument
  block at `0x00404A55` after verifying that the stored handle is consumed by
  the Bink open path for `SuspendThread`/`ResumeThread`.
- `GetTickCount`: accepted only the absolute-target wait/threshold blocks at
  `0x004109EB`, `0x00459A43`, and `0x005BDD35`. Other uses were elapsed logging,
  frame-time initialization/update, or accumulator paths based on subtraction.
- Global scene animation time: accepted the saved `flt_B33A30` precision guard
  at `0x004413DB` after verifying the frame-loop update and scene-root use in
  `sub_4424D0`.
- TragicEngineFix merge: accepted the per-actor `ActorAnimData +0x94` clock
  rebase only after verifying the active slot sample path, save/load summed-time
  path, and lower `NiControllerSequence` time update path. The standalone
  `ActorAnimClockFix.dll` is not part of the supported deployment; Modern Engine
  Fixes owns the actor-clock hook sites directly.
- Renderer init: accepted only the `sub_4980D0(1)` failure-message preservation
  path at `0x004983C0`; no display-mode fallback or device-capability policy was
  invented.
- Actor null process: accepted only `Actor::GetAttacked` at `0x005E58C0` and
  `Actor::IsTalking` at `0x005E0E62`, where IDA shows immediate null-sensitive
  dereferences and nearby same-offset guarded wrappers. The broader
  `ActorWithoutProcessCTD` family remains unmerged until each individual
  callsite is verified the same way.
- `GetVersionExA`: imported but no code xrefs in the IDA database; no patch.
- `QueryPerformanceCounter/Frequency`: I/O manager code uses 64-bit counter
  comparisons; no patch.
- `GlobalMemoryStatus`: memory heap callsites use physical-memory fields for
  telemetry/low-memory cleanup thresholds; no verified modern-Windows semantic
  replacement was applied.
- `GetDriveTypeA`/`GetLogicalDriveStringsA`: `sub_404940` enumerates CD-ROM
  drives and looks for `OblivionLauncher.exe`; its WinMain callers display
  copy-protection prompts. This was decoded but deliberately not patched.
- `_strrchr` xrefs: most callsites test the return pointer before use. Several
  savegame path helpers use `strrchr(path, '\\') + 1`; those assume full internal
  paths and are not the high-address signed-subtract failure fixed above.

## Verification Notes

- Exact original bytes were read from IDA for the installed patch sites before adding
  validation arrays.
- `_strrchr` xrefs were scanned in the IDA database. `0x004A26F7` was the only
  verified `strrchr` callsite found with an immediate unchecked `return - source`
  length path.
- Renderer initialization was verified against both the mode-validation helper
  and its single startup caller before accepting the `0x004983C0` early-exit
  guard.
- `Actor::GetAttacked` and `Actor::IsTalking` were verified against their raw
  vtable wrappers and adjacent guarded actor process wrappers before accepting
  their null guards.
- The plugin validates exact decoded bytes before patching any site.
- The plugin refuses to patch non-1.2.0.416 runtimes through `OBSEPlugin_Query`.
