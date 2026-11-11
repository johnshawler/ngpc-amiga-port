# Native hole overview on stock PAL A1200

This separate bounded target continues the existing selector, course scenery,
course title, tournament title and fresh round-record children through the real
normal tee owner and its first child, the **hole overview**. It shows the hole
number, distance, par, green inset, perspective course and original animated
objects. The owner stops at the actual unconverted `0x2096a4` shot-scene request.
**There is no playable shot, swing, ball flight, hole simulation or audio.**

The target starts normal single-player golfer 0 with fresh saved scores 10,
handicaps zero, reset kind 0, English language 1, base difficulty 2 and previous
selected difficulty 0. It carries the preceding round's actual course order and
shared RNG into hole preparation. It does not reset the RNG to choose a desired
hole. All 54 geometry entries and all 162 overview views remain available.
Saved-game restoration, linked/alternate modes and match-play screens are not
implemented by this hardware frontend.

## Separate disk layout

`tee_build.sh` builds a freestanding native m68020 executable and ordinary
880 KiB DD ADF. The selector/title/round targets and their cache formats remain
unchanged. No original CPU core or cartridge interpreter is linked into this
target. The exporter borrows only bounded resource spans from an authorized
local cartridge, and no cartridge, firmware, generated assets or captures are
tracked.

The naive tee resource spans total 3,602,325 bytes because many stamp headers
address overlapping tile pools. Sharing overlapping source ranges reduces them
to **169,016 bytes** in 21 ranges, with all 54 holes, eight animations and hole
preparation tables retained. The 54 geometry structures are additional. The two
handicap art fields are unused by the supported normal-mode child and remain
zero; unsupported modes are not silently substituted.

Simply adding this data to the previous round executable would exceed DD disk
capacity. This target instead losslessly PackBits-compresses all fifteen existing
selector pictures. It retains every complete original comparison key and all
364,800 decoded picture bytes. The resident cache is **347,492 bytes**:
190,440 bytes of exact keys/pointers/lengths and 157,052 compressed picture bytes,
compared with 555,120 bytes in the legacy uncompressed cache.

Each decode validates the complete stream before writing, rejects invalid,
truncated, overlong and no-op streams, and produces exactly 24,320 bytes in the
already existing compact bitmap buffer. Both passes poll input/application work
after at most 639 decoded bytes. A mismatch still invokes the exact general
renderer. Compression is a storage tradeoff, **not a speed improvement**.

The final executable is **792,312 bytes**, with **783,761 ELF text bytes** and
**80,412 BSS bytes**. Its DD ADF is 94.0% full. Explicit display allocation remains
170,000 Chip bytes including the 512-byte report block. The stock A1200 run
reports 1,121,432 free Chip bytes before that allocation and **951,432 afterward**.
This headroom is for the bounded slice, not a remaining-game/audio memory proof.

## Lifecycle and honest presentation

The converted owner allocates the overview, which allocates the normal notice
and handicap children in their original scheduler order. Original palette jobs,
direct font/header writes, deferred hole/par/course stamps and three independent
object clocks remain live. The normal notice and handicap children really finish;
they are not bypassed by frontend state patches. Audio calls are metadata-only
requests, never a claim of synthesis.

Original waits are unchanged. No task is paused or extended to help rendering.
CIA pacing is still 241 logical updates per eight nominal seconds with the
existing two-update catch-up bound. The host supplies RetroPad input through
emulated Amiga joystick registers only; it reads diagnostics and captures
memory/video without writing application state.

Immutable snapshots avoid tearing but may arrive late or miss short logical
phases. The report distinguishes snapshot phase, current phase and the object
state that prepared the preceding drawing-head upload. A logically visited
scene is never counted as displayed without a verified capture. The first
course overview composition remains approximately 2.8 seconds, not a whole-game
frame-rate result. Cache decoding adds substantial selector latency; see each
run's measured decode and input-to-presentation timing rather than the older
round target's timing figures.

Early skip scenarios press during logical HOLD, whether or not the overview has
arrived. Separate `teeseen0/1/2` scenarios wait until a stable overview is actually
displayed while current logic is still HOLD before pressing. These are different
input tests and their presentation delays must not be conflated.

## Measured final hardware matrix

All **20 flows and 521 captures** pass their gates with **zero dropped logical
updates**: three ordinary courses, modified/random, cancel, title skips, double
title/tournament skip, record skips/held input, early overview skips/held input,
and all three after-seeing overview skips.

