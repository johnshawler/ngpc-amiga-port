#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/shot-power-meter-check
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror'
SOURCES='native-game/shot_power_meter.c native-game/shot_scene_objects.c native-game/tee_graphics.c native-game/tee_objects.c native-game/game_services.c native-game/task_lifecycle.c native-game/course_choice.c native-game/input_state.c'
cc $FLAGS $SOURCES native-game/test_shot_power_meter.c -o build/shot-power-meter-check/test
build/shot-power-meter-check/test
cc $FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer $SOURCES native-game/test_shot_power_meter.c -o build/shot-power-meter-check/test-sanitize
ASAN_OPTIONS=detect_leaks=0 build/shot-power-meter-check/test-sanitize
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -msoft-float -ffreestanding -fno-builtin -c native-game/shot_power_meter.c -o build/shot-power-meter-check/meter-68020.o
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -static $SOURCES native-game/test_shot_power_meter.c -o build/shot-power-meter-check/test-68020
${QEMU:-qemu-m68k} -cpu m68020 build/shot-power-meter-check/test-68020
cc $FLAGS -shared -fPIC $SOURCES -o build/shot-power-meter-check/native.so
export NTM_SHOT_POWER_METER_LIBRARY="$ROOT/build/shot-power-meter-check/native.so"
python3 reference/compare_shot_power_meter.py > build/shot-power-meter-check/oracle.json 2> build/shot-power-meter-check/oracle.log
cat build/shot-power-meter-check/oracle.json
