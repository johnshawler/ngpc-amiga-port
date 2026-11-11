# Prepared-plane Amiga Blitter batching experiment

This is a separate synthetic hardware experiment. It does not replace the native
scene renderer or integrate Blitter rendering into Neo Turf Masters. The result
is intentionally allowed to be negative: batching must outperform an equivalent
CPU path before it is a useful architectural direction.

## Reproduce

Use the same official m68k GCC, vscode-amiga-debug `elf2hunk`/`exe2adf`, pinned
PUAE shared core and locally authorized private A1200 Kickstart as the existing
Amiga fixtures. With `AMIGA_BIN`, `PUAE_CORE`, `LIBRETRO_INCLUDE` and `KICKSTART`
set, run:

    bash native-renderer/amiga/blitter_build.sh
    bash native-renderer/amiga/blitter_run.sh
    python3 native-renderer/amiga/blitter_test.py

All outputs, including the local ROM copy, stay under ignored
`build/native-blitter-benchmark`. Do not publish the private ROM or ROM-derived
material. This source and its displayed and timed fixtures use synthetic data.
`CAPTURE_FRAMES` changes the capture deadline, not the timing interval; 6,000
emulator refreshes were sufficient for the recorded version-2 run. The default
allows 9,000. An unfinished run fails validation.

## What is equivalent

Every timed frame performs an eight-plane cookie-cut of a 160×152 bitmap. For
each destination bit the result is `(mask & source) | (~mask & old_destination)`.
Two different prepared source/mask sets alternate for 16 frames. Each pass starts
from the same patterned backdrop. The source pens, transparent pixels and tile
flips are synthetic and deterministic. All variants perform the same writes and
produce exactly the same 24,320 destination bytes.

The seven variants are:

1. CPU 32-bit masked loop, built with the existing `-m68020 -Os` flags.
2. The same CPU work unrolled four longwords at a time.
3. 1,520 actual 16×8 Blits, each covering two adjacent 8×8 tiles in one plane.
4. 152 actual 160×8 tile-row Blits, one row strip per plane.
5. Eight actual 160×152 Blits, one full plane each.
6. One actual 160×1216 Blit, covering the contiguous eight-plane allocation.
7. The same single batch with Blitter priority enabled while the CPU waits.

The Blitter uses A/B/C/D DMA, minterm `0xCA`, full word masks, unshifted pointers,
and ECS/AGA extended size registers. All DMA buffers come from an explicit
300,000-byte `MEMF_CHIP | MEMF_CLEAR` allocation. The narrow tile-pair path uses
18-byte source/destination modulo; larger groups use contiguous source storage.
Every timed frame waits for DMA completion. There is no CPU/Blitter overlap test.

The variants run forward and reverse. Each matching pair must agree within 1%.
All run with a fixed 320×256 eight-plane AGA display, 16-bit fetch mode, and the
same Copper list. This display load is heavier than a cropped game viewport.
Blitter priority is off except for the explicitly labeled final control.

## Timing and validation

The target is the pinned PUAE stock PAL A1200 profile: 68020, AGA, 2 MiB Chip,
zero Fast/Slow RAM, CPU, memory and Blitter cycle-exact settings enabled, immediate
Blits disabled, and no throttle adjustment. Timing uses chained CIA-B timers at 709,379 E-clocks/second. A measured
beam frame must fall within 19.8–20.2 ms. Host wall time is not a measurement.

Source and mask creation, backdrop setup, checksums and exact comparisons are
outside the timed rendering interval. The intentionally simple synthetic source
generator's cost is reported separately. Most importantly, the wide Blit sources
are **already gathered, converted, and contiguous**. No measured number includes
turning arbitrary tile descriptors into those batches. This is a prepared-source
best-case experiment, not a renderer benchmark.

Each pass compares every output byte to a separate bytewise target-side
reference. A Python indexed-pixel oracle independently checks all four source and
mask buffers, the full backdrop, the final destination and every recorded output
digest. It checks DMA state, completion, operation counts, timing and machine
settings. The existing display verifier additionally checks every captured RGB
pixel, all display bitplanes, and all 256 AGA colors. The wrapper reports the
actual larger allocation rather than the display fixture's 100,000-byte prefix.

`blitter_test.py` runs five test groups with corruption subcases covering report
protocol, CIA calibration, pointers, source/mask/destination bytes, checksums,
DMA state, an unfinished Blit, operation counts, ordering sensitivity, incomplete
headers and wrong machine configuration. It does not need a ROM or emulator.

## Results and interpretation

The final version-2 binary measured the following milliseconds per prepared
frame. Forward/reverse pairs differ by less than 0.002%; a second fresh emulator
boot produced an identical complete timing report.

| Method | Blits/frame | ms/frame |
| --- | ---: | ---: |
| CPU longword loop | 0 | 37.634 |
| CPU unrolled longwords | 0 | 36.370 |
| Blitter two-tile groups | 1,520 | 92.626 |
| Blitter full tile rows | 152 | 47.725 |
| Blitter whole planes | 8 | 44.583 |
| Blitter all planes, normal priority | 1 | 44.339 |
| Blitter all planes, priority enabled | 1 | 35.156 |

CIA calibration was 20.0246 ms per PAL frame. Prepared source, mask and backdrop
generation separately took about 5.87 seconds in the simple scalar fixture code;
that is not a per-frame conversion measurement. Exact ticks, provenance and
validation results are recorded in `blitter_results.json`; full local captures
and machine logs are under the ignored build directory.

Batching sharply reduces small-Blit setup overhead, but normal-priority batching
is still about 21.9% slower than the unrolled CPU. Giving the single large Blit
priority makes it only about 3.3% faster than that CPU baseline, and it still
requires roughly 35 ms before any composition or game work. The priority control
also gives no evidence of productive CPU overlap. These are measurements of this
compiled C harness, not chipset throughput guarantees.

Even a large improvement over small Blits is not sufficient evidence of game
acceleration. The CPU baseline is already working on the same prepared planes,
with no scene composition, palette conversion, clipping, arbitrary tile
gathering, shifted placement, sprite ordering, game logic, audio, input or display
flip. None of these measurements establishes full-speed Neo Turf Masters or
physical-hardware performance.
