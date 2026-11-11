#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/shot-accuracy-check
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror'
SOURCES='native-game/shot_accuracy.c native-game/task_lifecycle.c native-game/course_choice.c native-game/input_state.c'
cc $FLAGS $SOURCES native-game/test_shot_accuracy.c -o build/shot-accuracy-check/test
build/shot-accuracy-check/test
cc $FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer $SOURCES native-game/test_shot_accuracy.c -o build/shot-accuracy-check/test-sanitize
ASAN_OPTIONS=detect_leaks=0 build/shot-accuracy-check/test-sanitize
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -msoft-float -ffreestanding -fno-builtin -c native-game/shot_accuracy.c -o build/shot-accuracy-check/accuracy-68020.o
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -static $SOURCES native-game/test_shot_accuracy.c -o build/shot-accuracy-check/test-68020
${QEMU:-qemu-m68k} -cpu m68020 build/shot-accuracy-check/test-68020
cc $FLAGS -shared -fPIC $SOURCES -o build/shot-accuracy-check/native.so
export NTM_SHOT_ACCURACY_LIBRARY="$ROOT/build/shot-accuracy-check/native.so"
python3 reference/compare_shot_accuracy.py > build/shot-accuracy-check/oracle.json 2> build/shot-accuracy-check/oracle.log
cat build/shot-accuracy-check/oracle.json
