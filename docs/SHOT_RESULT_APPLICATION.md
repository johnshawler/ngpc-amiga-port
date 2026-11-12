# Native shot-result continuation

Additive source-level continuation after the verified swing application. The
preserved first-fire target is not rebuilt or modified.

## Implemented boundaries

* `209b05` resets the actual display/object state. Ordinary hits close the
  viewport, remove descendants, and finish the real shot task.
* Perfect hits allocate the actual `20c64e` child. Its queued palettes and two
  artwork groups, viewport transition, two countdowns and task completion use
  the original behavior. The parent waits for its actual task depth to clear.
* Miss `209b4b` allocates actual child `2109d5`. It copies terrain state, performs
  the bounded slot-5 condition, queues the notice and executes the original
  countdown. It stops before shared post-shot `210b74`, without setting `41e6`
  or inventing a result/completion.
* After real hit completion, outer `208799` waits for children and allocates
  original next child `20e065`. Execution stops at that child's entry. The
  original flight body and physics are not implemented by this slice.

All state/resources remain caller-owned. Original inputs are unchanged. No sound
requests are replaced with audible audio, and no stock timing claim is made.

## Component verification

* Exit parent: 3,072 unchanged-original cases, complete nonstack RAM/video,
  reset mirrors, callbacks and active/free links. Also 1,024 authored cases on
  host, ASan/UBSan and Linux m68020 instruction execution.
* Miss child: 3,072 original cases and 16 actual notice drawing cases on each
  backend. Authored wrapped-position/countdown coverage also passes all three
  instruction-test configurations. See `SHOT_MISS.md`.
* Perfect child: 2,304 original cases and 24 actual paired-art drawing cases on
  each backend. Authored complete lifecycle coverage passes all three
  instruction-test configurations. See `SHOT_PERFECT.md`.

## Verified integrated gate

Four-flow comparisons pass both backends: 4,464 updates and 18,102
ordered dispatches each, covering the actual miss notice path to `210b74`.
An ordinary-hit flow passes each backend through real shot completion and next
child allocation: 1,126 updates and 4,596 dispatches.

A separate input-driven perfect-hit flow passes each backend: 982 updates and
3,546 dispatches, including one genuine perfect-shot child entry, actual art
presentation and actual completion before flight-child allocation. Its ordinary
button sequence selects club 0, stops power at 118/120, and stops the accuracy
meter at 20. It does not patch game result flags or bypass any child.

The clean reproduction script exits successfully. Authored wrapper allocation
and boundary contracts pass on host, ASan/UBSan and Linux m68020, and the fresh
public suite passes all 167 tests. These are equivalence/instruction tests, not
stock Amiga timing or successful full-game operation.

Use `NTM_POWER_FLOWS=4 tools/check-shot-result-application.sh` with the existing
cross-toolchain activated. Default full coverage is larger and is not yet
verified for this continuation. No emulator target files are produced here.

## Next original-code gate

The `20e065` entry first branches on the genuine accuracy/result flags. Its
normal path calls the already-native camera clamp (`204e6c`), world-map loader,
HUD (`20a38a`) and scene setup (`209e3a`) before allocating further children,
including `20e443`, `20e2da` and `20e4d0`. Those actual bodies and associated
physics remain to be converted. Reuse the established services, but validate
ordering and real child completion rather than marking this allocation as
successful ball flight.
