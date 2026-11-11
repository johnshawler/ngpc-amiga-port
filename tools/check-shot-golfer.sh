#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/shot-golfer-check
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror'
SOURCES='native-game/shot_golfer.c native-game/shot_projection.c native-game/shot_terrain.c native-game/shot_aim.c native-game/shot_math.c native-game/shot_markers.c native-game/tee_objects.c native-game/game_services.c native-game/course_palette.c native-game/course_screen.c native-game/task_lifecycle.c native-game/course_choice.c native-game/input_state.c'
cc $FLAGS $SOURCES native-game/test_shot_golfer.c -o build/shot-golfer-check/test
build/shot-golfer-check/test
cc $FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer $SOURCES native-game/test_shot_golfer.c -o build/shot-golfer-check/test-sanitize
ASAN_OPTIONS=detect_leaks=0 build/shot-golfer-check/test-sanitize
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -msoft-float -ffreestanding -fno-builtin -c native-game/shot_golfer.c -o build/shot-golfer-check/golfer-68020.o
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -static $SOURCES native-game/test_shot_golfer.c -o build/shot-golfer-check/test-68020
${QEMU:-qemu-m68k} -cpu m68020 build/shot-golfer-check/test-68020
cc $FLAGS -shared -fPIC $SOURCES -o build/shot-golfer-check/native.so
export NTM_SHOT_GOLFER_LIBRARY="$ROOT/build/shot-golfer-check/native.so"
for variant in default private; do
 EXTRA=''
 if [ "$variant" = private ]; then EXTRA='-DNTM_SCREEN_PRIVATE_ASSETS native-renderer/amiga/intro_copy.c native-renderer/amiga/selector_memory.c'; fi
 cc $FLAGS -shared -fPIC $EXTRA $SOURCES -o "build/shot-golfer-check/native-$variant.so"
 export NTM_SHOT_GOLFER_LIBRARY="$ROOT/build/shot-golfer-check/native-$variant.so"
 for name in shot_golfer shot_golfer_objects; do
 python3 "reference/compare_$name.py" > "build/shot-golfer-check/$name-$variant.json" 2> "build/shot-golfer-check/$name-$variant.log"
 cat "build/shot-golfer-check/$name-$variant.json"
done
done
