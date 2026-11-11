# Packed planar and dirty-block rendering

Two optional APIs retain the original scalar conversion as a measured baseline:

- `nr_store_scene_row_packed`: transpose eight 8-bit pens into eight plane bytes
  using two 32-bit words and a fixed shift/mask network. No 64-bit arithmetic,
  lookup tables, heap allocation or undefined external symbols are needed.
- `nr_store_scene_row_dirty`: compare eight-pixel blocks with a persistent shadow
  and pack/store only changed blocks. The 68020 cache uses 24,944 bytes. Initialize
  it before use; explicitly invalidate after external writes to its bitmap. A
  destination pointer/stride change invalidates automatically. Palette-only
  changes need no pixel invalidation because the bitmap stores color indices.

This is dirty conversion/storage, NOT dirty scene composition. Every source row
is still composed; skipping composition safely requires a separate dependency
model for scroll/window/tile/sprite changes and raster-time state.

## Correctness

65,536 randomized rows, with exhaustive first-two-pen combinations, compare
scalar, packed and dirty buffers. Repeated rows check skipped conversion; tests
cover invalidation, changed destination/stride, guards and padding. Host ASan/
UBSan and explicit m68020 QEMU pass; the freestanding object has no unresolved
symbols. Run `make planar planar-sanitize planar-m68020` in this directory.

The original raster adapter also compares dirty and scalar bitplanes for every
row while both native scene paths are checked against original pixels. The
fairway and O.B. scripts retain video/audio/final-state and repeat validation.
Commercial data, screenshots and derived trace hashes remain private.

## Controlled stock target timing

Run `amiga/build.sh --planar-benchmark` and `amiga/run.sh --planar-benchmark` using
the existing tool/core/firmware environment. The version-3 benchmark is separate
from the preserved version-2 timing fixture. All variants share the sprite-cached
compositor, bitmap, output and active 320x256 eight-plane AGA DMA display. Timers,
calibration and constraints are as described in `amiga/BENCHMARK.md`.

Four-frame passes run scalar/packed/dirty, then dirty/packed/scalar. Dirty passes
include one cold frame and three unchanged frames. A separate conversion test
includes identical pen-row generation overhead; its worst-case form changes all
blocks every frame. Independent output digests and block counts are checked.

| Equivalent synthetic workload | Scalar ms/frame | Packed ms/frame | Dirty ms/frame |
| --- | ---: | ---: | ---: |
| Full redraw, stable scene | 2,864–2,865 | 2,076–2,077 | 1,994 |
| Conversion plus row generation, unchanged | 1,210 | 424 | 343 |
| Conversion plus row generation, every block changes | 1,211 | 424 | 655 |

Packed conversion reduces total redraw time by about 27.5% without extra cache
memory. Dirty storage reaches about 30.4% less total time in the stable test but
costs more memory and is slower than packed conversion when every block changes.
It is therefore an explicit option, not an unconditional replacement.

Both remain far above a roughly 20 ms PAL budget. These are emulated synthetic
renderer times, not physical-hardware performance or golf FPS. Game logic and
audio are excluded. The full 320x256 display's DMA load is heavier than a cropped
160x152 viewport. Native planar/dirty scene composition remains the next major
bottleneck; these improvements do not establish full-speed feasibility.
