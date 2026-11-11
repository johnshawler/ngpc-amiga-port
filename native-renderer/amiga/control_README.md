# Native aiming controls on a stock A1200

This separate target continues the real course selector, titles, round record,
hole overview and first-tee banner into the converted aiming ready gate. It
runs original-equivalent held/released aiming, club selection and timed highlight
restoration, active camera panning/restoration, and the main-loop `201608` world
scroll service at its genuine pre-task location. The old aim frontend is not
changed, and its preserved `build/native-aim-baseline-d46936c` remains separate.

The target is **not playable full golf, full-speed gameplay, or a physical
hardware result**. Fire and information-overlay/reset dispatch are explicit
partial-update boundaries. Swing, shot physics and game completion are not
fabricated. Audio requests are metadata, not native sound synthesis.

## Bounded machine and resources

The target uses PAL A1200, 68EC020/AGA, 2 MiB Chip and **zero Fast RAM**. Its
901,120-byte DD FFS disk is checked by two independent file-recovery verifiers.
The executable is 877,900 bytes, leaving 7,168 disk bytes free. Boot reports
69,712 Chip bytes free after the 170,000-byte display allocation. The ELF has
849,263 text bytes and 896,112 BSS bytes. A separate clean build produces
byte-identical ELF and executable files. These margins apply only to this slice.

All 54 holes, worlds, panoramas and object sets remain available. The exact
resource pool adds all 23 club highlights and contains 1,740 bounded source
spans. It is 596,238 bytes uncompressed and 306,527 bytes packed. The unchanged
96 intro arrays occupy 177,974 bytes after alignment, packed to 56,229 bytes.
Both pools unpack once, before logical updates start, using the existing bounded
originally authored decoder. Packed and unpacked storage remain resident.

The selector assets, all 15 selector cache keys and pictures, intro arrays and
packed intro stream are byte-identical to the preserved first-opening target.
Independent cartridge resolvers pass 3,816 exact span checks, plus all 96 intro
arrays and their C `sizeof` semantics. No art, course, hole, cache or resource
identity is removed to fit the disk or RAM budget.

## Hardware controls and independent verification

`control_host.c` drives only libretro joystick inputs. It never writes game RAM.
The target records its actual hardware-sampled input byte for every logical
update. The normal test sequence includes held left/right and release, six
separate up/down club presses with releases, the original highlight timeout,
button-32 camera entry, cardinal/diagonal movement, neutral camera hold,
release/restore, and idle. Separate idle and cancellation cases are included.

The host stops after a bounded run, while the normal application is still
running. It waits for a coherent picture of the genuinely restored camera and
expired highlight before its final capture. It does not install a game-complete
flag, lengthen a game timer, manufacture an input edge, or finish a pending task.
The target retains its bounded 4,096-update trace capacity and reports an error
rather than silently truncating it.

`compare_control_target.py` independently replays only that recorded hardware
trace through the unchanged original CPU routines and independently resolved
resources. Every update checks task links/callbacks, input, RNG, player/hole/aim,
club/camera state, scroll, queues and graphics source state. Captures additionally
check immutable control metadata and exact original tiles, maps, sprites,
palettes, source scroll and row plans. The original `201608` service is included;
the earlier first-opening picture alone is not used as a later-scene oracle.

Every capture then checks all 81,920 display bitplane bytes, all 256 Copper
colors in both nibble banks, and every captured RGB pixel including borders.
For every post-opening capture, the original's video state is independently
saved at the corresponding replay update. Only after all CPU replay has ended,
those original states are restored to the original GPU and its real latches are
settled without running CPU instructions. All 24,320 source pixels are checked
against both horizontal Amiga copies. No Amiga source bytes are copied into the
original renderer, and renderer checks do not substitute for source equality.

An exploratory first course0 run is retained in `build/native-control/initial`.
Its state and pixels passed, but its final capture still displayed an older
camera-pan snapshot after the live camera had restored. The final host wait was
corrected to capture the restored picture; no runtime or game timing changed.
The original initial host source and binary are preserved with that evidence.

## Timing limitations

The shared clock retains the exact 241/8-Hz timebase and fractional phase, with
at most eight updates per poll and explicit dropped-update accounting. Catch-up
executes elapsed logical updates; it cannot recreate transitions missed between
hardware polls or make rendering fast. Coherent display composition can take
several seconds and omit intermediate pictures despite zero dropped updates.

The final matrix passes **6 flows / 194 captures / 10,263 original updates**,
with zero dropped logical updates. All **40 post-opening captures** also pass
the direct unchanged-original GPU comparison. The maximum polling gap is
108.221 ms, maximum due/executed catch-up batch is three, and maximum single
composition is 4.907091 seconds. The largest captured snapshot age is 149
logical updates. Startup unpacking takes 1.763676 seconds before the clock
starts. These are measured limits, not full-speed or responsive gameplay.

The generated `final/summary.json` contains each flow's timing, multi-update
polls, free Chip bytes and captured snapshot ages. Snapshot ages are sampled at
captures, not a claim to measure every intervening display age.

## Reproduction

Use the existing official/reputable toolchain, PUAE and authorized private
cartridge/Kickstart inputs. No generated assets, cartridge bytes, captures or
private fingerprints belong in source control.

```sh
source ../amiga-recovery-m68k/activate.sh
AMIGA_BIN=../amiga-recovery-tools/vscode-amiga-debug/bin/linux \
  bash native-renderer/amiga/control_build.sh
bash native-renderer/amiga/control_test.sh
python3 reference/check_aim_target_assets.py --directory build/native-control
CONTROL_RUN_ROOT="$PWD/build/native-control/final" \
PUAE_CORE="$PWD/../amiga-recovery-tools/libretro-uae/puae_libretro.so" \
LIBRETRO_INCLUDE="$PWD/../amiga-recovery-tools/libretro-uae/libretro-common/include" \
KICKSTART="$PWD/private-inputs/Kickstart-3.1-A1200-40.68.rom" \
  bash native-renderer/amiga/control_run.sh course0
python3 native-renderer/amiga/control_summary.py build/native-control/final
```

Use fresh run directories for `course0`, `course1`, `course2`, `random`, `idle`
and `cancel`. The runner rejects directories containing earlier captures.
`CONTROL_BUILD_ROOT` selects a separate build for clean-rebuild comparisons.
Runtime source manifests, binary identities, failed/exploratory controls and all
private verification artifacts stay under ignored `build/`.

The shared clock and decoder pass host, ASan/UBSan and m68020/QEMU tests,
including 100,000 independent clock-model comparisons, malformed-stream atomic
rejection, 20,000 deterministic malformed streams, Python differential cases,
and exact decoding of both current private pools. The control target also
compiles with the scalar renderer fallback. Verified hardware runs use the
direct planar renderer. Core application tests are documented separately.
