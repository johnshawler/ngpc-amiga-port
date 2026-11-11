# Direct planar experiments: correct, not selected for speed

Two experimental CPU tile compositors are retained alongside the faster baseline:

- `nr_scene_row_planar` deinterleaves each NGPC 2-bit tile row into two eight-bit
  masks, derives palette bits and merges directly into the destination planes.
- `nr_scene_row_planar_words` combines four planes per 32-bit word in a 160-byte
  row scratch area, then emits the final plane bytes once. Hot mask/merge helpers
  are forced inline with GCC-compatible attributes. It avoids chunky pixel
  composition, the final chunky-to-planar transpose and a large row-reuse cache.

Both honor scroll wrap, clipping, windows, transparency, sprite chains and order.
They use the existing small sprite cache. Neither uses the Blitter or hardware
sprites. Eight-plane AGA display DMA is active, but that is not rendering
acceleration. These are experimental APIs, not new defaults.

## Validation

`make direct direct-sanitize direct-m68020` tests every possible 16-bit tile row
with both horizontal directions (131,072 cases), palette variation, vertical
selection and buffer guards. Existing randomized scene tests check word-packed
planes against the independent pixel oracle, including untouched rows/padding.
The freestanding 68020 object has no undefined symbols.

The read-only original raster adapter compares both direct buffers with scalar
planes on every visible row, while the pixel oracle checks the original game.
Both fairway and O.B. scripts retain exact video/audio/final-state and repeat
checks. These cover first-hole examples, not the entire game.

## Controlled target result

Use `amiga/build.sh --direct-benchmark` and `amiga/run.sh --direct-benchmark` with
the existing official toolchain/core and private firmware. Version-6 timing runs
four variants forward then backward on identical inputs, followed by an identical
scroll-changing workload. CIA calibration, all outputs and counters are checked.
Artifacts are separate under ignored `build/native-direct-benchmark`.

Stock emulated PAL A1200, 2 MiB Chip, zero Fast, with the same 320x256 eight-plane
AGA display DMA as previous comparisons; synthetic render size is 160x152:

| Variant | Stable ms/frame | Scroll changes each frame ms/frame |
| --- | ---: | ---: |
| Retained chunky compositor + packed conversion | 2,079 | 2,079 |
| Bytewise direct planes | 3,643 | 3,643 |
| Word-packed direct planes, hot helpers inline | 2,464 | 2,459 |
| Optional exact row reuse + dirty storage | 1,411 | 3,155 |

The word-packed implementation improves substantially on naive direct stores but
still takes about 18% longer than the packed baseline. Eliminating conversion
alone does not repay tile mask/setup/merge costs in these C implementations.
Neither direct variant is promoted as a performance improvement. Keep packed
conversion for high-change scenes and optional row reuse for stable scenes;
no automatic dispatch is introduced.

## Remaining direction and limits

This rules out these two CPU implementations under the measured conditions, not
all direct-planar rendering. Blitter batching, predecoded tile surfaces, hardware
sprite use and event-driven dirty scheduling require separate implementations
and verification. Frame batching must first establish that raster-time changes
are handled; a frame-end snapshot is insufficient. A cropped display, assembly
and physical hardware may change results and have not been measured here.

All times exclude native game logic/audio and remain far above a roughly 20 ms
PAL frame budget. No playable or full-speed port claim is supported.
