# Original NGPC reference oracle (new reimplementation)

This is a **host-only** behavioral reference, not Amiga code and not evidence of
Amiga performance. It was rebuilt from the pinned official Beetle NeoPop source,
not recovered from a lost runtime. No upstream source changes are required.

## Build and smoke test

From the repository root on Linux with gcc, g++, make, git, tar and Python 3:

```
tools/build-reference.sh
python -m unittest discover -s tests -p 'test_reference*.py'
python reference/run.py
```

The script requires commit `a50d5ac288a81f2104ddf43195a4efdd15c72227`, exports
its committed tree into a fresh temporary directory inside ignored
`reference/build/`, and builds the shared core there. It removes only its own
temporary directory, leaving any older export or user files untouched. Completed
artifacts are installed by atomic per-file rename, with the manifest last. This
is not a transactional multi-file update; do not build concurrently with runs. Toolchain versions, host architecture, artifact hashes
and full compiler commands are retained in `reference/build/build-manifest.txt`
and `build.log`. Repeat builds on the same toolchain should match; this is not
a cross-toolchain binary reproducibility claim. No downloads or installs occur.
The exported source keeps its upstream licenses. The clock shim is separate.

## Private cartridge run

```
mkdir -p reference/runs
python reference/run.py --rom /private/game.ngc --frames 600 \
  --inputs reference/inputs.example.json --replay-check 120 \
  --state-out reference/runs/final.state > reference/runs/game.jsonl
```

Do not publish cartridge bytes, save states or other cartridge-derived artifacts.
Builds and runs are ignored. A temporary empty save directory is used for every
process and removed on exit; no existing flash saves are loaded or overwritten.
The full cartridge buffer is retained throughout the run. Inputs are zero-based
frame numbers mapped to lists of NGPC `a`, `b`, `up`, `down`, `left`, `right`,
`option`. Missing frames release all buttons; repeated buttons are deduplicated.
`a`/`b` follow the NGPC labels (libretro B/A respectively).

The process restarts once with its local clock shim in `LD_PRELOAD`, sets UTC,
and checks the actual `time()` symbol. RTC is deliberately **frozen**, default
2000-01-01 00:00:00 UTC, configurable with `--epoch`. It does not advance with
frames and is not a test of real-time RTC behavior. Language is English.

JSONL records identify core and ROM SHA256, input hash, frozen time, pixel format,
host byte order, and source-core 60.25 Hz / 44100 Hz timing. Each frame hashes only
visible RGB565 rows (excludes pitch padding) and interleaved stereo int16 samples.
Hashes are host-byte-order dependent. Final libretro serialization size/hash is
reported; optional `--state-out` retains it. `--replay-check N` serializes after
the requested frames, runs N frames, restores, reruns identical inputs and
compares video, audio and final serialized state separately. Every mismatching
video/audio frame is reported. Any mismatch fails the process. Original-core
sound restore may reset resampler buffers, so PCM mismatch must be investigated
separately from video divergence; there is no silent warm-up exemption. Upstream restore returns
true unconditionally, so the subsequent comparison, not that return value alone,
is the evidence of replay correctness. Serialized bytes may include core-private
layout and should only be used with the identical core/build.

No-ROM mode verifies core loading, callback initialization, clock setup and
metadata only. It does not exercise video, audio, emulated instructions, input
response, serialization or gameplay. Run results must distinguish those checks
from actual private cartridge tests. Even successful game replay only checks
reference determinism; it does not establish native-port equivalence.

## Read-only CPU/RAM/GPU oracle

Build the instrumented core separately with bash tools/build-reference.sh --instrumented.
The source export appends state-export.c.inc to its private build copy of
libretro.c. The pinned submodule and unmodified core are unchanged. The build
has a distinct hash and directory, and still executes the original game.

Run: python3 reference/check_state_export.py --rom private-inputs/neo-turf-masters.ngc --frames 600
This compares clean original and instrumented runs, including original video/audio
and final serialization. It exercises the read-only export on every frame.
Temporary commercial-derived snapshots are removed when that comparison ends.
To retain private captures, use run.py with --core pointing to the instrumented
core and --snapshot-dir inside ignored private-inputs or reference/runs.

NGR1 version 1 is exactly 32,876 bytes. All numeric words are big-endian:
- Header: magic NGR1, version 1, total byte count, 23 register words.
- Registers: pc, sr, f_dash, gprBank[0..3][0..3], gpr[0..3], each stored as u32.
- 16,384 RAM bytes representing source addresses 0x4000–0x7fff.
- 16,384 GPU readable bytes representing 0x8000–0xbfff.

This is a frame-boundary observation, not a restorable full-machine snapshot.
GPU MMIO uses the pinned read-only ngpgfx_read8 implementation, including raster
position and zero for unmapped reads. Internal rendering latches, timer phase,
DMA and sound-private state are not exported. Do not assume this final-frame
GPU state alone can reproduce every pixel drawn earlier in that frame.

The 600-frame private input sequence passes original versus instrumented video,
audio and final-state equivalence. Exported RAM matches libretro's independent
RAM API; repeated reads are stable and short output buffers reject atomically.
This establishes a bounded observation oracle, not native routine correctness.

Native control-state comparison: `tools/build-reference.sh --control` and
`run.py --control-check` observe the original input-state routine. Optional
`--control-cases` runs isolated original/native differential cases before frames.
See `native-game/README.md` for scope, exactness gates and target timing limits.
