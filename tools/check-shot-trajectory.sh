#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
OUT=build/shot-trajectory-check
mkdir -p "$OUT/native"
cp native-game/*.[ch] "$OUT/native/"
# Compile unmodified shared sources in this separate fire-child build directory.

SOURCES="$OUT/native/shot_trajectory.c $OUT/native/tee_objects.c $OUT/native/course_palette.c $OUT/native/course_screen.c $OUT/native/game_services.c"
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror'
cc $FLAGS "$OUT/native/test_shot_trajectory.c" $SOURCES -o "$OUT/test"
"$OUT/test"
cc $FLAGS -g -fsanitize=address,undefined -fno-omit-frame-pointer "$OUT/native/test_shot_trajectory.c" $SOURCES -o "$OUT/test-sanitize"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test-sanitize"
for variant in default private; do
 EXTRA=''
 if [ "$variant" = private ]; then EXTRA='-DNTM_SCREEN_PRIVATE_ASSETS native-renderer/amiga/intro_copy.c native-renderer/amiga/selector_memory.c'; fi
 cc $FLAGS -shared -fPIC $EXTRA $SOURCES -o "$OUT/native-$variant.so"
 NTM_SHOT_TRAJECTORY_LIBRARY="$ROOT/$OUT/native-$variant.so" python3 reference/compare_shot_trajectory.py > "$OUT/oracle-$variant.json" 2> "$OUT/oracle-$variant.log"
 cat "$OUT/oracle-$variant.json"
 NTM_SHOT_TRAJECTORY_LIBRARY="$ROOT/$OUT/native-$variant.so" python3 reference/compare_shot_trajectory_objects.py > "$OUT/objects-$variant.json" 2> "$OUT/objects-$variant.log"
 cat "$OUT/objects-$variant.json"
done
if command -v m68k-linux-gnu-gcc >/dev/null 2>&1; then
 m68k-linux-gnu-gcc $FLAGS -m68020 -static "$OUT/native/test_shot_trajectory.c" $SOURCES -o "$OUT/test-m68020"
 qemu-m68k "$OUT/test-m68020"
fi
