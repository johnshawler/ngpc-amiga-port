# Native Amiga course-introduction target

This separate target continues the interactive course selector through the real
course-title object, tournament title, skip input and cleanup for USA, Germany
and Japan. It stops at the outer owner's **round-record request**. It is a
bounded, silent native conversion, not playable golf. The original selector-only
target and its fifteen-entry exact planar cache remain separate.

**Target:** PAL A1200, 68EC020, AGA, 2 MiB Chip RAM, zero Fast RAM. Like the
selector target, this executable takes over display, interrupts and CIA B until
emulator reset. It has no OS-return path and must not be launched over unsaved
work. Physical hardware has not been tested.

## Behavior and boundaries

Joystick port 2 retains left/right navigation, first fire for acceptance, second
fire for cancel, and up plus first fire for modified/random acceptance. Initial
selection is USA. First fire also skips the currently active title or tournament
wait through the original rising-edge input path. A held button is not repeated
skip input. The test host supplies only external RetroPad input through PUAE's
hardware-input callbacks. It never writes native task state or phase into RAM.

The actual outer task runs before the selector parent. Selector exit does not
rerun the owner that tick. On the following logical update the owner increments
the normal golfer record once, allocates the intro child, and that child runs
later in the same traversal. Service order remains input, palette head, old
fills/stamps, object head, tasks, palette tail and object tail.

The introduction clears map 0, resets owned display state, queues the course
palette/background, and opens the viewport at the original delay boundary.
The next stage queues its title stamp and registers the real tag-38 object. The
shared object engine executes its one-frame terminal animation, including the
0xc0 completion/freeze semantics and deferred tile/sprite uploads. A separate
native title record prevents stale selector aliases from replacing its graphics
index, position or flags. Nothing substitutes a pre-rendered course-title image.

Expiry or first-fire skip resets the title display and queues the tournament
stamp. Its short delay and subsequent wait preserve their original scheduling.
Cleanup closes the viewport, resets owned display state, removes the child, and
lets the owner request round-record on its next traversal. **This target stops
before round-record.** The separate
[round-record target](round_README.md) continues to the still-unimplemented
tee-shot request. Cancel stops at the prior-selection
request. The standalone target begins with normal golfer 0 in bank 0 and normal
game/input mode; it does not restore save progression or linked-player state.
The core's mode-2 and linked-input branches have separate synthetic/original tests.

See [scenery contracts](../../native-game/COURSE_INTRO.md) and
[title contracts and original comparisons](../../native-game/COURSE_TITLE.md).
No TLCS-900 interpreter runs in this Amiga executable. The old scenery-only
application entry remains available for its retained boundary oracle.

## Rendering and measured limits

Input and application updates retain the CIA cadence of 241 updates per eight
nominal seconds, a two-update catch-up bound, and explicit dropped-update counts.
The private-source copy backend still requires generated sources disjoint from
destination tile/map arrays. Validation, queue ordering and the generic default
path remain intact. All nine tested target flows have **zero dropped updates**.

Selector snapshots retain the same exact-key planar cache and safe fallback.
The intro target uses the existing general `nr_scene_row_planar_words` backend
for cache misses, with only the scene-renderer translation unit built at `-O2`.
All new scenes are confirmed misses of the selector cache. A generic cached-row
optimization also returns the already-filled outside pixels for empty/offscreen
horizontal windows. Scalar-oracle tests cover closed-to-open cache transitions.

The backend choice followed measurement. The former chunky/packed fallback
could finish its first course composition only after the whole course-title
wait had ended. Changing compiler optimization alone still omitted the title.
Direct word-plane composition allows all three ordinary course-title pictures
to be displayed, without pausing tasks, extending waits or injecting state.

**This is not full-speed presentation.** Rendering presents immutable earlier
snapshots while logic continues. The rebuilt USA and Japan runs omitted the
separate scenery stage: their first actual course picture was the title. Germany and the
modified/random run presented scenery, title and tournament as separate pictures.
All ordinary titles were first shown after logic had already reached the
tournament phase. These current title-target reruns are separate from the larger
round target, which misses additional stages; see its own report.

| Flow | Accept to first verified title | First title composition | Maximum input-poll gap |
| --- | ---: | ---: | ---: |
| USA | 4.80 s | 2.584 s | 37.59 ms |
| Germany | 5.26 s | 1.463 s | 43.45 ms |
| Japan | 4.84 s | 2.624 s | 38.80 ms |
| Modified/random | 7.10 s | 1.322 s | 41.86 ms |

