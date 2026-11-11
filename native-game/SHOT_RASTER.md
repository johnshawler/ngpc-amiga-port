# Native normal banner raster service

`shot_raster.c` is the bounded normal banner-exit raster dependency. It converts
registration `0x20042e` with divisor 1 and callback `0x20c620`, removal `0x200453`,
and the row-local scroll effect. It does not implement a CPU, general timer,
interrupt controller, shot physics, audio or an Amiga presentation target.
The previous tee target and its baseline remain untouched.

## Owned state and lifetime

`ntm_shot_raster` holds the two existing source bytes, `lower_scroll` at original
`0x4441` and `upper_scroll` at `0x4443`, the relevant Timer0/GPU control values,
the callback selection, counter and game interrupt-shadow byte `0x6c27`.
Callback zero represents the original immediate-RETI handler `0x200471`; callback
one represents the banner handler. Source bytes are not assets or invented
animation progress.

Installation stores the banner callback, stops Timer0 with `timer_run & 0x8e`,
resets its counter, sets mode zero and divisor one, restarts Timer0 and enables
GPU control bit 6. The normal priority request is level 3. Removal stores the
RETI callback, stops/reset Timer0, disables its priority and clears GPU bit 6.
Removal leaves timer mode/divisor and both source bytes unchanged. GPU control
accepts only its actual readable bits 6 and 7; impossible low bits reject before
mutation. The unchanged control bits are preserved.

The game's removal helper also writes `interrupt_shadow &= 0xf0` at `0x6c27`
and writes that byte to the priority port. Installation leaves this shadow byte
unchanged in the pinned original oracle: its BIOS INTLVSET implementation uses a
separate priority cache. The native state preserves this distinction rather than
normalizing away a full-RAM comparison difference. General BIOS priority caches
and unrelated interrupts are not implemented by this helper.

Neither installation nor removal writes raw plane-1 horizontal scroll. The
banner owner remains responsible for its separate explicit scroll reset during
cleanup. Replacing registration with a no-op, immediately changing scroll on
install, or exiting the child by time alone would not implement this contract.

## Exact row order

The existing unchanged original core was exercised through its real timer,
graphics latch/draw and interrupt functions. It latches delayed GPU controls
before rendering a row, increments its raster counter, then dispatches the
Timer0 interrupt. The callback therefore reads the **next** raster counter:

- eligible HINT/latch boundaries are rendered rows 0–150 and `screen_period`;
- rows 151 through `screen_period - 1` retain the previous latched controls;
- the callback writes `upper_scroll` when its current raster counter is 0–72,
  and `lower_scroll` when it is 73–255;
- at the final row, the counter wraps to zero and the callback primes the next
  frame's upper scroll.

In a steady frame this produces visible rows **0–72 upper, 73–151 lower**.
The first row immediately after installation is different: it retains the
previous raw scroll until the first eligible interrupt has executed. No first-row
value is guessed or silently replaced by a steady-frame approximation.

`ntm_shot_raster_step` exposes the actual latch → shown-row → next-counter IRQ
order. Call it for every line, including blanking. It supports screen periods
151–255, accepts independent raw/latched/shown storage and validates the entire
request before mutation. It assumes this bounded interrupt is serviced before
the following scanline. The original tested callback takes at most 48 CPU clocks
against a 515-clock line, but this is an isolated reference timing observation,
not a complete-game timing guarantee.

The source bytes are read at dispatch time. A caller can therefore represent
source changes between individual rows. A frame-end scalar cannot prove the
same result, and is not a substitute for the row service.

## Native renderer and AGA mapping

The existing native scene renderer already accepts state for an individual row.
The separate shot frontend can snapshot an immutable 152-byte scroll plan,
copy the base `nr_scene` for each row, set only `scroll_x[0]` to that row's value,
and invoke the existing scalar/cached/direct-word-plane compositor. Snapshot
metadata and dirty-row decisions must include this row plan. Sprites, plane 2,
window, palette and vertical scroll must retain their own original state.

The existing eight-bitplane AGA scanout can then show the composed picture with
its ordinary Copper list. Source row 73 maps to bitmap row 125 with the current
52-row inset, or nominal PAL beam line 169 with the current display start.
There is no need to run the original callback code on the Amiga beam.

A single Copper `BPLCON1` horizontal split is not equivalent: the current eight
bitplanes encode already-composited pen indices, not independent original
layers. Such a shift would move sprites/plane 2 too, or corrupt pen bits if only
odd/even planes were shifted. A future hardware-layer optimization would need
its own layer separation and complete source/pixel proof. No such shortcut is
used or claimed here.

## Verification

- Public C tests exhaust **16,777,216** raster/source-byte combinations,
  **65,536** control states including atomic rejection, and **152,832** complete
  line boundaries. Installation/removal, blanking, first-frame latch retention,
  steady frames, no eager scroll write and invalid arguments are covered.
- A separate synthetic renderer test checks all **24,320 pixels** and all eight
  bitplanes with a sprite crossing the split and an independently patterned
  second plane. Only plane 1 moves; the sprite and plane 2 remain anchored.
- Both suites pass on host, ASan/UBSan and QEMU m68020. The freestanding m68020
  helper has no unresolved symbols.
- The unchanged-original oracle passes **1,024 registration/removal pairs**,
  **65,536 direct IRQ cases**, and **12,160 actual timer/latch rows** in 32
  sequences. It checks vector, timer internals, GPU control, `0x6c27`, raw scroll,
  all CPU registers and nonstack RAM for the IRQ, screen periods 151/152/198/255,
  and source changes around the split and final visible lines.

The probe's reads of write-only timer ports `0x22/0x24` return 4, not the stored
mode/divisor. The oracle therefore resolves existing unstripped core symbols by
name and reads the actual timer state. It calls original timer/graphics functions
without rebuilding the shared core or patching its instructions. These are
isolated service checks, not a fresh live whole-game raster replay or a stock
A1200 hardware-input/presentation result.

Run the public tests with:

```sh
python3 -m unittest discover -s tests -p test_shot_raster.py -v
```

With the existing authorized private inputs, unstripped screen-probe core and
m68k toolchain, run `bash tools/check-shot-raster.sh`. Its authoritative private
report is `build/shot-raster-check/original.json`; generated resources and logs
remain under ignored `build/`. No cartridge, firmware, captures or private
hashes are tracked.
