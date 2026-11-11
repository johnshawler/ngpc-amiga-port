#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/native-selector"
mkdir -p "$OUT"
SOURCES=("$ROOT/native-renderer/amiga/selector_frontend.c" "$ROOT/native-renderer/amiga/selector_test.c" "$ROOT/native-renderer/scene_renderer.c" "$ROOT/native-renderer/tile_renderer.c")
FLAGS=(-std=c99 -O2 -Wall -Wextra -Werror)
cc "${FLAGS[@]}" "${SOURCES[@]}" -o "$OUT/test-frontend"
"$OUT/test-frontend"
cc "${FLAGS[@]}" -fsanitize=address,undefined -fno-omit-frame-pointer "${SOURCES[@]}" -o "$OUT/test-frontend-sanitize"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test-frontend-sanitize"
if command -v m68k-linux-gnu-gcc >/dev/null && command -v qemu-m68k >/dev/null; then
    m68k-linux-gnu-gcc "${FLAGS[@]}" -m68020 -static "${SOURCES[@]}" -o "$OUT/test-frontend-68020"
    qemu-m68k -cpu m68020 "$OUT/test-frontend-68020"
fi
