#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/native-display"
EXTRA=()
case "${1:-}" in
    '') ;;
    --scene) OUT="$ROOT/build/native-scene-display"; EXTRA=(--scene) ;;
    *) echo 'usage: run.sh [--scene]' >&2; exit 2 ;;
esac
: "${PUAE_CORE:?Set PUAE_CORE to built official libretro PUAE shared core}"
: "${LIBRETRO_INCLUDE:?Set LIBRETRO_INCLUDE to official libretro.h directory}"
: "${KICKSTART:?Set KICKSTART to locally authorized A1200 3.1 ROM}"
mkdir -p "$OUT/run"
cp "$KICKSTART" "$OUT/run/kick40068.A1200"
cc -std=c99 -O2 -Wall -Wextra -Werror -I"$LIBRETRO_INCLUDE" "$ROOT/native-renderer/amiga/headless.c" -ldl -Wl,--no-as-needed -lm -o "$OUT/host"
timeout 180 "$OUT/host" "$PUAE_CORE" "$OUT/display.adf" "$OUT/run" 1200 "$OUT/screen.ppm" "$OUT/chip.bin" >"$OUT/run.log" 2>&1
python3 "$ROOT/native-renderer/amiga/verify.py" "${EXTRA[@]}" --capture "$OUT/screen.ppm" --chip "$OUT/chip.bin" --log "$OUT/run.log" --report "$OUT/verification.json"
