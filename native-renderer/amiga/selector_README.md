# Native Amiga course-selector frontend

This is an actual input-driven, silent selector slice, not another rendering
microbenchmark and not a complete golf game. It boots an Amiga executable,
initializes the two real backgrounds, course label, palette and animated cursor,
accepts Amiga joystick input, and reaches the converted accept/cancel result.
The original cartridge and firmware are local inputs, not distributed assets.

**Target:** stock PAL A1200 profile, 68EC020, AGA, 2 MiB Chip RAM, zero Fast RAM.
The executable takes over display, interrupts and CIA B timers until emulator
reset. It does not return to the OS. Physical-hardware use has not been tested.
Do not launch it over unsaved work. The previous display/benchmark fixtures and
binaries are separate and unchanged.

## Controls and scope

Use joystick port 2 (the normal Amiga game joystick port):

- Left/right: change Japan / USA / Germany. Initial selection is USA.
- First fire button: accept the selected course.
- Second fire button: cancel.
- Up plus first fire: original modified/random acceptance path.

The Libretro verifier maps RetroPad B to first fire and A to second fire. A
one-button joystick cannot use cancel. After the bounded parent finishes, the
frontend retains its final display; reset the emulator to start again. There is
no subsequent course gameplay, swing/shot physics, sound engine, save state,
keyboard frontend, or OS-return path here.

## Display and scheduling

`selector_main.c` calls the actual native course application. It does not script
selection changes or interpret cartridge CPU instructions. `selector_host.c` is
a separate test host: it supplies RetroPad samples through PUAE's public input
callback and reads report memory to decide when to sample the display. It never
writes native state or cartridge data into emulator memory.

CIA B supplies the time base. The explicit conversion is 241 logical updates
per eight nominal seconds (30.125/s), matching the observed reference scheduler
cadence. This is not a claim about measured physical NGPC timing, and it is not
one source update per PAL display frame. Each input pump performs at most two
due logical updates. Additional elapsed work is counted as dropped instead of
causing an unbounded catch-up loop. This makes a long stall visible in reports.

Rendering uses two complete snapshots and two Chip bitmaps. A displayed frame,
its palette and exported scene always refer to one immutable source snapshot.
New input keeps updating the application while rows of the next frame compose.
The finished bitmap/Copper list is installed in the safe pre-display PAL blank
interval, before line 20 and the line 44 display start. Palette
inversion is applied only at the Copper adapter. Live startup controls were
verified in the original: window control 0x07, pending background 0x81 then 0x80,
viewport (80,0,0,152) then (0,0,160,152), zero scrolling and no plane swap.

The default build has fifteen privately generated planar candidates: three
courses times five cursor shapes. Offline generation runs the actual native
application, rejects a full-window candidate without its 28 live cursor sprites,
and verifies 1,233 baseline-rendered frames covering all normal/accept shapes,
mixed navigation, and 87 fade/exit fallback states. Runtime cache lookup never
changes game state or trusts a selection/frame number as proof of a match.

A hit requires exact controls, both complete map arrays, every sprite record and
sprite palette index, and all tile bytes referenced by either map or any sprite
record. Unreferenced tile bytes are irrelevant to the renderer and may retain
old label data. This avoids both false hits and unnecessary history-dependent
misses. Palette RGB is separate because cached pixels are pen indices: each
snapshot still builds its own Copper palette, including inversion. Lookup
statistics are cumulative; the displayed entry and its lookup duration are
latched with the presented snapshot.

Any mismatch, including non-full fade viewports, uses the existing renderer.
Exact dirty-row checks compare changed maps/tiles and resolved linked sprites;
unchanged rows come from the previous complete display. No hashes authorize
reuse. Matching, invalidation, cached-copy chunks and palette building yield to
the input pump. Small 68020 assembly helpers copy/compare exact byte spans,
without compiler aliasing assumptions or overreads. Tests cover all sixteen
independent source/destination alignments.

This protects coherent output, **not full-speed presentation**. The measured
cached selector still responds visually in about half a second. Intermediate
cursor/fade/random-selection frames can be skipped, and fallback frames can take
seconds. Presses entirely between input polls can be missed. Every source
animation frame is not guaranteed to be displayed.

## Reproduce privately

Set up the existing m68020 toolchain, and provide the official conversion tools:

```sh
export AMIGA_BIN=/path/to/vscode-amiga-debug/bin/linux
bash native-renderer/amiga/selector_build.sh
```

The build reads the authorized cartridge at
`private-inputs/neo-turf-masters.ngc`. The authored exporter follows bounded,
validated tables and writes the backgrounds, labels, palette sources, fade and
random tables, and both cursor animations under ignored `build/native-selector/`.
The exporter refuses output outside this checkout's ignored build directory.
The build also writes the private exact-key/planar include there.
The generated include, executable, ADF, screenshots, RAM captures and firmware
must remain private. There are no original asset bytes in public source.

