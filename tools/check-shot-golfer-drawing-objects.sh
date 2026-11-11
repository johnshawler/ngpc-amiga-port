#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
OUT=build/shot-golfer-drawing-object-check
mkdir -p "$OUT"
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror'
SOURCES='native-game/tee_objects.c native-game/game_services.c'
TEST=native-game/test_shot_golfer_drawing_objects.c
cc $FLAGS $SOURCES "$TEST" -o "$OUT/test"
"$OUT/test"
cc $FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer $SOURCES "$TEST" -o "$OUT/test-sanitize"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test-sanitize"
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -msoft-float -ffreestanding -fno-builtin -c native-game/tee_objects.c -o "$OUT/objects-68020.o"
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -static $SOURCES "$TEST" -o "$OUT/test-68020"
${QEMU:-qemu-m68k} -cpu m68020 "$OUT/test-68020"
for variant in default private; do
 EXTRA=''
 if [ "$variant" = private ]; then EXTRA='-DNTM_SCREEN_PRIVATE_ASSETS native-renderer/amiga/intro_copy.c native-renderer/amiga/selector_memory.c'; fi
 cc $FLAGS -shared -fPIC $EXTRA $SOURCES -o "$OUT/native-$variant.so"
 export NTM_SHOT_GOLFER_LIBRARY="$ROOT/$OUT/native-$variant.so"
 python3 reference/compare_shot_golfer_drawing_objects.py > "$OUT/oracle-$variant.json" 2> "$OUT/oracle-$variant.log"
 cat "$OUT/oracle-$variant.json"
done
