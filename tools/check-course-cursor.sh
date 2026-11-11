#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/cursor-check
bash tools/check-course-initialization.sh > build/cursor-check/startup-regression.log
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror -shared -fPIC'
cc $FLAGS native-game/course_cursor.c native-game/game_services.c -o build/cursor-check/native_cursor_final.so
cc $FLAGS -I native-game reference/course_application_bridge.c native-game/input_state.c native-game/course_choice.c native-game/task_lifecycle.c native-game/task_allocation.c native-game/random_byte.c native-game/viewport.c native-game/game_services.c native-game/course_parent.c native-game/course_screen.c native-game/course_palette.c native-game/course_initialization.c native-game/course_cursor.c -o build/cursor-check/native_application_final.so
for suite in cursor cursor_queue application; do
    python3 "reference/compare_course_$suite.py" > "build/cursor-check/final-$suite.json" 2> "build/cursor-check/final-$suite.log"
    cat "build/cursor-check/final-$suite.json"
done