Run each target flow with the official PUAE core and authorized A1200 firmware:

```sh
export PUAE_CORE=/path/to/puae_libretro.so
export LIBRETRO_INCLUDE=/path/to/libretro-common/include
export KICKSTART=/path/to/authorized-A1200-3.1.rom
bash native-renderer/amiga/selector_run.sh accept
bash native-renderer/amiga/selector_run.sh cancel
bash native-renderer/amiga/selector_run.sh random
```

For the identical frontend without the planar cache:

```sh
bash native-renderer/amiga/selector_build.sh --uncached
bash native-renderer/amiga/selector_run.sh accept --uncached
bash native-renderer/amiga/selector_run.sh cancel --uncached
bash native-renderer/amiga/selector_run.sh random --uncached
python3 tools/compare-selector-performance.py build/native-selector-uncached build/native-selector
```

The uncached build uses the same memory helpers, presentation timing and hardware
input test script. Its files stay separate under `build/native-selector-uncached/`.
Each scenario has its own ignored output directory and native host. Run reports
include private authored-source manifests. The CPU-target executable has no
unresolved libc/compiler helper dependencies.

Cartridge-free boundary tests:

```sh
bash native-renderer/amiga/selector_test.sh
python3 -m unittest discover -s tests -p test_selector_frontend.py -v
```

These test joystick decoding, both palette polarities, exact nominal source
cadence, bounded long stalls, and an exporter regression where the source palette
group count differs from cursor geometry count. They check 384 randomized
invalidation cases against full baseline rendering, plus 4,653 exact-key misses
across every control/map/sprite/palette-index/referenced-tile byte and 160 fade
widths. An unreferenced-tile mutation must still hit and render identically. The script runs host, ASan/UBSan and, when the
tools are available, QEMU m68020 tests.

## Verification and present performance

The actual target verifier checks initial, six mixed-navigation and completed frames in
all three flows. It independently reconstructs pixels with destination/depth
logic and checks all 81,920 displayed planar bytes. All 512 palette bytes are
independently resolved from the cartridge jobs, including the final command 84
overwrite on acceptance, rather than trusted from target staging. It checks all
256 AGA palette entries and both nibble banks, and every captured RGB pixel, including the border. It
also checks the stock memory/chipset profile, source-update drops and final
accept/cancel result. The random test must observe the random-delay branch,
not merely the same final result as ordinary acceptance.

The final measured timings and memory values are in each private scenario's
`verification.json`. All three cached and all three identical uncached flows pass
all eight captures, including independently resolved source palettes. No logical
updates were dropped. These are emulator results, not physical-hardware
measurements or a full-game speed claim.

- Cached navigation composition: **0.239–0.260 s**; identical uncached frontend:
  **2.720–2.727 s**, a measured **10.46–11.38 times** composition improvement.
- Button-to-verified-visible capture: **500–560 ms**, versus **4,560–4,620 ms**
  uncached. The capture guard waits for two stable frames; this is not a claim
  about the first illuminated pixel. Milliseconds use nominal PAL 50 Hz.
- Maximum observed input-poll gap: **54.19 ms** cached, including startup.
  Cache lookup costs remain visible in reports, rather than timing only the copy.
- Cold floppy boot to verified full selector: **35.98 s** cached versus
  **10.40 s** uncached. The larger linked cache makes startup materially slower.

The historical published checkpoint had 3.385 s full composition and
2.824–3.004 s navigation. The cache-specific comparison above instead uses an
uncached build with the same updated copy/presentation helpers, so those other
improvements are not incorrectly credited to caching.

Display allocation remains exactly 170,000 Chip bytes: two 81,920-byte bitmaps,
two nonoverlapping 2,304-byte Copper reservations, and report/header space.
The cache adds **555,120 bytes** of linked static read-only data: 364,800 planar
bytes and 190,320 exact-key bytes, including alignment padding. It remains
allocated for the executable's entire lifetime, even after selection finishes.
A future integrated game needs reclaimable/overlay storage or a new memory
budget; this checkpoint does not implement either.

The cached executable is 607,080 bytes, with 606,021 ELF text bytes and 69,904 BSS
bytes. Exec reports **1,139,664 free Chip bytes** after loading it and allocating
the display. The identical uncached executable is 51,612 bytes and leaves
1,695,104 Chip bytes. Reports include `AvailMem(MEMF_CHIP)` before and after the
explicit display allocation; explicit allocation alone is not total program/OS
memory use. This is memory headroom for the bounded selector, not proof that the
later game fits. Later gameplay, audio and its rendering strategy remain work.
