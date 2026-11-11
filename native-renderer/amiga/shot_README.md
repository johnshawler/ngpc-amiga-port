# Native first-tee banner target

This separate target continues the genuine selector, course title, tournament,
round record and hole overview into first-shot preparation and the first-tee
banner. It stops at the original **0x209751** boundary before unconverted aiming
scene setup. It is not a playable shot or a complete golf game. Sound requests
remain metadata; this target does not synthesize audio. The harness starts
normal unlinked golfer 0 with fresh scores 10, zero handicaps, language 1 and the
existing shared RNG. It does not restore saves; the optional Option/save menu is
an explicit unsupported boundary, not a working feature.

The required machine remains PAL A1200, 68EC020, AGA, **2 MiB Chip and zero Fast
RAM**. The package is a standard **901,120-byte DD floppy**, using Kickstart
3.1-supported FFS instead of OFS. It is not an HD floppy or hard-disk package.
Previous selector, intro, round and tee targets are unchanged. The complete tee
DD baseline remains separately preserved at source checkpoint `299f589`.

## Exact resources and packaging

The private exporter resolves all 54 holes, all 162 overview views, all eight
prior overview animations and banner velocities 60/148/152/156 from the supplied
cartridge. It includes all six character labels, both player labels and both
language hints, the actual first-tee terrain stamp, portrait and animated
palette commands. No hole/RNG replacement, generated substitute art, artificial
task completion, enlarged wait or dropped asset is used.

The 489 bounded source spans total 3,709,331 bytes before sharing. Merging only
identical overlapping source addresses gives a 231,082-byte pool in 36 ranges;
resolved source/target fade colors add 256 bytes. This is **62,322 extra
resource bytes** over the prior 169,016-byte overview pool. The real first shot
is exactly at the prepared tee center and returns terrain 4 before reading a map.
Uninitialized original terrain-map storage is therefore left absent, not
replaced by a made-up map. The terrain masks/shapes and the full first-shot
geometry/wind inputs remain.

The target executable is **866,232 bytes**. ELF text is 856,555 bytes and BSS
87,208 bytes. The FFS disk uses 1,723 of 1,760 sectors, leaving **18,944
bytes**. Both a separate stdlib-only raw-block verifier and `amitools`
extraction recover the entire executable byte for byte. The independent verifier
also recovers the preserved 792,312-byte OFS tee baseline exactly.

An OFS attempt is explicitly unusable: `exe2adf` printed success and 99.9% fill
for an overfull executable, but the generated filesystem had corrupt metadata
and incomplete file data. Success text is not a packaging test. Every current
build requires independent recovered-file equality before it can pass.

The existing lossless selector cache still retains all fifteen exact keys,
347,492 resident bytes. No new per-frame compression was added for the banner.
Before any full-picture cache hit, every one of the 152 row-scroll values must
match the scalar scroll key. A synthetic test changes just row 72 and rejects the
hit. Cache misses use the general per-row renderer.

## Raster lifecycle and presentation

The target owns the original raster source/control state, starting with live
normal-game TRUN 136, GPU control 128 and interrupt shadow 0. The banner installs
and removes the actual converted callback service; it does not replace it with a
no-op or a delay-only completion. Direct scroll writes remain separate from
upper/lower raster source bytes.

Each immutable display snapshot includes all 152 row-scroll values. Only source
plane 1 gets that row's horizontal scroll. Sprites, plane 2 and all other controls
retain their own state. Both the direct planar and scalar fallback paths use the
same row-local scene. Dirty dependencies include the row plan and conservatively
invalidate affected rendering when source tiles/maps change. No Copper shift of
already-composited sprite/plane 2 pixels is used.

The underlying raster helper independently proves latch-before-draw and
next-counter IRQ order. A further live original trace starts from genuine
controller input, then observes unchanged interpreter/timer/Z80 functions. The
observer's entire final **31,743-byte serialized state** equals ordinary
`retro_run` replay after 382 frames. Across all three courses, with automatic
and hardware-input exits, registration, all nineteen source increments and
removal occur in VBlank before row 198. That final blanking interrupt primes
visible row 0: rows 0–72 use the new upper byte and rows 73–151 use the lower byte.

The target follows that demonstrated blanking-first order for two source frames
per logical update. Only repeated row spans whose state does not change are
coalesced. **262,144** synthetic states compare the resulting full row plan and
final raw/latch state against all 398 uncoalesced row steps. This includes both
source bytes and timer/GPU enable combinations, plus invalid-request and dirty
row tests. This is a bounded first-tee proof, not cycle-exact emulation of the
whole game's CPU or arbitrary raster effects.

## Verification

