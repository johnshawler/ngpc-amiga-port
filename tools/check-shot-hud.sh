#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT="$ROOT/build/shot-hud-check"
mkdir -p "$OUT"
SOURCES=("$ROOT/native-game/shot_hud.c" "$ROOT/native-game/tee_graphics.c" "$ROOT/native-game/course_screen.c")
FLAGS=(-std=c99 -O2 -Wall -Wextra -Werror)
cc "${FLAGS[@]}" "${SOURCES[@]}" "$ROOT/native-game/test_shot_hud.c" -o "$OUT/test"
"$OUT/test"
cc "${FLAGS[@]}" -fsanitize=address,undefined -fno-omit-frame-pointer "${SOURCES[@]}" "$ROOT/native-game/test_shot_hud.c" -o "$OUT/test-sanitize"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test-sanitize"
m68k-linux-gnu-gcc "${FLAGS[@]}" -m68020 -static "${SOURCES[@]}" "$ROOT/native-game/test_shot_hud.c" -o "$OUT/test-m68020"
qemu-m68k -cpu m68020 "$OUT/test-m68020"
cc "${FLAGS[@]}" -shared -fPIC "${SOURCES[@]}" -o "$OUT/native.so"
python3 "$ROOT/reference/compare_shot_hud.py"
cc "${FLAGS[@]}" -shared -fPIC -DNTM_SCREEN_PRIVATE_ASSETS "${SOURCES[@]}" "$ROOT/native-renderer/amiga/intro_copy.c" "$ROOT/native-renderer/amiga/selector_memory.c" -o "$OUT/private.so"
NTM_SHOT_HUD_LIBRARY="$OUT/private.so" python3 "$ROOT/reference/compare_shot_hud.py"