| Ordinary flow | Accept to first verified overview | First overview composition | Snapshot age at capture |
| --- | ---: | ---: | ---: |
| USA | 17.44 s | 2.805 s | 86 logical updates |
| Germany | 17.20 s | 2.824 s | 87 logical updates |
| Japan | 17.16 s | 2.785 s | 85 logical updates |
| Modified/random | 19.04 s | 2.783 s | 85 logical updates |

These four pictures arrived while logical HOLD was still active. The
`teeseen0/1/2` host then pressed using only hardware input after seeing the stable
picture, reducing hold-to-cleanup from 181 updates to 100, 92 and 91 respectively.
The early logical skips reduced it to 10 updates. They are not equivalent:

- All three ordinary flows displayed scenery, tournament, record and overview,
  but **missed the separate course-title picture**.
- Modified/random displayed title, tournament, record and overview, but
  **missed the separate scenery picture**.
- Record-skip and held-record-skip flows **missed the record picture**.
- Early USA overview skip and held overview skip **missed the overview picture**.
- Early Germany/Japan overview skips presented a captured HOLD picture only
  **after logical cleanup**. They do not prove a subsequent press would act on
  the stage shown in that delayed picture.

The maximum observed input-poll gap was **56.954 ms**; a short press between polls
can be missed. Complete selector navigation pictures arrived **1.40–1.48 s**
after input, compared with **0.56–0.62 s** in the preserved round baseline.
Maximum cache decode time was **477.863 ms**. The compressed cache fits the new
resources on DD without dropping keys, but measurably worsens selector
responsiveness. There is no claim that this is an overall faster renderer.

Authoritative final run reports are under ignored `build/native-tee/final/`.
Exploratory runs are not counted in these final totals. The retained legacy
selector/title/round targets do not adopt this cache or its timing regression.

## Verification

The independent verifier resolves stamps, colors, geometry, marker coordinates,
shared-RNG pin choice and object animation/shape/tile pointers directly from the
cartridge, independently of the exporter and native resource bindings. It checks
all inherited and newly written known tile/map bytes, all hardware sprites and
sprite palettes, all 81,920 displayed bitplane bytes, all 256 Copper colors in
both AGA nibble banks and every captured RGB pixel including borders. It checks
source par/distance, normal object clocks, deferred head state, scene controls,
exact next-child boundary and zero dropped logical updates.

The cache tests compare all fifteen complete keys and all **364,800** decoded
picture bytes against the original generated cache on host, ASan/UBSan and QEMU
m68020. Synthetic tests cover interval sharing, all 54 hole/162 view bindings,
malformed resource headers, font-only resources, color tile words, inclusive
sprite clipping, animation stops, run/literal boundaries, atomic decoder
rejection and hardware-only input. Native core/original comparisons are described
separately in `native-game/TEE_SETUP.md`, `TEE_OBJECTS.md` and `HOLE_PREPARE.md`.

## Retained target regression

The shared inclusive cursor-edge correction was also tested by rebuilding the
legacy targets from this source: selector 3 flows/24 captures, title/intro
9 flows/161 captures, and round-record 13 flows/294 captures all passed. These
separate executables keep their original uncompressed selector caches. The prior
round checkpoint and its measured results remain preserved rather than overwritten
as evidence for the new tee executable.

## Reproduce privately

Use the authorized local cartridge and A1200 Kickstart, and the existing official
toolchain. All generated assets and reports remain in ignored `build/`.

```sh
export AMIGA_BIN=/path/to/vscode-amiga-debug/bin/linux
bash native-renderer/amiga/tee_build.sh
bash native-renderer/amiga/tee_test.sh
python3 -m unittest discover -s tests -p test_tee_frontend.py -v
export PUAE_CORE=/path/to/puae_libretro.so
export LIBRETRO_INCLUDE=/path/to/libretro-common/include
export KICKSTART=/path/to/authorized-A1200-3.1.rom
export TEE_RUN_ROOT="$PWD/build/native-tee/final"
for scenario in course0 course1 course2 random cancel skip0 skip1 skip2 skipboth \
                recordskip0 recordskip1 recordskip2 recordheld \
                teeskip0 teeskip1 teeskip2 teeheld teeseen0 teeseen1 teeseen2; do
  bash native-renderer/amiga/tee_run.sh "$scenario"
done
```

The `TEE_RUN_ROOT` override separates final captures from exploratory captures;
verification reads only filenames listed by the current run. It never silently
uses stale images. Timing uses emulated CIA ticks and nominal PAL 50 Hz with a
two-stable-frame capture guard, not first illuminated-pixel latency or physical
hardware measurement. The program takes over the machine until emulator reset.
