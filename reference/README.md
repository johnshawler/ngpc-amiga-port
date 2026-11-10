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
