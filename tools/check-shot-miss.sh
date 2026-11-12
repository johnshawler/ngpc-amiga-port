#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT";OUT=build/shot-miss-check;mkdir -p "$OUT"
SOURCES='native-game/shot_miss.c native-game/course_screen.c'
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror'
cc $FLAGS native-game/test_shot_miss.c $SOURCES -o "$OUT/test";"$OUT/test"
cc $FLAGS -g -fsanitize=address,undefined -fno-omit-frame-pointer native-game/test_shot_miss.c $SOURCES -o "$OUT/test-sanitize"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test-sanitize"
for variant in default private;do
 EXTRA=''
 if [ "$variant" = private ];then EXTRA='-DNTM_SCREEN_PRIVATE_ASSETS native-renderer/amiga/intro_copy.c native-renderer/amiga/selector_memory.c';fi
 cc $FLAGS -shared -fPIC $EXTRA $SOURCES -o "$OUT/native-$variant.so"
 for oracle in miss miss_draw;do
  NTM_SHOT_MISS_LIBRARY="$ROOT/$OUT/native-$variant.so" python3 "reference/compare_shot_$oracle.py" >"$OUT/$oracle-$variant.json" 2>"$OUT/$oracle-$variant.log"
  tail -1 "$OUT/$oracle-$variant.json"
 done
done
if command -v m68k-linux-gnu-gcc >/dev/null 2>&1;then
 m68k-linux-gnu-gcc $FLAGS -m68020 -static native-game/test_shot_miss.c $SOURCES -o "$OUT/test-m68020"
 qemu-m68k "$OUT/test-m68020"
fi
