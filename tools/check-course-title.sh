#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Uses an existing pinned screen-probe core. Build it separately with
# tools/build-reference.sh --screen-probe, never during other reference runs.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/course-title-check build/course-init-check build/cursor-check
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror -shared -fPIC'
COMMON='native-game/input_state.c native-game/course_choice.c native-game/task_lifecycle.c native-game/task_allocation.c native-game/random_byte.c native-game/viewport.c native-game/game_services.c native-game/course_parent.c native-game/course_screen.c native-game/course_palette.c native-game/course_initialization.c native-game/course_cursor.c'
cc $FLAGS native-game/course_palette.c -o build/course-init-check/native_palette.so
cc $FLAGS native-game/course_palette.c native-game/course_initialization.c native-game/course_screen.c native-game/game_services.c -o build/course-init-check/native_initialization.so
cc $FLAGS native-game/course_cursor.c native-game/game_services.c -o build/cursor-check/native_cursor_final.so
for variant in default private; do
 EXTRA=''
 if [ "$variant" = private ]; then
  EXTRA='-DNTM_SCREEN_PRIVATE_ASSETS native-renderer/amiga/intro_copy.c native-renderer/amiga/selector_memory.c'
 fi
 cc $FLAGS $EXTRA native-game/course_intro.c $COMMON -o "build/course-title-check/native_intro_$variant.so"
 cc $FLAGS $EXTRA native-game/course_title.c $COMMON -o "build/course-title-check/native_title_$variant.so"
 cc $FLAGS $EXTRA -I native-game reference/course_intro_application_bridge.c native-game/course_intro.c native-game/course_title.c $COMMON -o "build/course-title-check/native_title_application_$variant.so"
 export NTM_INTRO_LIBRARY="$ROOT/build/course-title-check/native_intro_$variant.so"
 export NTM_TITLE_LIBRARY="$ROOT/build/course-title-check/native_title_$variant.so"
 export NTM_TITLE_APPLICATION_LIBRARY="$ROOT/build/course-title-check/native_title_application_$variant.so"
 for suite in title title_application; do
  python3 "reference/compare_course_$suite.py" > "build/course-title-check/final-$suite-$variant.json" 2> "build/course-title-check/final-$suite-$variant.log"
  cat "build/course-title-check/final-$suite-$variant.json"
 done
done
python3 reference/compare_course_title_cursor.py > build/course-title-check/final-title-cursor.json 2> build/course-title-check/final-title-cursor.log
cat build/course-title-check/final-title-cursor.json
