# Exact row composition reuse

`nr_scene_row_reused` is an optional cache above the retained sprite-cached
compositor. Its per-row key contains the live window, scroll, plane-order and
background indices; every visible plane descriptor and selected character row;
and each relevant sprite's resolved position, descriptor, palette index and
character row. The key is compared byte-for-byte, never by a hash. Identical
keys reuse the previously composed pens. Palette RGB writes remain separate.

There is no frame-boundary assumption: a change before any original scanline is
observed at that row. Transparent sprites and disabled/relative anchors retain
the baseline's rules. The maximum key is 758 bytes (42 plane tiles and 64 sprites)
inside a 768-byte slot. The 68020 ABI cache uses 143,510 bytes, including its own
sprite cache. Initialize it once; inputs and output must not alias cache storage.
This large memory-for-work trade-off is optional and is not a full-game RAM budget.

## Correctness and coverage

Randomized and directed scene tests compare the new path to the independent
oracle and retained baseline. Tests mutate every character-row byte while
visible, exercise every sprite/palette byte change, and construct the maximum
758-byte key. Host sanitizers and explicit m68020 QEMU pass. Original fairway and
O.B. traces compare reused pens with original pixels at every raster row; all
video/audio/final-state and instrumented-repeat checks continue to pass.

This avoids composition only when the exact dependencies match. Changes that do
not alter visible colors can conservatively cause a miss. It does not infer
scene semantics, invent intermediate frames or skip original game execution.

## Equivalent target benchmark

Build and run `amiga/build.sh --composition-benchmark` and
`amiga/run.sh --composition-benchmark` with the existing tool/core/private-firmware
environment. Outputs are isolated under `build/native-composition-benchmark`.
The version-4 protocol retains the stock PAL A1200 profile, zero Fast RAM,
320x256 eight-plane AGA DMA and calibrated CIA timing from prior benchmarks.

Both variants use identical dirty planar storage and synthetic 160x152 inputs.
Four-frame A/B and B/A passes include one cold composition-cache frame. A second
pair changes plane scroll each frame to force misses. A final pair measures
composition without pixel storage. Every output digest and cache count is checked
against the independent pixel oracle. No emulator wall time is used.

| Workload | Baseline ms/frame | Reuse cache ms/frame |
| --- | ---: | ---: |
| Stable full redraw, one cold frame in four | 1,982 | 1,408 |
| Scroll changes every frame | 2,247 | 3,149 |
| Stable composition only | 1,740 | 1,167 |

Stable redraw is about 29% quicker, but continuous invalidation is about 40%
slower. Keep the baseline available for scrolling/high-change workloads. A
future dispatch policy needs its own validation; this API does not silently
switch paths or conceal cache misses.

The cache is a correctness-preserving experiment, not proof of full-speed golf.
Even the stable synthetic result is far above a 20 ms PAL budget and excludes
native game logic/audio. Key collection, Chip-memory comparisons and copying
still cost time. More direct native planar/tile hardware rendering or event-driven
invalidation is required. Cropped display DMA and physical hardware remain
separate, unmeasured conditions.
