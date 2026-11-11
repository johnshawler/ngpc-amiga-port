# Controlled stock-PAL renderer timing

This measures a synthetic 160x152 scene redraw on the pinned PUAE emulator's
stock PAL A1200 profile: 68020, AGA, 2 MiB Chip, zero Fast/Slow RAM, CPU/memory
cycle-exact settings enabled and no CPU throttle adjustment. It is not a
physical-hardware result or a game-frame-rate measurement.

## Reproduction

Use the same official toolchain, PUAE core and private Kickstart inputs as the
existing display fixture, then run:

    bash native-renderer/amiga/build.sh --benchmark
    bash native-renderer/amiga/run.sh --benchmark

The benchmark gets a separate ignored `build/native-benchmark` directory. The
host runs 9,000 emulator refreshes to allow slow test code to finish. That number
includes boot/wait time and is NOT the benchmark duration. `CAPTURE_FRAMES` can
change the capture deadline; the verifier rejects incomplete measurements.

The fixture owns hardware until emulator reset. It disables OS/custom interrupts
and reclaims CIA-B timers A and B as a chained 32-bit E-clock downcounter. Timing
uses 709,379 PAL E-clocks per second and coherent timer reads. A measured beam
frame must calibrate to 19.8–20.2 ms. Host wall time is never used.

## Equivalent workload and controls

- The synthetic tile maps, character data, sprites, scroll/window settings and
  palette are identical between the cached and uncached paths.
- Both render every row and write the same full eight-plane 160x152 backbuffer.
- The displayed scene stays fixed: eight AGA planes fetch a 320x256 bitmap using
  16-bit fetch mode. That DMA load is deliberate and identical, but heavier than
  a narrowly cropped 160x152 display. Future game display arrangements can differ.
- Stable A/B then B/A passes each redraw four frames, including one cold cache
  build for each cached pass. The two orderings must agree within 1%.
- One additional pair toggles a sprite palette byte every row, forcing cache
  rebuilds on all 147 active rows. This is an artificial worst case, not observed
  game cadence. The changed sprite is disabled, preserving identical pixels.
- Separate four-frame passes measure composition alone and planar stores alone.
  A minimal volatile sink keeps row outputs observable. Output digests are taken
  outside the timed interval and checked against independent synthetic pixels.
- Full capture, Chip bitmap and palette validation still runs. Timer record,
  clock, output, cache-count and incomplete-run corruption tests are included.

## Initial measured result

Rounded representative measurements from the completed version-2 benchmark:

| Workload | Uncached ms/frame | Cached ms/frame |
| --- | ---: | ---: |
| Stable full redraw | 3,183–3,184 | 2,867–2,868 |
| Forced change every row | 3,183 | 4,500 |
| Composition only | 2,059 | 1,739 |
| Planar stores only | 1,121 | not applicable |

The stable cache saves about 10% of total redraw time. Rapid invalidation makes
it about 41% slower than the uncached path in this deliberately bad case. The
remaining composition and scalar chunky-to-planar loops dominate; neither is
close to a roughly 20 ms PAL frame budget. Even these times exclude game logic,
audio, input scheduling and a dynamic display flip.

The test is GCC -Os portable C with all memory on Chip and active eight-plane DMA.
It does not estimate the performance of a future assembly/Blitter/dirty-tile
renderer. A native planar or dirty-region architecture and faster pixel packing
are required; this cache alone cannot deliver full-speed gameplay. See local
`timing.json` for exact ticks and the repeat capture for reproducibility.
