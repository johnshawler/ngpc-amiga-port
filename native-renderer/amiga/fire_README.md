# Native first-fire frontend on a stock A1200

This is a separate target using `ntm_shot_fire_application_update`. It follows
real course selection, titles, round record, overview, aiming and the first-fire
transition. The genuine scheduler runs spin, trajectory, golfer initialization,
initial green/scenery drawing and subsequent animation. No children are skipped,
no input is written into game RAM and no completion is fabricated.

This is **not full golf, full-speed gameplay, native sound synthesis, or a
physical-hardware result**. Power selection, cancel transitions and ball flight
remain explicit later boundaries. The current host matrix stops while first-fire
is still running. This target adds new files without changing existing tracked source. The
preserved control executable, ELF and DD image remain byte-identical.

## Resource and package design

The first package is explicitly a **hard-disk directory**, not a DD image. Copy
`fire.exe`, `fire.pool` and `intro.pool` together and launch from that directory.
The generated `hd/S/startup-sequence` supports a dedicated boot volume. The
reproduction runner mounts that directory read-only in PUAE. It does not claim
a real IDE controller or physical disk performance result. Two-DD usability is
not implemented and no resources were removed to make a disk fit.

The new exporter extends the preserved control resolver without editing it.
Its 2,131 bounded source spans include all 54 overview/world/panorama/object sets,
all first-fire art and all 108 large/close terrain maps plus 54 feature lists.
Original source overlap, rather than summing duplicated per-hole spans, shares
storage. The combined fire pool is 794,340 bytes unpacked and 415,430 packed.
The unchanged 96 intro arrays occupy 177,974 bytes after alignment and pack to
56,229 bytes. Each file adds a 20-byte header. The original selector assets and
all 15 selector cache keys/pictures remain present.

Both pools are loaded before any hardware takeover. The DOS loader uses a
512-byte read buffer, not a full compressed-pool allocation. Header lengths and
FNV-1a checksum must match the spec compiled for this exact generated pool. The
first pass checks every token, back-reference distance, output bound, checksum
and exact EOF without changing output. The second pass decodes and rechecks the
entire stream. A second-pass failure scrubs its destination. Bindings are not
published before both files load. FNV-1a is accidental-corruption detection, not
a cryptographic signature or protection against malicious replacement.

The files are closed on all load outcomes. DOS library handles are released
before takeover or return. Missing/corrupt data, read/seek/close errors, library
failure and display-allocation failure stop before hardware changes. Actual
missing and corrupt resource boot tests return to AmigaDOS with error 20; both
screens were inspected. The ordinary successful target then takes over until
emulator reset. It does not implement a game-to-Workbench exit.

## Memory and verification

The executable contains uncompressed resource storage in BSS but no resident
compressed fire or intro payload. All target memory figures must be taken from
the final measured run, not inferred from the removed compressed bytes.
`build-sources.sha256`, per-run binary manifests and compiler `.su` files retain
the exact inputs and stack-frame measurements. A dedicated 16,384-byte BSS stack
is filled with a canary before use; captured touched-byte counts are observations
for these flows, not a universal maximum stack proof.

`fire_test.sh` runs the authored stream and bootstrap fixtures on host,
ASan/UBSan and static m68020/QEMU. The stream fixture includes positive short
reads, every truncation, individual bit corruptions, wrong headers/checksums,
invalid match distances, output overflow, first/second-pass read and seek
failures, changed second-pass data, trailing bytes and 20,000 deterministic
malformed streams differentially compared with the separately verified bounded
in-memory decoder. The bootstrap fixture injects library, open, read, rewind,
close and allocation failures and verifies cleanup before takeover. Actual
private fire and intro files decode byte-for-byte on all three backends.

`reference/check_fire_target_assets.py` passes 7,274 independent original-resolver
checks, including the unchanged 3,816 preceding/control checks and all fire
palettes, screen assets, maps, features and object families. The intro checks
also preserve each generated C array's real `sizeof`, not merely its bytes.

The frontend report has a BUSY/READY publication guard around both live updates
and displayed-snapshot pointers. Hardware captures ignore incomplete reports.
Each immutable snapshot also records its input-attempt index, separate from
completed updates, so later partial-boundary tests can be identified precisely.
The sampled input trace includes an attempted partial update rather than
silently discarding its button edge.

