# Fine write dependencies and adaptive redraw investigation

This extends the independent recovery scaffold. Existing coarse event, exact-key,
packed and scalar paths remain intact. These are native renderer APIs validated
against the original game; they do not execute the game's controls, shot physics,
AI or sound routines. No interactive/full-speed golf claim is made.

## Narrow dependencies

`fine_event_renderer.c` depends on the actual selected 16-bit tile-row word,
including vertical flip, instead of every row in a referenced tile. Both bytes
of that word invalidate the dependent row. Two 32-bit column masks and selected
map-row indices replace a full map bitmap. Sprite palette-index writes invalidate
only rows with a potentially visible opaque texel from that sprite, accounting
for clipping and horizontal flip. Hidden texels may conservatively retain a
palette dependency; transparent tile words retain tile dependencies so later
writes can make them visible. Sprite descriptor/position changes still invalidate
all rows because relative chains can move later sprites.

All changed graphics bytes must be notified, and reset/load/array replacement
must invalidate caches. Latched scene registers remain checked live per row.
RGB color writes do not change stored pen indices. The cache uses 109,162 bytes
on m68020, versus 77,242 for coarse events and 143,510 for exact keys. Dense word
bits make tile writes constant-time per valid row; map masks reduce miss setup
and memory compared with a fully dense fine dependency layout.

## Adaptive selection

`adaptive_renderer.c` starts unknown rows on the packed baseline. Once a row's
registers are stable, it can use fine dependencies and dirty planar storage.
A register change selects packed drawing immediately; two stable observations
are required before caching resumes. Fallback invalidates that row's fine cache
and the caller invalidates its separate dirty-planar shadow before packed writes.
The tests explicitly cover switching back to cached output without stale pixels.
This policy is intentionally narrow: it responds to scrolling/window/register
changes, not a predictive model of all sprite/tile traffic. It is optional and
not installed as a universal default.

The adaptive cache is 111,450 bytes on m68020. Its initial baseline choice avoids
paying dependency construction on a scene that immediately scrolls. It sacrifices
some four-frame cold-start performance on stable scenes; long stable intervals
would amortize that startup, but no unmeasured throughput claim is made.

## Correctness gates

- Fine tests cover all 12,608 tile/map/sprite/palette byte positions, selected-word
  invalidation, clipping, flips, transparent sprites, relative chains, mixed
  raster writes, reset/invalid inputs, and selective counters.
- Adaptive tests cover all tile/map/sprite byte positions and live-register
  mutations, plus packed/dirty transitions and full planar byte comparison.
- Host, ASan/UBSan, and static QEMU m68020 execution pass.
- Original reference instrumentation observes actual changed graphics writes,
  reset/load and original raster draw points. Both full gameplay scripts are
  compared with original video/audio/final state and fresh repeats. No game
  assets, private snapshots or private trace hashes are published.

Both representative original-game scripts pass all 189,696,000 compared pixels.
Fine dependencies compose 88,096 and reuse 550,304 rows on the 4,200-frame fairway
script; they compose 66,231 and reuse 480,969 rows on the 3,600-frame O.B. script.
These reduce recomposed rows only modestly versus coarse events (88,679/66,910).
Adaptive selection uses packed fallback for 75,905/41,858 rows and cache requests
for 562,495/505,342 rows respectively. Its fallback does not skip drawing.
Video/audio/final state and fresh repeated traces match exactly. Coverage still
excludes putting, completed holes and wider game modes.

Verified target milliseconds per synthetic frame:

| Workload | Packed baseline | Coarse events | Fine events | Adaptive |
| --- | ---: | ---: | ---: | ---: |
| Unchanged, four-frame cold start | 2,079 | 963–965 | 933 | 1,400–1,401 |
| Scroll changes each frame | 2,078 | 2,852 | 2,730 | 2,110 |
| 32 changed tile bytes per frame | 2,079 | 1,729 | 1,085 | 1,504 |

Fine dependencies cut the changed-tile case by about 37% versus coarse events,
recomposing 188 rather than 353 rows, and by about 48% versus packed redraw.
On the actual original traces the reuse gain is much smaller, so the synthetic
speedup must not be applied to golf. Adaptive selection cuts the fine cache's
scrolling regression substantially: 2,110 ms versus 2,730, but remains about 1.5%
slower than the packed baseline. Its safe cold-start choice costs stable-scene
throughput relative to unconditional fine caching. All paths are still far from
20 ms PAL redraws; no universal speed win or game FPS is established.

An earlier exploratory adaptive binary cached its first unknown row. It reached
~960 ms on stable input but ~2,269 ms scrolling. The retained policy starts on
packed rendering to minimize the scrolling penalty; that earlier capture remains
local and is not mixed with the final table. All final target timings repeat
identically on a fresh boot.

## Timing scope and reproduction

The version-9 target fixture compares packed baseline, coarse events, fine events,
and adaptive fine/packed selection in the same binary. Four frames per pass;
stable passes run forward and reverse, followed by changing-scroll and 32 changed
tile-byte/frame workloads. Every variant sees identical input mutations. Checksums
are outside timing, verified against independent synthetic pixels. Mutations
restore the tile bytes after four frames; separate exhaustive tests validate
intermediate changes. The mutation schedule is not a replay of original writes.

Stock PAL A1200 emulator: 68020/AGA, 2 MiB Chip, zero Fast/Slow RAM, cycle-exact
CPU/memory, full 320x256 eight-plane display DMA. The 160x152 redraw is timed by
chained CIA-B counters calibrated against the PAL beam. Host time is not used.
The full DMA viewport is heavier than a potential cropped display. This does
not establish physical hardware performance or golf FPS.

```
make -C native-renderer fine_event fine_event-sanitize fine_event-m68020
make -C native-renderer adaptive adaptive-sanitize adaptive-m68020
bash tools/build-reference.sh --scene
# Run both gameplay scripts twice and compare with original traces.
bash native-renderer/amiga/build.sh --adaptive-benchmark
bash native-renderer/amiga/run.sh --adaptive-benchmark
python3 -m unittest discover -s tests -v
```

Use the existing documented official cross tools, PUAE and private firmware
variables. Public CI uses no ROM and does not reproduce private gameplay or
stock emulator captures. A future native game loop must funnel all graphics
writes through the validated notification contract; these read-only reference
hooks are not a substitute for translating that game logic.
