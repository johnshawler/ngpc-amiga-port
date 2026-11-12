#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/swing-exit-check
FLAGS='-O2 -std=c99 -Wall -Wextra -Werror -Inative-game'
COMMON='native-game/shot_swing_exit.c native-game/tee_objects.c native-game/game_services.c native-game/task_allocation.c native-game/task_lifecycle.c native-game/course_choice.c native-game/input_state.c'
cc $FLAGS native-game/test_shot_swing_exit.c $COMMON -o build/swing-exit-check/test
build/swing-exit-check/test
cc $FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer native-game/test_shot_swing_exit.c $COMMON -o build/swing-exit-check/test-sanitize
ASAN_OPTIONS=detect_leaks=0 build/swing-exit-check/test-sanitize
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -static native-game/test_shot_swing_exit.c $COMMON -o build/swing-exit-check/test-68020
${QEMU:-qemu-m68k} -cpu m68020 build/swing-exit-check/test-68020
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -msoft-float -ffreestanding -fno-builtin -c native-game/shot_swing_exit.c -o build/swing-exit-check/exit-68020.o
if [ "${NTM_SWING_EXIT_SKIP_ORACLE:-0}" = 0 ]; then
 cc $FLAGS -shared -fPIC $COMMON -o build/swing-exit-check/native.so
 python3 reference/compare_shot_swing_exit.py >build/swing-exit-check/oracle.json 2>build/swing-exit-check/oracle.log
 cat build/swing-exit-check/oracle.json
fi
