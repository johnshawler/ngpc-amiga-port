#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/shot-spin-check
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror'
SOURCES='native-game/shot_spin.c native-game/task_lifecycle.c native-game/course_choice.c native-game/input_state.c native-game/course_screen.c'
cc $FLAGS $SOURCES native-game/test_shot_spin.c -o build/shot-spin-check/test
build/shot-spin-check/test
cc $FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer $SOURCES native-game/test_shot_spin.c -o build/shot-spin-check/test-sanitize
ASAN_OPTIONS=detect_leaks=0 build/shot-spin-check/test-sanitize
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -msoft-float -ffreestanding -fno-builtin -c native-game/shot_spin.c -o build/shot-spin-check/spin-68020.o
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -static $SOURCES native-game/test_shot_spin.c -o build/shot-spin-check/test-68020
${QEMU:-qemu-m68k} -cpu m68020 build/shot-spin-check/test-68020
cc $FLAGS -shared -fPIC $SOURCES -o build/shot-spin-check/native.so
export NTM_SHOT_SPIN_LIBRARY="$ROOT/build/shot-spin-check/native.so"
python3 reference/compare_shot_spin.py > build/shot-spin-check/oracle.json 2> build/shot-spin-check/oracle.log
cat build/shot-spin-check/oracle.json
python3 reference/compare_shot_spin_control.py > build/shot-spin-check/control.json 2> build/shot-spin-check/control.log
cat build/shot-spin-check/control.json
cc $FLAGS -shared -fPIC -DNTM_SCREEN_PRIVATE_ASSETS $SOURCES native-renderer/amiga/intro_copy.c native-renderer/amiga/selector_memory.c -o build/shot-spin-check/native_private.so
export NTM_SHOT_SPIN_LIBRARY="$ROOT/build/shot-spin-check/native_private.so"
python3 reference/compare_shot_spin_control.py > build/shot-spin-check/control-private.json 2> build/shot-spin-check/control-private.log
cat build/shot-spin-check/control-private.json
