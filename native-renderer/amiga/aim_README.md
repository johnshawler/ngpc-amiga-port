# Native aiming-scene opening target

This is a separate stock PAL A1200 target. It retains the genuine course
selector, titles, round record, hole overview and first-tee banner, then opts
into native aiming-world setup and the first viewport opening. The preserved
shot target and its full `build/native-shot-baseline-e489487` remain unchanged.

The scope stops after the genuine 2098af viewport write, with next 2098ee and its
original delay 3 installed. The scene owns the actual ball/pin/power objects,
marker/club children, delayed hint/camera children, object queues and viewport.
The first-open image still contains conspicuous repeated bright strips in the
world/panorama area. The unchanged original GPU produces the same strips when
rendering the independently replayed original state at this exact boundary.
A direct comparison checks all 24,320 original pixels against both horizontal
copies in the Amiga capture, including this region. The reference renderer
settles its register latches without advancing the original CPU. This verifies
the bounded opening image, not a later camera-ready scene or whole-game video.
Further camera, input and swing callbacks are outside this target.

It is **not a playable shot, full-speed game or physical-hardware result**.
Audio requests remain metadata. Inputs, game waits and resource identities are
not substituted to make a screen appear.

## Stock machine and bounded storage

The required machine remains PAL A1200, 68EC020/AGA, 2 MiB Chip and **zero Fast RAM**.
The package is a standard 901,120-byte DD FFS floppy supported by Kickstart 3.1.
Every build must pass both the existing independent raw-block verifier and the
separate package recovery check. `exe2adf` success text is not accepted as proof.
No expanded-memory result is used as a stock result.

The exporter includes all 54 aim worlds, all 54 panoramas and course/hole objects,
all preceding overview/banner resources, ten opening object selections, all 12
club power tables, 360 normal/overflow lie spans, 23 labels, 11 digit stamps, 19 lie
icons and the exact marker lookup table. 1694 bounded source spans share only
identical overlapping cartridge addresses. Embedded indices, signed-window
origins and source identity tags remain unchanged.

The combined exact resource pool is 591,902 bytes. A new original bounded LZ
stream stores it in 303,906 bytes. The 96 existing intro/title/round arrays share
only identical contents and are 177,974 bytes after alignment, stored in 56,229
bytes. All 15 existing selector cache keys and their 347,492-byte resident cache
remain intact. No art, hole or cache is dropped to obtain disk fit.

`aim_unpack.c` is newly authored code under the repository's GPL-2.0-or-later
license, not copied from an external decompressor. Opcodes 0..127 are 1..128-byte
literals; 128..255 are 3..130-byte matches followed by a big-endian 16-bit backward
distance. A full validation pass checks exact input/output exhaustion,
truncation, output capacity and nonzero in-range match distances before writing
anything. Source/destination overlap is rejected. The decode uses caller-owned
buffers, no allocator/dictionary, and supports repeated overlapping matches.

Resources are unpacked **once before logical updates begin**. The uncompressed
buffers are borrowed for the whole application lifetime. Both compressed and
uncompressed storage remain resident; this is an explicit Chip-RAM cost, not
freed memory or per-frame resource recompression. The prior selector picture
cache retains its unchanged per-snapshot unpacking path.

The reviewed executable is **869,912 bytes**; its DD FFS disk has **14,848 bytes
free**. Stock boot reports **81,864 Chip bytes free** after its 170,000-byte
display allocation. These are tight slice-specific budgets, not proof that
later scenes or the full game fit. The ELF has 842,037 text bytes and 891,184 BSS
bytes. A clean rebuild produces a byte-identical executable and ELF. The
independent disk verifier recovers the whole executable, not a truncated prefix.

## Timing and honest boundaries

An initial opening control run reached the viewport but exposed one dropped
logical update: the inherited selector clock allowed only two updates after a
poll. Its maximum observed polling gap was 105.34 ms. The complete evidence stays
in the private `build/native-aim/opening-initial` directory and is not included
as a passing final hardware run.

The separate aim frontend now retains the same exact 241/8-Hz logical timebase
and fractional phase while permitting a bounded maximum of eight updates for
actual elapsed time. Updates above that bound are counted explicitly as dropped.
No original game delay is extended. It does not recreate button transitions
missed between hardware polls, improve rendering throughput or demonstrate
responsive/full-speed play. The diagnostic report separates maximum polling
gap, due/executed batch size, multi-update polls, dropped updates and individual
snapshot composition time. The final matrix measured a 111.740 ms maximum poll
gap, a maximum actual catch-up batch of three updates and zero dropped logical
updates. Its longest single composition was 3.224495 seconds; maximum observed
snapshot age was 99 logical updates. Startup resource unpacking cost 1.753276
seconds before the logical clock started. Long coherent compositions can omit
intermediate pictures even while every logical update is executed.

## Verification and reproduction

The exact original-input comparison consumes only the target's recorded hardware
input trace. Neither the host nor target patches phase, RNG or memory to progress.
The original interpreter and independent cartridge resolvers verify source maps,
tiles, sprites, palettes, selection, task state and aim/opening state. Pixel verification
then compares all 81,920 displayed bitplane bytes, 256 Copper colors in both nibble
banks, and every captured RGB pixel including borders. Source equality is separate
from the renderer oracle; they cannot validate each other by circular comparison.

The final hardware matrix passes **8 flows / 215 captures**, with **8035 updates
replayed against the original** and zero dropped updates. Cases are all three
courses, random, cancel, early banner input, held banner input and held input
carried from the overview. These are separate from the core application's
24-flow / 7473-update comparison on each of two tile-fetch backends. All seven
non-cancel final opening images also pass the direct original-GPU pixel check.
The input polling gaps and burstiness above remain real limitations despite
correct state and picture comparisons.

The resource checker compares 3770 bounded spans/arrays against independent
original resolvers, plus all 96 original intro arrays and their C `sizeof`
semantics. The startup decoder is tested on host, ASan/UBSan and QEMU m68020,
including full private-stream equality, malformed/truncated streams and unchanged
output on failure. 20,000 deterministic random malformed streams and independent
Python differential cases are included. The clock gets 100,000 comparisons against
an independent 64-bit timing model, including fractional phase and cap overflow.
The scalar renderer fallback also compiles separately for m68020; the verified hardware runs use the direct
planar renderer.

Use the existing official/reputable m68020 toolchain, PUAE and authorized private
cartridge/Kickstart inputs. Keep all generated includes, ROMs, art, captures and
private hashes out of source control.

```
source ../amiga-recovery-m68k/activate.sh
AMIGA_BIN=../amiga-recovery-tools/vscode-amiga-debug/bin/linux \
  bash native-renderer/amiga/aim_build.sh
bash native-renderer/amiga/aim_test.sh
python3 reference/check_aim_target_assets.py
AIM_RUN_ROOT="$PWD/build/native-aim/final" \
PUAE_CORE="$PWD/../amiga-recovery-tools/libretro-uae/puae_libretro.so" \
LIBRETRO_INCLUDE="$PWD/../amiga-recovery-tools/libretro-uae/libretro-common/include" \
KICKSTART="$PWD/private-inputs/Kickstart-3.1-A1200-40.68.rom" \
  bash native-renderer/amiga/aim_run.sh course0
python3 native-renderer/amiga/aim_summary.py build/native-aim/final
```

The runner requires a fresh capture directory, preventing stale captures from a
failed earlier run from being mislabeled as new evidence. Course0/1/2, cancel,
random and the preceding banner/overview skip and held-input cases are supported.
The original application adapter is host-only and is rebuilt atomically before
replay. No original CPU emulator is part of the Amiga executable.
