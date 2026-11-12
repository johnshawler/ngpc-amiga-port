# Native trajectory swing child

This additive implementation converts original entry `20b43f`, callback `20b47f`
and animation `20b50b`, including `20b5d1` sound selection. It does not implement
ball physics, audio synthesis, or claim target frame rate.

## State and behavior

The original trajectory state remains caller-owned. Its counter byte is task
`+16`; `last_label` becomes the timer reload at `+1e`. New state keeps timer
`+17`, result `+1f`, and the animation callback. Entry selects reload 3 when
signed power is at least 60, otherwise reload 4 and a sprite selection increment
of 4. It clears freeze and requests a real sprite animation restart.

Countdown and the original putter frame-remapping table drive HUD queue entries.
Normal hit occurs at old counter 9, putter hit at 10, conditional on the original
accuracy flag. Miss events occur at 10 and 14 respectively, including the
original skipped counter/frame. Signed counters at least 15 publish result 255.
A hit publishes result 1. Both use the genuine task finish/defer/unlink/recycle
path; no child completion is fabricated. Negative counters outside the actual
animation range return an explicit boundary before mutation.

Sound requests retain service 0 command 65533, conditionally suppressed by
original `447e`, followed by service 2 command `0x100 | sound_index`. Sound
indices come from private terrain/club tables; miss uses command `0x104`.
The service 2 parameter is its consumed low-byte argument, zero. The original
sound subsystem is intercepted only at the existing API boundary in the oracle.

Resources are privately resolved from the owner's original game. HUD bases are
nonmonotonic, so selection uses the exact six-entry table, not golfer arithmetic.
The exhaustive hypothetical matrix includes zero-tile descriptors outside tested
renderable combinations; preserving their producer identity is not a claim that
the renderer accepts them. Actual gameplay drawing requires the separate
application/drain gate. No original bytes are embedded
in these new source files.

## Sprite mapping extension

Tag 27 selection bases are multiples of 64, from 64 through 384. Existing
selection offsets 0, 8 and 16 retain indices `3*(base/64-1)+offset/8` (0..17).
New swing offsets 4, 12 and 20 append indices
`18+3*(base/64-1)+(offset-4)/8` (18..35). This fits the existing forty-slot
asset array and preserves old exporters and binary layout. The new complete
resolver is `reference/shot_trajectory_swing_object_assets.py`.

## Verification

`tools/check-shot-trajectory-swing.sh` builds authored host and sanitizer tests,
optional m68020/QEMU tests, unchanged-original state/sound/lifecycle oracles for
both rendering backends, and original animated-object comparisons. The state
oracle excludes only measured CPU stack scratch; it checks all other work RAM,
video/hardware, queued original resource identities, and owner/task links.
The object oracle executes the original drawing head and tail, comparing RAM
and video with native rendering under animation, flips, freeze, and clipping.

The authored tests use independent fixture resources and cover all 256 power
bytes, genuine hit completion, sound request identity, and atomic boundaries.
QEMU execution is an instruction-correctness check, not A1200 performance.

Target disk and Amiga frontend are deliberately untouched by this module.

### Verified results for this slice

- 6,144 original state cases and 612 sound requests passed per backend
  (default and private-copy).
- 31,104 original tag 27 object drawing rounds passed with all 36 selections,
  eight freeze/flip flag combinations, three positions, and 36 animation ticks.
- 1,024 authored complete normal/putter, hit/miss, power-byte flows passed on
  host, ASan/UBSan, and m68020 under QEMU.

These are functional equivalence checks, not stock-target timing measurements.
- Previous eighteen-selection original regression passed 648 drawing rounds
  after the append-only mapping change. Existing authored object regressions
  passed on host, ASan/UBSan, and m68020/QEMU: 65,536 flags/delay pairs,
  131,072 clipping positions, ordering/removal, queue occupancy, resource
  identity and malformed-resource atomic rejection.

The 31,104-round standalone object gate was run with the default backend;
its private-copy counterpart is available in the reproduction script but was
not separately run for this slice. Both producer-state backends passed, and
application-level drawing integration is reported by the parent application
verification rather than conflated with this module's standalone coverage.
