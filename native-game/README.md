# Native game-control slice

`input_state.c` is a new native C implementation of Neo Turf Masters' eight-button
state update, rather than a renderer or an instruction interpreter. It is GPL-2.0-
or-later, with license text in `../native-renderer/COPYING`. It contains no ROM
bytes/assets and performs no ROM reads, CPU emulation, allocation or OS calls.
It compiles to a self-contained freestanding m68020 object with no unresolved
symbols.

## Exact scope

The verified original routine begins at 0x2002c2, samples the input register at
0x2002cd, and reaches RET at 0x20042d. Its state occupies sixteen bytes at 0x40bf.
The native function accepts that state and a sampled eight-bit button value:

- Bytes 0/4: current and previous buttons; byte 1: newly pressed buttons.
- Byte 2: periodic-counter mask; byte 3: signed-comparison mask.
- Bytes 5/6: counter-mask and signed-threshold configuration; byte 7 untouched.
- Bytes 8–15: eight held-button counters.

A button counts only when held in both samples. Otherwise its counter resets.
Held counters increment, with an overflow to zero replaced by 128. The periodic
mask is set when the counter AND configuration byte 5 is zero. The other mask
compares that **masked** counter, as a signed byte, against byte 6. The unusual
masked/signed behavior is preserved exactly, not rewritten as an assumed repeat
delay. Configuration/reserved bytes are preserved.

This is state-level equivalence. TLCS scratch registers, flags, stack/calling
conventions and cycle timing are not a replacement ABI. The reference observer
still executes the original routine and compares the native result alongside it.
The Amiga fixture runs the native function directly, but there is not yet a native
interactive game loop, joystick mapping, complete menu state machine, shot physics or
sound integration. These results must not be described as playable golf.

## Differential verification

The optional original-core observer captures input and the 16-byte state on
entry and checks the native result just before RET. Other processors/peripherals
continue updating shared sound RAM between CPU instructions, so the live observer
correctly compares the routine state, not all shared RAM. Isolated routine cases
check that the original writes no other RAM and restore RAM/register state before
normal emulation resumes. Original video/audio/final serialized state match after
those isolated cases, giving an additional restoration check.

- **136,192 isolated original/native cases:** all 65,536 old/new button pairs,
  all 65,536 mask/threshold pairs with varying counters, and 5,120 boundary counter
  cases (all counter values with four masks and five signed thresholds).
- **3,900 live comparisons:** 2,100 calls over the 4,200-frame fairway script and
  1,800 over the 3,600-frame O.B. script. Original video/audio/final state and
  fresh repeated traces match exactly.
- **16,842,752 independent C-oracle cases:** all old/new button pairs plus every
  counter/mask/threshold combination with all buttons held. Host, ASan/UBSan and
  static QEMU m68020 pass. This is not exhaustive over every possible 16-byte
  state and external scheduling interleaving.

This specific routine runs on every second reference display frame in both
scripts, consistently at zero-based frames 1, 3, 5, ... . At the core's reported
60.25 Hz display cadence that is **30.125 input-state updates/second**. This does
not establish the rendering or physics update rate, nor physical NGPC timing.

Private cartridge-based traces, hashes, original bytes and snapshots stay local.
Public CI checks source/native tests and builds the observer without a cartridge.

## Stock target cost

A synthetic native-control benchmark runs four state configurations forward and
reverse, 16,384 calls per pass. Independently computed final state bytes and
digests match, and fresh-boot timing reports are identical.

The routine plus call-loop/sample arithmetic costs **about 105.6–109.3 microseconds
per update (0.106–0.109 ms)** in the stock PAL A1200 emulator. The profile is 68020,
AGA, 2 MiB Chip, zero Fast/Slow RAM, CPU/memory cycle-exact, with full 320x256
eight-plane display DMA active. Chained CIA-B timing calibrates against a
20.0246 ms PAL beam frame. State setup and checksums are outside the interval.

This is the cost of one native routine, not rendering, other game logic, audio,
a whole-game frame or an original-TLCS speed comparison. No physical-hardware or
full-speed game claim follows from it.

## Reproduce

```
make -C native-game test sanitize m68020
bash tools/build-reference.sh --control
python3 reference/run.py --core reference/build/control/mednafen_ngp_libretro.so \
  --rom private-inputs/neo-turf-masters.ngc --control-check --control-cases \
  --frames 4200 --inputs reference/inputs.gameplay-fairway.json
# Repeat, run the O.B. script, then compare saved private JSONL files with
# reference/compare_control_trace.py against the original traces.
bash native-renderer/amiga/build.sh --control-benchmark
bash native-renderer/amiga/run.sh --control-benchmark
python3 -m unittest discover -s tests -v
```

Use the existing official cross tools and emulator/firmware environment variables.
`--control-probe` builds a separate diagnostic reference export that logs a small
number of private input-site instruction traces; it is not used for equivalence
claims. The observer is specific to the verified original game/version, not a
generic ROM translator.

Next gameplay-facing work should translate a consumer of these control-state
outputs and verify its state transitions, then connect a native scheduler/input
adapter. This slice supplies a tested contract for that work without pretending
the remaining game routines have already been converted.

The next verified consumer is the [native course-selection handler](COURSE_CHOICE.md), including a composed input-to-selector tick and explicit sound/finish requests.

## Course-screen integration

See [COURSE_SCREEN.md](COURSE_SCREEN.md) for the native deferred drawing queues,
course-label tile/map loader, parent render adapter and original comparisons.
This advances real drawing; complete interactive application/audio remain unfinished.
