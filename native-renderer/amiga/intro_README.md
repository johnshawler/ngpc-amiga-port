# Native Amiga course-introduction scenery target

This separate target continues the actual interactive course selector into the
first course-scenery screen for USA, Germany and Japan. It is a bounded, silent
native conversion, not playable golf. The original selector-only target and
its fifteen-entry exact planar cache remain separate.

**Target:** PAL A1200, 68EC020, AGA, 2 MiB Chip RAM, zero Fast RAM. Like the
selector target, this executable takes over display, interrupts and CIA B until
emulator reset. It has no OS-return path and must not be launched over unsaved
work. Physical hardware has not been tested.

## Behavior and boundaries

Joystick port 2 uses the unchanged controls: left/right navigation, first fire
for ordinary acceptance, second fire for cancel, and up plus first fire for the
original modified/random path. Initial selection is USA. The test host supplies
only external RetroPad input through PUAE's hardware-input callbacks; it does
not script native phase changes or write task state into emulated memory.

The optional outer owner is linked into the real task ring before the selector
parent. Parent exit writes the result and does not execute the owner again that
tick. On the next logical update the owner increments the normal golfer record
once, allocates the intro child, and that child runs later in the same traversal.
The order remains input, palette head, existing fills/stamps, object head, tasks,
palette tail and object tail. The empty object list after reset clears the old
selector cursor through the ordinary deferred object path.

The intro directly clears map 0, resets owned display state, queues the mapped
course's two palette jobs and map-1 background, then opens the viewport at the
original signed-delay boundary. It halts at the next unsupported title callback.
The following title object/animation, tournament title, round-record screen and
tee-shot routines are **not implemented**. Cancel stops at the prior-selection
request rather than inventing that earlier screen. A special-record request is
explicitly unsupported. The standalone target starts with normal golfer 0 in
bank 0; it does not restore save progression.

See [core contracts and original comparison](../../native-game/COURSE_INTRO.md)
for isolated and integrated state equivalence. No TLCS-900 interpreter runs in
the Amiga executable.

## Rendering and timing

The target shares the tested selector display frontend, snapshots, hardware
joystick sampling and CIA cadence: 241 updates per eight nominal seconds,
bounded to two due updates per poll with any loss counted explicitly. It does
not alter the catch-up limit to conceal stalls. Full selector snapshots still
use exact cache hits. Course scenery uses the general renderer, with exact-key
cache misses asserted; no selector candidate is accepted for a new background.

A private-source copy backend is enabled only by `NTM_SCREEN_PRIVATE_ASSETS`.
It requires generated read-only source spans to be disjoint from tile/map
arrays. Each already-validated 16-byte tile uses the tested bounded 68020 copy;
validation, map writes, queue order and the default generic path are unchanged.
Both branches pass host, ASan/UBSan, QEMU m68020, and original routine/pipeline
comparisons. The same compile flag is used by the private oracle's fast-copy
pass. The ordinary selector build does not enable it.

This optimization was required by measurement: the generic `-Os` tile loop
recomputed source offsets for every byte. A large course background produced
one dropped update and a 105.02 ms maximum input-poll gap. Compiling only that
consumer at `-O2` still dropped one update at 102.46 ms. The opt-in copy path
passes every tested flow with zero drops. Those rejected timing runs remain in
ignored local build directories, not source.

Coherent output is **not full-speed presentation**. All four accepted runs
already reached the unsupported title boundary before the first scenery bitmap
was presented. The logical viewport state itself is reached at the converted
update; rendering deliberately presents an immutable earlier snapshot. No
course-title animation is shown or implied. Short presses between polls can
still be missed.

Measured with the verified PAL PUAE profile:

| Flow | Accept to first verified scenery | First scenery composition | Maximum input-poll gap |
| --- | ---: | ---: | ---: |
| USA, mapped course 0 | 3.60 s | 2.303 s | 44.32 ms |
| Germany, mapped course 1 | 3.56 s | 2.283 s | 48.21 ms |
| Japan, mapped course 2 | 3.52 s | 2.263 s | 40.76 ms |
| Modified/random acceptance | 5.62 s | 2.303 s | 42.12 ms |
| Cancel | No scenery requested | Not applicable | 31.02 ms |

The accept-to-capture interval includes the original accept/random delay,
rendering and a two-stable-frame capture guard. Seconds for that interval use
nominal PAL 50 Hz; it is not first illuminated-pixel latency. Composition uses
the emulated CIA clock. Later unchanged scenery frames use dirty-row reuse at
about 0.381 seconds; that number is **not** initial scenery creation time.
No physical-hardware or whole-game speed claim follows from these measurements.

## Exact target checks and memory

Five hardware-input scenarios pass, with **46 captured snapshots** including
mixed and repeated navigation, all three course outputs, first scenery and final
boundary states. Every capture checks all 81,920 displayed bitplane bytes against
an independent per-pixel/depth oracle, all 256 Copper colors in both AGA nibble
banks, and every captured RGB pixel including borders. All 512 palette bytes
are independently resolved from original cartridge jobs. Scenery tiles and map
writes are independently decoded from the cartridge descriptors. Both sprite
VRAM and sprite palettes are checked empty after reset. All five flows report
zero logical update drops.

The final executable is **637,280 bytes**, with **635,883 ELF text bytes** and
**70,264 BSS bytes**. The explicit display allocation remains 170,000 Chip bytes.
Exec reports 1,279,456 free Chip bytes before that allocation and **1,109,456**
after it. The 555,120-byte selector cache remains resident even after acceptance.
This is headroom for the bounded slice, not proof that audio or the rest of the
game fits. Overlay/reclaimable storage is not implemented.

After these shared-code changes, the separate selector-only target was rebuilt
and all three accept/cancel/random hardware flows passed their unchanged
pixel/palette/drop gates. Frozen assets and runs from selector baseline 396d724
remain under the separate ignored baseline directory.

## Reproduce privately

Provide the authorized cartridge at `private-inputs/neo-turf-masters.ngc` and
use the existing m68020 toolchain. Then:

```sh
export AMIGA_BIN=/path/to/vscode-amiga-debug/bin/linux
bash native-renderer/amiga/intro_build.sh
export PUAE_CORE=/path/to/puae_libretro.so
export LIBRETRO_INCLUDE=/path/to/libretro-common/include
export KICKSTART=/path/to/authorized-A1200-3.1.rom
for scenario in course0 course1 course2 random cancel; do
  bash native-renderer/amiga/intro_run.sh "$scenario"
done
bash native-renderer/amiga/intro_test.sh
python3 -m unittest discover -s tests -p test_intro_frontend.py -v
```

The exporter resolves three bounded descriptor/background groups and six
palette jobs, refusing output outside ignored `build/`. Assets, executable,
ADF, report manifests, screenshots, RAM captures and firmware stay private in
`build/native-intro`. No commercial assets or private-derived hashes are in
tracked source. Public tests use only synthesized resources and require no
cartridge or firmware.
