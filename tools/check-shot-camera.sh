#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/shot-camera-check
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror'
SOURCES='native-game/shot_camera.c native-game/shot_markers.c native-game/shot_math.c native-game/tee_objects.c native-game/game_services.c native-game/task_allocation.c native-game/task_lifecycle.c native-game/course_choice.c native-game/input_state.c'
cc $FLAGS $SOURCES native-game/test_shot_camera.c -o build/shot-camera-check/test
build/shot-camera-check/test
cc $FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer $SOURCES native-game/test_shot_camera.c -o build/shot-camera-check/test-sanitize
ASAN_OPTIONS=detect_leaks=0 build/shot-camera-check/test-sanitize
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -msoft-float -ffreestanding -fno-builtin -c native-game/shot_camera.c -o build/shot-camera-check/camera-68020.o
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -static $SOURCES native-game/test_shot_camera.c -o build/shot-camera-check/test-68020
${QEMU:-qemu-m68k} -cpu m68020 build/shot-camera-check/test-68020
cc $FLAGS -shared -fPIC $SOURCES -o build/shot-camera-check/native.so
export NTM_SHOT_CAMERA_LIBRARY="$ROOT/build/shot-camera-check/native.so"
for name in shot_camera shot_camera_exit; do
 python3 "reference/compare_$name.py" > "build/shot-camera-check/$name.json" 2> "build/shot-camera-check/$name.log"
 cat "build/shot-camera-check/$name.json"
done