The random flow's first scenery composition was **2.483 s**, captured **5.80 s**
after acceptance. Germany's first scenery composition was 2.524 seconds. Their
shorter title compositions reuse unchanged rows, so these are not cold
scene-creation measurements. First ordinary tournament compositions
were 1.221, 1.240 and 1.262 seconds. Final unchanged closed frames take about
0.400 seconds; that is not initial scenery/title creation time. No unchanged
title redraw was observed, so no steady title presentation rate is claimed.

Accept-to-capture values include the original accept/random delay, rendering,
and a two-stable-frame capture guard, using nominal PAL 50 Hz. Composition times
use the emulated CIA clock. These are not first illuminated-pixel latency or
physical-hardware measurements. Short presses between input polls may be missed.

Skip tests supply hardware fire while the **logical** title wait is active, log
that phase/update, and verify that it falls from 60 updates to 9. Double-skip
also reduces the tournament wait from 60 updates to 9. They do not prove that a
press made after the user sees a delayed title will act on that same stage.
Shortened stages can be omitted or shown after cleanup; presentation logs retain
both snapshot and current logical phase rather than labeling these as timely.

## Exact checks and memory

Nine hardware-input scenarios cover ordinary acceptance for all three courses,
random acceptance, cancel, title skip on all three courses, and double skip.
There are **158 verified captures**, including mixed/repeated navigation and
actual title/tournament/transition/boundary snapshots. Each checks all 81,920
bitplane bytes, all 256 Copper colors in both AGA nibble banks, and every captured
RGB pixel including borders. All 512 palette bytes are independently resolved
from original cartridge jobs. Background/title/tournament writes and the real
title shape, source tiles, sprite positions/attributes and sprite palettes are
independently decoded from the cartridge, accounting for deferred head timing.
The final viewport is closed and stale sprites are removed through normal services.

Cartridge-free application tests add **24 real traversals** across three mappings,
normal/random/cancel, title skip, double skip, held input, mode 2 and linked skip.
The previous 12 scenery application flows remain. Both application sets pass
host, ASan/UBSan and static QEMU m68020, with generic and private-copy paths.
General direct-planar tests cover 131,072 synthetic tile rows. Scene tests retain
the independent scalar oracle plus closed/offscreen-to-open cache transitions.

The executable is **728,560 bytes**, with **726,389 ELF text bytes** and **71,112
BSS bytes**. Its ordinary 880 KiB ADF fits without requiring an HD disk. The
explicit display allocation remains 170,000 Chip bytes. Exec reports 1,188,096
free Chip bytes before that allocation and **1,018,096** afterward. The
555,120-byte selector cache stays resident. This is bounded-slice headroom, not
proof that the remaining game or audio fits; reclaimable overlays are not implemented.

After the shared changes, the separate selector-only target was rebuilt and all
three accept/cancel/random hardware flows passed their retained pixel/palette/drop
gates. Baseline selector and scenery artifacts remain in separate ignored local
directories. No cartridge, firmware, generated assets, captures or private hashes
are tracked.

## Reproduce privately

Provide the authorized cartridge at `private-inputs/neo-turf-masters.ngc` and
use the existing m68020 toolchain:

```sh
export AMIGA_BIN=/path/to/vscode-amiga-debug/bin/linux
bash native-renderer/amiga/intro_build.sh
export PUAE_CORE=/path/to/puae_libretro.so
export LIBRETRO_INCLUDE=/path/to/libretro-common/include
export KICKSTART=/path/to/authorized-A1200-3.1.rom
for scenario in course0 course1 course2 random cancel skip0 skip1 skip2 skipboth; do
  bash native-renderer/amiga/intro_run.sh "$scenario"
done
bash native-renderer/amiga/intro_test.sh
python3 -m unittest discover -s tests -p test_intro_frontend.py -v
make -C native-renderer scene scene-sanitize scene-m68020 direct direct-sanitize direct-m68020
```

The exporter resolves 43 bounded spans: full course descriptors, scenery/title/
tournament assets, title object animations/shapes/tiles and palette sources. It
rejects malformed or unsupported headers, shapes, tile destinations and spans,
and refuses output outside ignored `build/`. The verifier uses only capture names
logged by the current run, excluding stale files left by earlier experiments.
All generated artifacts remain private under `build/native-intro`.
