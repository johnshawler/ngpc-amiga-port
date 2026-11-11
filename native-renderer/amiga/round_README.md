# Native round-record continuation on stock PAL A1200

This is a separate bounded target. `round_build.sh` produces a native m68020
Amiga executable and ordinary 880 KiB ADF that runs the selector, scenery,
course title, tournament title and fresh normal round-record children. The
outer owner resumes after actual round-record cleanup and stops at its tee-shot
request, original address 0x208753. **Tee-shot, shot and hole simulation remain
unimplemented. This is not playable golf or a full-speed port. There is no audio.**

The target starts normal single-player golfer 0 with missing/no-result course
records. It does not restore save progression or support linked/alternate game
modes. The core supports six normal golfer resource selections; only golfer 0
is exercised by this standalone hardware frontend. Unsupported record variants
remain explicit boundaries, rather than guessed substitutions.

The selector-only and title-only targets retain their entry points and bounds.
The previous verified title artifacts were preserved separately before this
work. After the shared changes, all nine rebuilt title-only hardware flows passed
their retained gates (158 captures), including actual ordinary title presentation.
The rebuilt selector-only accept/cancel/random flows also passed (24 captures).
No cartridge, firmware, generated asset, screenshot, trace or private hash is tracked in the source repository.

## Native lifecycle and display

Round-record uses its real scheduled outer task, drawing child and blinking
object child. Preparation runs the converted 0x204a88 helper. Font and footer
stamps execute immediately, while header, course, golfer and label stamps use
the original queue and next drawing-head timing. Missing-score text copies glyph
map cells from the live font map. It is not replaced with an image or a font of
our own. Palette commands are independently resolved from the cartridge.

The third outer callback opens the viewport and stores delay 239 without
immediately decrementing. Normal cleanup follows 240 later updates, or the
original first-fire skip test. The blink child retains its immediate initial
countdown, 48-sprite shape, terminal animation and five-update hide/show cycle.
Cleanup closes the viewport, removes descendants, resets display state and
removes the outer child. The owner stops on its next scheduled traversal.

No task is paused and no original wait is extended for rendering. The host
supplies only RetroPad input through the emulated Amiga joystick registers. It
reads diagnostics and captures memory/video; it never patches application state.

The CIA pacing remains 241 logical updates per eight nominal seconds with the
existing two-update catch-up bound. The record scenes miss the exact-key selector
cache and use the general direct word-plane compositor. Immutable snapshots
prevent tearing but do not keep presentation in step with logic. Every report
records both the presented snapshot phase and the current logical phase.

## Actual presentation and limits

All 13 tested flows reached their intended boundary with **zero dropped logical
updates**. There are **288 verified captures**. That does not mean all logical
pictures were presented. In the new round target:

- All three ordinary courses presented title, tournament and round-record, but
  **missed the separate scenery picture**.
- An earlier tested round binary missed the USA/Japan titles instead. A later
  guard-only rebuild changed presentation alignment enough to show those titles.
  Those earlier misses remain evidence that the current slow snapshot pipeline
  does not guarantee presentation of every short stage.
- Modified/random presented all four of those stages.
- Title skip and double skip omitted additional short stages. The logs and JSON
  retain those omissions instead of inferring presentation from logical visits.
- Record skip was injected during the logical record wait. All three courses and
  held-fire reduced that wait to nine updates. Their eventual record picture was
  first presented **after logical cleanup**, not while its input stage was active.
  This does not prove that a press made after seeing the delayed picture would
  act on that same stage.

| Ordinary flow | Accept to first verified record | First record composition | Maximum input-poll gap |
| --- | ---: | ---: | ---: |
| USA | 8.76 s | 2.684 s | 35.59 ms |
| Germany | 9.04 s | 2.705 s | 40.47 ms |
| Japan | 8.92 s | 2.704 s | 40.04 ms |
| Modified/random | 11.14 s | 2.724 s | 43.44 ms |

The first normal/random record snapshots were 82–84 logical updates old when
captured, although the live logic was still in the record wait. Across all 13
flows the largest observed input-poll gap was 43.44 ms. Short presses between
polls can be missed. Blink pictures can also be omitted or displayed late.

Accept-to-capture figures include original preceding delays, composition and a
two-stable-frame capture guard. They use nominal PAL 50 Hz. Composition uses the
emulated CIA clock. These are not first illuminated-pixel latency, physical
hardware measurements or a steady rendering-rate claim. The title-only target's
previous timing figures do not describe this larger binary.

## Verification and memory

Each recorded capture checks all 81,920 displayed bitplane bytes, all 256 Copper
colors in both AGA nibble banks and every RGB pixel, including borders. All 512
palette bytes are independently resolved from original cartridge jobs. The
resource verifier independently decodes source stamps, narrow and wide map
rows, glyph copies, blink animation/shape/tile pointers and sprite coordinates,
attributes and palettes. It checks original scene controls and deferred head
visibility separately from current object metadata. It reads only captures
listed by that run, not stale files from prior experiments.

The hardware matrix includes ordinary selection of all three courses, random
acceptance, cancel, title skip on all courses, double title/tournament skip,
record skip on all courses and held record-skip input. Public synthetic tests
cover opt-in export, all three course/six golfer resource bindings, malformed
inputs, the independent narrow/wide decoder and blink geometry/hiding. The
24-flow native application test and 65,536-state object test also run with generic
and private-copy paths on host, ASan/UBSan and static QEMU m68020.

The executable is **803,592 bytes**, with **800,685 ELF text bytes** and **71,716
BSS bytes**. Its standard DD ADF is 95.3% full. Explicit display allocation remains
170,000 Chip bytes, including the round target's enlarged 512-byte diagnostic
header. Exec reports 1,113,200 free Chip bytes before that allocation and
**943,200 bytes afterward**. The 555,120-byte selector cache remains resident.
This is headroom for the bounded slice, not a remaining-game/audio memory proof.

## Reproduce privately

Use the authorized private cartridge and A1200 Kickstart inputs already required
by the title target, with the official existing tools:

```sh
export AMIGA_BIN=/path/to/vscode-amiga-debug/bin/linux
bash native-renderer/amiga/round_build.sh
export PUAE_CORE=/path/to/puae_libretro.so
export LIBRETRO_INCLUDE=/path/to/libretro-common/include
export KICKSTART=/path/to/authorized-A1200-3.1.rom
for scenario in course0 course1 course2 random cancel skip0 skip1 skip2 skipboth \
                recordskip0 recordskip1 recordskip2 recordheld; do
  bash native-renderer/amiga/round_run.sh "$scenario"
done
bash native-renderer/amiga/round_test.sh
python3 -m unittest discover -s tests -p test_round_frontend.py -v
```

`tools/export-course-intro-assets.py --round` adds bounded normal record resources
and emits 96 private spans under ignored `build/`. Without that option the legacy
43-span title export remains unchanged. All generated target and verification
output stays under ignored `build/native-round`.
