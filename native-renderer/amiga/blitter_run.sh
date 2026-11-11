#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/native-blitter-benchmark"
: "${PUAE_CORE:?Set PUAE_CORE to built official libretro PUAE shared core}"
: "${LIBRETRO_INCLUDE:?Set LIBRETRO_INCLUDE to official libretro.h directory}"
: "${KICKSTART:?Set KICKSTART to locally authorized A1200 3.1 ROM}"
mkdir -p "$OUT/run"
cp "$KICKSTART" "$OUT/run/kick40068.A1200"
cc -std=c99 -O2 -Wall -Wextra -Werror -I"$LIBRETRO_INCLUDE" "$ROOT/native-renderer/amiga/headless.c" -ldl -Wl,--no-as-needed -lm -o "$OUT/host"
timeout 180 "$OUT/host" "$PUAE_CORE" "$OUT/display.adf" "$OUT/run" "${CAPTURE_FRAMES:-9000}" "$OUT/screen.ppm" "$OUT/chip.bin" >"$OUT/run.log" 2>&1
python3 "$ROOT/native-renderer/amiga/blitter_verify.py" --capture "$OUT/screen.ppm" --chip "$OUT/chip.bin" --log "$OUT/run.log" --report "$OUT/timing.json"
