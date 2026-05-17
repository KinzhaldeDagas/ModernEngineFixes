# Modern Engine Fixes

`Modern Engine Fixes.dll` is a narrow OBSE runtime plugin for Oblivion
1.2.0.416. It targets IDA-verified engine-level compatibility failures observed
on modern Windows.

Verified fixes:

- `0x00404A55`: fixes `OSGlobals` main-thread handle duplication. Oblivion
  stored a thread handle for later Bink `SuspendThread`/`ResumeThread` use, but
  the original `DuplicateHandle` call passed null process handles.
- `0x004A26F7`: guards a `BSTexturePalette` texture-path pass where Oblivion
  calls `strrchr(path, '_')`, subtracts the source pointer from the return value,
  and only then checks the signed result. If `BA_EngineFixes` has already
  installed the exact same guard thunk, this plugin accepts that site as already
  fixed and continues with the remaining patches.
- `0x004109EB`: replaces the loading texture wait block. The plugin preserves
  the same configured duration and `sub_410390(1)` pump call, but uses wrap-safe
  elapsed `GetTickCount` math. The DLL also shortens the verified artificial
  static `.dds` loading-screen display interval by 30%.
- `0x00459A43`: fixes the load-game start/menu transition threshold check by
  preserving the tick write and comparing elapsed time instead of `now` against
  `start + 3000`.
- `0x005BDD35`: fixes a worldspace message delay loop by preserving the same
  three pump calls and replacing `now < start + 1000` with wrap-safe elapsed
  time.
- `0x004413DB`: guards the saved global scene animation timer. On load, if
  `flt_B33A30` is already large enough to lose ordinary frame deltas as a
  32-bit float, the plugin resets it before scene-root visual controllers resume.
  If EngineBugFixes v2.22 has already installed its verified `GlobalAnimTimerFix`
  hook, this plugin accepts that site as already fixed and continues.
- `0x004983C0`: preserves renderer-startup failure specificity after Oblivion's
  adapter/render-mode validation reports failure. The patch returns before the
  later Gamebryo renderer path can replace the specific text with a generic
  renderer error; it does not make unsupported render modes supported. If
  EngineBugFixes v2.22 has already installed its verified `InitRenderer` hook,
  this plugin accepts that site as already fixed.
- `0x005E58C0`: guards `Actor::GetAttacked` when `Actor +0x58` is null. IDA
  shows the original function immediately dereferences that pointer before
  tail-jumping through the pointee vtable; the patch returns false only for the
  null-pointer case.
- `0x005E0E62`: applies the same verified null-pointer guard to
  `Actor::IsTalking`. The adjacent actor wrapper at `0x005E0E80` already returns
  false for a null `Actor +0x58`, so the patch mirrors that local engine pattern.
- `0x00476E86` and `0x00476FA6`: merge the local TragicEngineFix-lineage
  actor-animation clock guard. The plugin keeps `ActorAnimData +0x94` and the
  paired active `BSAnimGroupSequence +0x48` offsets numerically small while
  preserving their summed sample time. This fix is implemented directly in
  Modern Engine Fixes; the standalone `ActorAnimClockFix.dll` should not be
  deployed alongside it.

The full IDA-backed decode is in `IDA_DECODE.md`.

Output:

```text
Modern Engine Fixes.dll
```
