#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/native-shot-tests"
mkdir -p "$OUT"
SOURCES=(native-renderer/amiga/shot_frontend_test.c native-renderer/amiga/shot_frontend.c native-renderer/amiga/selector_frontend.c native-renderer/amiga/selector_memory.c native-game/shot_raster.c native-renderer/tile_renderer.c)
for i in "${!SOURCES[@]}"; do SOURCES[$i]="$ROOT/${SOURCES[$i]}"; done
FLAGS=(-std=c99 -O2 -Wall -Wextra -Werror)
cc "${FLAGS[@]}" "${SOURCES[@]}" -o "$OUT/frontend"
"$OUT/frontend"
cc "${FLAGS[@]}" -fsanitize=address,undefined -fno-omit-frame-pointer "${SOURCES[@]}" -o "$OUT/frontend-sanitize"
ASAN_OPTIONS=detect_leaks=0 "$OUT/frontend-sanitize"
m68k-linux-gnu-gcc "${FLAGS[@]}" -m68020 -static -DSELECTOR_FAST_MEMORY "${SOURCES[@]}" "$ROOT/native-renderer/amiga/selector_memory.s" -o "$OUT/frontend-m68020"
qemu-m68k -cpu m68020 "$OUT/frontend-m68020"
python3 "$ROOT/native-renderer/amiga/shot_disk.py" "$ROOT/build/native-shot/shot.adf" "$ROOT/build/native-shot/shot.exe"
python3 "$ROOT/tools/verify-adf-file.py" --adf "$ROOT/build/native-shot/shot.adf" --file shot.exe --source "$ROOT/build/native-shot/shot.exe"
