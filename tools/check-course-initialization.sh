#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/course-init-check
bash tools/build-reference.sh --screen-probe > build/course-init-check/probe-build.log
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror -shared -fPIC'
cc $FLAGS native-game/course_palette.c -o build/course-init-check/native_palette.so
cc $FLAGS native-game/course_palette.c native-game/course_initialization.c native-game/course_screen.c native-game/game_services.c -o build/course-init-check/native_initialization.so
cc $FLAGS -I native-game reference/course_driver_bridge.c native-game/input_state.c native-game/course_choice.c native-game/task_lifecycle.c native-game/task_allocation.c native-game/random_byte.c native-game/viewport.c native-game/game_services.c native-game/course_parent.c native-game/course_screen.c native-game/course_palette.c -o build/course-init-check/native_driver.so
for suite in palette initialization driver; do
    python3 "reference/compare_course_$suite.py" > "build/course-init-check/final-$suite.json" 2> "build/course-init-check/final-$suite.log"
    cat "build/course-init-check/final-$suite.json"
done
