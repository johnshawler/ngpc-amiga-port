# Native swing continuation (work in progress)

This is additive reimplementation from original-game execution, not recovery of
lost source. It does not replace or rebuild the preserved first-fire target.

## Verified parent prefix

`shot_swing.c` implements original `209aa2`, `209ad3` and `209af4`:

* Set lifecycle 4, clear the real parent result, set overlay 255, and request
  sound service 0 / command 65532 unless original flag 447e suppresses it.
* Wait for the real child result. Positive signed result freezes object 16 and
  enters the real four-count delay, including same-call decrement to three.
  Negative signed result stops before the original miss continuation `209b4b`.
* Decrement the 16-bit delay with original signed-underflow semantics, install
  callback `209b05`, then stop before its unconverted display reset/body.

`tools/check-shot-swing.sh` passes 131,328 authored cases separately on host,
ASan/UBSan and Linux m68020 instruction execution. The private unchanged-original
oracle passes 4,864 cases comparing full nonstack RAM, video, callbacks, result,
delay and conditional sound requests. Randomized tests include every result and
suppression byte, plus signed delay boundaries. No child is marked complete by
this module. QEMU instruction tests are not stock Amiga timing.

## Integrated original-game evidence

The new opt-in `shot_swing_application.c` installs genuine swing parent and
trajectory handlers without changing the preserved power application or target.
The copied whole-application bridge/oracle compares original task dispatch,
state, resource queues, screen maps, sprites, palette and sound ordering.

Verified results on both normal and private-asset backends:

* Four flows: 4,434 updates and 17,981 ordered dispatches per backend, including a genuine
  trajectory-published miss reaching parent boundary `209b4b`.
* A separate hit-input flow: 1,125 updates and 4,593 ordered dispatches, reaching
  `209b05` after the actual child publishes hit and the parent runs its delay.
  The fixture presses the normal button while the actual accuracy meter is in
  range. It does not patch a result or make the child finish artificially.

The append-only tag27 mapping passed 31,104 original drawing rounds across
all 36 selections and 648 legacy-selection regression rounds. The shared
implementation is byte-identical to the disposable copy used by these tests.
Slots 0..17 retain their old meaning; additional offsets use slots 18..35.
This remains source-level integration, not a rebuilt target disk.
Reproduce the bounded integrated set with `NTM_POWER_FLOWS=4` and
`tools/check-shot-swing-application.sh` after activating the toolchain. The script
also runs the hit-input flow on each backend. Without that bound its default is
24 flows. The larger sweep also passed on both backends: 13,384 updates and
64,206 ordered dispatches per backend across all three courses, including five
genuine hits, one miss, ten canceled-world boundaries, five waiting/head-drain
cases and three earlier unsupported-input boundaries.

Authored wrapper tests separately exercise enable/dispatch/boundary contracts;
their explicitly injected result values are NOT the gameplay evidence above.

Actual ball physics, subsequent hit reset `209b05`, miss child `209b4b`, target
packaging and audible Paula playback remain outside this verified slice.
There is no new stock speed measurement or full-speed claim here. Baseline
stock first-fire disk remains unchanged.
