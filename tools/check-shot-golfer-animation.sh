#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/shot-golfer-animation-check
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror'
SOURCES='native-game/shot_golfer_animation.c native-game/course_screen.c'
cc $FLAGS $SOURCES native-game/test_shot_golfer_animation.c -o build/shot-golfer-animation-check/test
build/shot-golfer-animation-check/test
cc $FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer $SOURCES native-game/test_shot_golfer_animation.c -o build/shot-golfer-animation-check/test-sanitize
ASAN_OPTIONS=detect_leaks=0 build/shot-golfer-animation-check/test-sanitize
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -msoft-float -ffreestanding -fno-builtin -c native-game/shot_golfer_animation.c -o build/shot-golfer-animation-check/golfer-animation-68020.o
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -static $SOURCES native-game/test_shot_golfer_animation.c -o build/shot-golfer-animation-check/test-68020
${QEMU:-qemu-m68k} -cpu m68020 build/shot-golfer-animation-check/test-68020
for variant in default private; do
 EXTRA=''
 if [ "$variant" = private ]; then EXTRA='-DNTM_SCREEN_PRIVATE_ASSETS native-renderer/amiga/intro_copy.c native-renderer/amiga/selector_memory.c'; fi
 cc $FLAGS -shared -fPIC $EXTRA $SOURCES -o "build/shot-golfer-animation-check/native-$variant.so"
 export NTM_SHOT_GOLFER_ANIMATION_LIBRARY="$ROOT/build/shot-golfer-animation-check/native-$variant.so"
 python3 reference/compare_shot_golfer_animation.py > "build/shot-golfer-animation-check/$variant.json" 2> "build/shot-golfer-animation-check/$variant.log"
 cat "build/shot-golfer-animation-check/$variant.json"
done
