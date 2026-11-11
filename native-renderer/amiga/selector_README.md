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
The finished bitmap/Copper list is installed at a PAL frame boundary. Palette
inversion is applied only at the Copper adapter. Live startup controls were
verified in the original: window control 0x07, pending background 0x81 then 0x80,
viewport (80,0,0,152) then (0,0,160,152), zero scrolling and no plane swap.

Exact byte comparisons identify changed tiles and maps. Resolved linked-sprite
positions, descriptors, palette indices and referenced tiles invalidate affected
rows. Changed window/scroll/layer controls invalidate the full frame. Unchanged
rows are copied from the previous complete display, without composing them again.
No hashes or assumptions about a static cursor authorize reuse. Invalidation
itself yields to the input pump in bounded chunks. Palette changes still produce
a new Copper palette even when pixel indices are unchanged.

This protects coherent output, **not real-time visual responsiveness**. Complete
frames can lag input by seconds, and intermediate cursor/fade/random-selection
animation frames can be skipped. Presses entirely between input polls can also
be missed. The latest complete scene is correct; every source animation frame
is not guaranteed to be displayed.

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
group count differs from cursor geometry count. It also checks 384 randomized
invalidation cases against full
independent baseline rendering. The script runs host, ASan/UBSan and, when the
tools are available, QEMU m68020 tests.

## Verification and present performance

The actual target verifier checks initial, right, left and completed frames in
all three flows. It independently reconstructs pixels with destination/depth
logic and checks all 81,920 displayed planar bytes. All 512 palette bytes are
independently resolved from the cartridge jobs, including the final command 84
overwrite on acceptance, rather than trusted from target staging. It checks all
256 AGA palette entries
and both nibble banks, and every captured RGB pixel, including the border. It
also checks the stock memory/chipset profile, source-update drops and final
accept/cancel result. The random test must observe the random-delay branch,
not merely the same final result as ordinary acceptance.

The final measured timings and memory values are in each private scenario's
`verification.json`. Current source/cursor integration is deliberately slow:
full coherent composition is about 3.39 seconds, navigation compositions about
2.82–3.00 seconds, with a measured maximum input-poll gap of about 56.84 ms. There were zero
dropped logical updates in the three tested flows. These are emulator results,
not physical-hardware measurements or a full-speed claim.

Display allocation is exactly 170,000 Chip bytes: two 81,920-byte bitmaps, two
nonoverlapping 2,304-byte Copper reservations, and report/header space. The
startup report records Exec `AvailMem(MEMF_CHIP)` before/after that allocation,
after the executable and its BSS have already loaded. The build prints ELF
text/data/BSS sizes. Do not confuse explicit display allocation with total
program/OS memory use. In the final runs, the loaded executable was 50,596
bytes, with 49,611 bytes ELF text and 69,864 bytes BSS. Exec reported
1,866,112 free Chip bytes before the display allocation and 1,696,112 afterward.
This leaves room for the proposed 364,800-byte cache in this bounded slice; it
does not reserve memory for the unimplemented later game.

One proposed next optimization is a strictly keyed, privately generated
planar cache for the three selection labels and five cursor shapes (15 native
160x152 frames = 364,800 bytes). Native application updates would still determine
selection, animation, viewport and palette. Any state outside a proven cache key
must fall back to the exact renderer; fades require separate validated clipping.
This could trade available Chip RAM for much faster display changes, but has not
been implemented or measured here. Later game scenes still need their own
rendering strategy.
