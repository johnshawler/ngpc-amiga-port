#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/native-display"
EXTRA=()
FRAMES=1200
case "${1:-}" in
    '') ;;
    --scene) OUT="$ROOT/build/native-scene-display"; EXTRA=(--scene) ;;
    --cached) OUT="$ROOT/build/native-cached-display"; EXTRA=(--scene) ;;
    --benchmark) OUT="$ROOT/build/native-benchmark"; FRAMES=9000; EXTRA=(--scene) ;;
    --planar-benchmark) OUT="$ROOT/build/native-planar-benchmark"; FRAMES=9000; EXTRA=(--scene) ;;
    --composition-benchmark) OUT="$ROOT/build/native-composition-benchmark"; FRAMES=9000; EXTRA=(--scene) ;;
    --direct-benchmark) OUT="$ROOT/build/native-direct-benchmark"; FRAMES=9000; EXTRA=(--scene) ;;
    --event-benchmark) OUT="$ROOT/build/native-event-benchmark"; FRAMES=9000; EXTRA=(--scene) ;;
    --adaptive-benchmark) OUT="$ROOT/build/native-adaptive-benchmark"; FRAMES=9000; EXTRA=(--scene) ;;
    --control-benchmark) OUT="$ROOT/build/native-control-benchmark"; FRAMES=9000; EXTRA=(--scene) ;;
    --course-benchmark) OUT="$ROOT/build/native-course-benchmark"; FRAMES=9000; EXTRA=(--scene) ;;
    *) echo 'usage: run.sh [--scene|--cached|--benchmark|--planar-benchmark|--composition-benchmark|--direct-benchmark|--event-benchmark|--adaptive-benchmark|--control-benchmark|--course-benchmark]'  >&2; exit 2 ;;
esac
: "${PUAE_CORE:?Set PUAE_CORE to built official libretro PUAE shared core}"
: "${LIBRETRO_INCLUDE:?Set LIBRETRO_INCLUDE to official libretro.h directory}"
: "${KICKSTART:?Set KICKSTART to locally authorized A1200 3.1 ROM}"
mkdir -p "$OUT/run"
cp "$KICKSTART" "$OUT/run/kick40068.A1200"
cc -std=c99 -O2 -Wall -Wextra -Werror -I"$LIBRETRO_INCLUDE" "$ROOT/native-renderer/amiga/headless.c" -ldl -Wl,--no-as-needed -lm -o "$OUT/host"
timeout 180 "$OUT/host" "$PUAE_CORE" "$OUT/display.adf" "$OUT/run" "${CAPTURE_FRAMES:-$FRAMES}" "$OUT/screen.ppm" "$OUT/chip.bin" >"$OUT/run.log" 2>&1
python3 "$ROOT/native-renderer/amiga/verify.py" "${EXTRA[@]}" --capture "$OUT/screen.ppm" --chip "$OUT/chip.bin" --log "$OUT/run.log" --report "$OUT/verification.json"

if [ "${1:-}" = --benchmark ]; then
    python3 "$ROOT/native-renderer/amiga/verify_benchmark.py" --chip "$OUT/chip.bin" --log "$OUT/run.log" --report "$OUT/timing.json"
fi

if [ "${1:-}" = --planar-benchmark ]; then
    python3 "$ROOT/native-renderer/amiga/verify_planar_benchmark.py" --chip "$OUT/chip.bin" --log "$OUT/run.log" --report "$OUT/timing.json"
fi

if [ "${1:-}" = --composition-benchmark ]; then
    python3 "$ROOT/native-renderer/amiga/verify_composition_benchmark.py" --chip "$OUT/chip.bin" --log "$OUT/run.log" --report "$OUT/timing.json"
fi

if [ "${1:-}" = --direct-benchmark ]; then
    python3 "$ROOT/native-renderer/amiga/verify_direct_benchmark.py" --chip "$OUT/chip.bin" --log "$OUT/run.log" --report "$OUT/timing.json"
fi

if [ "${1:-}" = --event-benchmark ]; then
    python3 "$ROOT/native-renderer/amiga/verify_event_benchmark.py" --chip "$OUT/chip.bin" --log "$OUT/run.log" --report "$OUT/timing.json"
fi

if [ "${1:-}" = --adaptive-benchmark ]; then
    python3 "$ROOT/native-renderer/amiga/verify_adaptive_benchmark.py" --chip "$OUT/chip.bin" --log "$OUT/run.log" --report "$OUT/timing.json"
fi

if [ "${1:-}" = --control-benchmark ]; then
    python3 "$ROOT/native-renderer/amiga/verify_control_benchmark.py" --chip "$OUT/chip.bin" --log "$OUT/run.log" --report "$OUT/timing.json"
fi

if [ "${1:-}" = --course-benchmark ]; then
    python3 "$ROOT/native-renderer/amiga/verify_course_benchmark.py" --chip "$OUT/chip.bin" --log "$OUT/run.log" --report "$OUT/timing.json"
fi
