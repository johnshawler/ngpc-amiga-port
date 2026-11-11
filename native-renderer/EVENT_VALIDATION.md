# Explicit graphics-write dependency experiment

This is new recovery work, not recovery of the lost translated runtime. Existing
renderers remain available and unchanged. No interactive golf or full-speed claim.

## Contract and implementation

`event_renderer.c` stores row pixels plus exact tile-ID and map-entry dependency
bitsets. A changed tile byte invalidates every valid row depending on that tile;
a changed map byte invalidates dependent rows. Sprite descriptor/palette changes
conservatively invalidate all rows because relative sprite chains can move later
sprites. Thirteen latched scene registers are compared at each row, so raster
scroll/window/background changes are not assumed to occur only at frame borders.
RGB palette changes need no pen invalidation. Whole-tile dependencies intentionally
invalidate more rows than the exact-byte key cache.

Every changed tile/map/sprite byte MUST be notified, and reset, load, bulk input
replacement or switching streams requires invalidation. Unannounced memory writes
are not safe. The reference adapter hooks the actual graphics write entry point,
compares the old byte (including palette masking), and invalidates on reset/load.
This remains instrumentation of the reference core, not a new native game writer.
The native cache uses 77,242 bytes on m68020 (77,244 host ABI), versus 143,510 for
the previous exact-key cache. It avoids recollecting all visible dependencies on
hits but still copies a 160-byte row; packed/dirty storage remains separate.

## Exactness

Host, ASan/UBSan and QEMU m68020 checks mutate every one of 8,192 tile bytes,
4,096 map bytes and 320 sprite/palette bytes, checking all 152 rows against the
uncached independent path after each write. They also exercise 5,000 live-register
changes and invalid offsets. The suite covers all 12,288 tile/map byte positions,
not every possible multi-byte scene configuration.

The two representative original-game scripts pass exact scanline comparison:

| Private trace | Frames | Compared pixels | Event composed rows | Reused rows | Changed-byte notifications |
| --- | ---: | ---: | ---: | ---: | ---: |
| First-hole drive to fairway | 4,200 | 102,144,000 | 88,679 | 549,721 | 131,284 |
| Short shot to O.B. | 3,600 | 87,552,000 | 66,910 | 480,290 | 130,704 |

That is 86.1% and 87.8% reuse, lower than exact-key reuse of 92.2% and 94.3%.
Both traces preserve every original video/audio digest and final serialized state;
fresh instrumented repeats match exactly. Traces, captures and hashes remain local.
Putting, completed holes and broader modes remain uncovered.

## Stock target measurement

`build.sh --event-benchmark` / `run.sh --event-benchmark` use the same stock PAL
68020/AGA 2 MiB Chip / zero Fast profile as earlier tests. CIA-B chained timers
are calibrated against a PAL beam frame. All allocations are Chip memory, with
full 320x256 eight-plane display DMA active; measured rendering is 160x152.
Four frames per pass, reverse-order checks, final output digest
verified independently, and fresh-boot repeat. This is emulator target time, not
host wall time, physical hardware evidence, or golf FPS.

Modes include unchanged input, scrolling every frame, and 32 actual tile-byte
changes per frame. Every variant receives the same tile mutations; event variants
also pay explicit notification cost. Four frames restore those bytes, allowing
independent final-output verification. The per-byte exactness suite above checks
intermediate mutations. This synthetic mutation stream is NOT the original game's
write-event timing or distribution, so neither hit rates nor these times establish
original-game throughput.

| Workload | Packed baseline | Exact-key + dirty | Event + dirty | Event forced full invalidation |
| --- | ---: | ---: | ---: | ---: |
| Unchanged (one cold, three warm) | 2,086 ms | 1,446 ms | 962 ms | 2,571–2,573 ms |
| Scroll changes each frame | 2,085 ms | 3,251 ms | 2,856 ms | 2,861 ms |
| 32 changed tile bytes per frame | 2,087 ms | 1,583 ms | 1,728 ms | 2,602 ms |

The event path is about 54% faster than packed redraw, and 33% faster than exact
keys, only in the unchanged synthetic case. With the changed-tile workload it is
about 17% faster than packed redraw but 9% slower than exact keys because whole-tile
invalidation recomposes 353 rows versus 188. On continuous scrolling it is slower
than the packed baseline. All remain far from a 20 ms PAL frame budget. There is
no general acceleration claim, no selected default change, and no native game-loop
speed result. Detailed target counters stay in the ignored local timing report.

## Reproduction

```
make -C native-renderer event event-sanitize event-m68020
bash tools/build-reference.sh --scene
# reference/run.py --scene-check with both gameplay input scripts, twice
# reference/compare_scene_trace.py compares original/observed/repeated JSONL
bash native-renderer/amiga/build.sh --event-benchmark
bash native-renderer/amiga/run.sh --event-benchmark
python3 -m unittest discover -s tests -v
```

Use the documented official cross-tool and emulator environment variables. No
private ROM or capture is needed for the public source tests. This optional path
must not replace the general renderer until all native graphics writers obey its
notification contract. Scrolling/adversarial workloads require a fallback policy.