The core application has its own separate original comparison: **24 flows and
7,410 updates on each of two original tile-fetch backends**, including nested
tasks, geometry, shared RNG, palettes, objects, input and raster registration.
Those are not counts of Amiga hardware runs.

The Amiga test host supplies real emulated joystick/button input. It never
patches target RAM, RNG or phase state. Every target update records the sampled
input in bounded diagnostic storage. A separate original replay uses exactly
that trace and independently resolved cartridge resources to verify every
captured map, tile, sprite, sprite-palette, color-palette, selection, banner
phase and row-scroll plan. Pixel verification then checks all 81,920 displayed
bitplane bytes, all 256 Copper colors in both nibble banks and every captured
RGB pixel, including borders. Snapshot and current logical phases are reported
separately. The source input trace is evidence, not an input playback mechanism
inside the Amiga target.

The final hardware matrix passes **16 flows / 459 captures**, with **16,698
updates replayed against the original** and **zero dropped logical updates**. It
covers all three courses, random/cancel, early and held input, input after an
actually displayed banner, held input carried from the overview, and three
partial-fade raster capture cases. These counts are distinct from the core's
24-flow oracle. The independent exported-binding check additionally compares 459
bounded spans against original resource resolvers, including all 54 holes and
162 views. Host, ASan/UBSan and QEMU m68020 frontend tests all pass.

Final hardware timing totals and per-phase omissions are recorded privately in
`build/native-shot/final/summary.json`. No private cartridge, firmware,
generated art, capture or private hash belongs in source control.

## Known presentation limits

Rendering remains much slower than the source update rate. The logical clock
continues at 30.125 updates per second while coherent snapshots are composed;
short transitions may be omitted or presented after logical cleanup. The first
final USA run misses the short exit picture. Germany and random show a
partial-fade exit after logical cleanup; Japan's captured exit state is already
black. All three dedicated partial-fade cases show upper 60/lower 0, but each is
**17 logical updates late** after cleanup. Tests vary genuine button timing to
exercise more of this transient; they do not extend its original duration.
Nonzero row-scroll metadata alone is not evidence of a visible exit: a late fade
can already be black. Earlier black-only raster explorations are excluded from
the final dedicated partial-fade cases. Even a visibly correct exit snapshot is
not proof that every animation frame appeared. These are not full-speed or
physical-hardware claims, and navigation/individual picture timings are not
whole-game FPS.

The observed target has 871,840 Chip bytes free after its 170,000-byte display
allocation. This includes the resident selector cache and new static resources.
It is a slice-specific budget, not a proof that the complete game will fit. The
maximum measured polling gap is **57.267 ms**, with no dropped logical updates.
Verified selector navigation takes **1.50–1.60 seconds**. Ordinary course0/1/2
first-banner pictures take 3.084/3.064/3.023 seconds to compose and arrive
95/94/92 logical updates after their snapshot, while HOLD is still active. The
longest measured single composition across the matrix is **3.285 seconds** (the
held-overview-input case); course0's maximum is **3.125 seconds**. These are
individual snapshot composition times, not a whole-game frame rate. Per-run
composition age, omitted stages and input latency remain in the final summary.

A clean rebuild from the final reviewed sources produces a byte-identical
executable and ELF. Moving the per-row scene into the scalar fallback's scope
also leaves the tested m68020 object byte-identical; that fallback compiles
separately. Both tests and the rebuilt-source fingerprint are preserved with the
final private artifacts.

## Reproduction

Use the existing m68020 toolchain, official PUAE core and authorized private
A1200 Kickstart3.1/cartridge inputs. Install the normal PyPI `amitools==0.9.0`
package into an ignored build directory, for example:

```
python3 -m pip install --target build/shot-pack-tools amitools==0.9.0
source ../amiga-recovery-m68k/activate.sh
AMIGA_BIN=../amiga-recovery-tools/vscode-amiga-debug/bin/linux \
  bash native-renderer/amiga/shot_build.sh
bash native-renderer/amiga/shot_test.sh
python3 reference/check_shot_target_assets.py
```

`shot_run.sh` additionally requires `PUAE_CORE`, `LIBRETRO_INCLUDE` and
`KICKSTART`. It builds a host, runs hardware input, replays the exact trace
against the original, then verifies all rendered output. It accepts the prior
course, random/cancel and skip cases plus `shotseen0..2`, `shotskip0..2`,
`shotheld` and `raster0..2`. The latter presses after a longer genuinely shown
banner interval to exercise exit snapshots; game scheduling is unchanged.

`reference/trace_shot_raster_live.py --course N [--automatic]` reproduces the
live phase proof without rebuilding or patching the original core. Public tests
need no cartridge and cover exporter bounds/sharing, row coalescing, cache
rejection, input-only host behavior and independently verified OFS/FFS file
recovery.