Independent target verification uses the recorded joystick samples as the only
input to the unchanged original CPU. It compares original task dispatch order,
task headers/links, scratch, RNG, sound requests and source graphics plus
immutable metadata. Only after replay ends are the corresponding original
video states restored to the unchanged original GPU. Its own rendered pixels
are compared against both horizontal Amiga copies. Separately, `fire_verify.py`
checks all 81,920 displayed bitplane bytes, every Copper color in both AGA nibble
banks, and every captured RGB pixel including borders. Original source equality
and original-GPU equality are separate checks, not replacements for each other.

Earlier exploratory builds remain under ignored build directories. The first
exposed a pointer-return ABI error in new assembly wrappers, corrected by moving
OS D0 pointer results into GCC's A0 return register. A later course1 capture
exposed partially published report fields; it was correctly rejected and led
to the BUSY guard. Neither exploratory timing set is final evidence.

## Measured guarded stock build

The final guarded build's executable is 545,744 bytes. Its three-file hard-disk
payload totals 1,017,443 bytes before filesystem/startup-script overhead:
415,450 bytes for `fire.pool` and 56,249 for `intro.pool`. ELF text is 509,606
bytes and BSS is 1,123,072 bytes, including the dedicated stack and resource
buffers. The native fire application structure is 22,482 bytes; the added
54 drawing/terrain sets, three object families and golfer-asset structures total
10,428 bytes. These structures are already included in BSS, not added again.

All three guarded runs report **171,840 Chip bytes free** after the separate
170,000-byte display allocation, with zero Fast memory. The stack's maximum
observed touched region is 3,308 of its reserved 16,384 bytes. The loader's
512-byte buffer is in its stack frame; GCC reports 608 bytes for that decoder
pass and 36 for its public wrapper, not another persistent heap allocation.
These figures apply to this bounded native frontend and the mounted-directory
emulator setup. They do not prove a later whole-game memory budget.

The final matrix passes **3 flows / 86 captures / 4,382 original updates** and
18,680 ordered original/native scheduler dispatches. Every capture passes
source/live metadata, bitplanes, palette and full RGB verification. All four
post-opening captures also match the unchanged original GPU, including the
final real fire drawing for all three courses. Four negative controls reject
a changed source tile, input-trace prefix, fire metadata word and displayed
pixel. A clean rebuild produces byte-identical ELF, executable and both pools.
The reference build helper also reproduces the independently tested oracle.

There are zero dropped logical updates and the largest catch-up batch is three.
Maximum polling gap is 111.312 ms and maximum single composition is 5.207775 s.
The oldest captured picture is 162 logical updates behind the live state.
These are measured limits, **not a full-speed or responsive-gameplay claim**.
Startup file-loading duration is not instrumented and is not reported as zero.
Actual Chip fit and correct resource release do not make this renderer fast.
Final evidence is in `build/native-fire/final2`; original replay/pixel reports
are in `build/fire-target-reference-check/final2`.

## Reproduction

Use the existing official toolchain, PUAE, authorized cartridge and Kickstart.
No generated assets, commercial bytes, screenshots or private fingerprints
belong in source control.

```sh
source ../amiga-recovery-m68k/activate.sh
AMIGA_BIN=../amiga-recovery-tools/vscode-amiga-debug/bin/linux \
  bash native-renderer/amiga/fire_build.sh
bash native-renderer/amiga/fire_test.sh
bash native-renderer/amiga/fire_reference_build.sh
FIRE_RUN_ROOT="$PWD/build/native-fire/final2" \
PUAE_CORE="$PWD/../amiga-recovery-tools/libretro-uae/puae_libretro.so" \
LIBRETRO_INCLUDE="$PWD/../amiga-recovery-tools/libretro-uae/libretro-common/include" \
KICKSTART="$PWD/private-inputs/Kickstart-3.1-A1200-40.68.rom" \
  bash native-renderer/amiga/fire_run.sh course0
```

Use a fresh run root for each matrix; repeat with `course1` and `course2`.
The runner refuses to overwrite a run. `FIRE_BUILD_ROOT` selects an isolated
clean build. Build the reference oracle with `fire_reference_build.sh` and replay with
`python3 reference/compare_fire_target.py --directory RUN --output CHECKS`.
Pass `CHECKS/original-verification.json` to
`fire_verify.py --directory RUN --original CHECKS/original-verification.json`. Finally run
`fire_summary.py RUN_ROOT`. `FIRE_REFERENCE_BUILD_ROOT` selects an isolated oracle output; set
`NTM_SHOT_APPLICATION_LIBRARY` to that output's `oracle.so` when using a
nondefault location. `fire_failure_run.sh` reproduces the two DOS failure boots;
choose a new `FIRE_FAILURE_ROOT` and inspect the saved shell screenshots. Never alter the original engine to make a
reference pass.
