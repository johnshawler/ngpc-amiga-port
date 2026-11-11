#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT="$ROOT/build/shot-raster-check"
mkdir -p "$OUT"
CC=${CC:-cc}
CROSS_CC=${CROSS_CC:-m68k-linux-gnu-gcc}
QEMU=${QEMU:-qemu-m68k}
FLAGS=(-std=c99 -O2 -Wall -Wextra -Werror)
CORE=("$ROOT/native-game/shot_raster.c")
for kind in service compositor; do
    SOURCES=("${CORE[@]}")
    if [[ $kind == service ]]; then SOURCES+=("$ROOT/native-game/test_shot_raster.c");
    else SOURCES+=("$ROOT/native-renderer/scene_renderer.c" "$ROOT/native-renderer/tile_renderer.c" "$ROOT/native-renderer/test_shot_raster.c"); fi
    "$CC" "${FLAGS[@]}" "${SOURCES[@]}" -o "$OUT/test-$kind"
    "$OUT/test-$kind"
    "$CC" "${FLAGS[@]}" -fsanitize=address,undefined -fno-omit-frame-pointer "${SOURCES[@]}" -o "$OUT/test-$kind-sanitize"
    ASAN_OPTIONS=detect_leaks=0 "$OUT/test-$kind-sanitize"
    "$CROSS_CC" "${FLAGS[@]}" -m68020 -static "${SOURCES[@]}" -o "$OUT/test-$kind-m68020"
    "$QEMU" -cpu m68020 "$OUT/test-$kind-m68020"
done
"$CROSS_CC" "${FLAGS[@]}" -m68020 -ffreestanding -fno-builtin -c "${CORE[0]}" -o "$OUT/freestanding.o"
"$CC" "${FLAGS[@]}" -shared -fPIC "${CORE[@]}" -o "$OUT/native.so"
python3 "$ROOT/reference/compare_shot_raster.py" >"$OUT/original.json"
cat "$OUT/original.json"
