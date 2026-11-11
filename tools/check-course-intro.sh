#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/course-intro-check build/course-init-check build/cursor-check
# Separate diagnostic export; pinned source and cartridge stay unchanged.
# Do not build concurrently with reference runs, as for other oracle scripts.
bash tools/build-reference.sh --screen-probe > build/course-intro-check/probe-build.log
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror -shared -fPIC'
COMMON='native-game/input_state.c native-game/course_choice.c native-game/task_lifecycle.c native-game/task_allocation.c native-game/random_byte.c native-game/viewport.c native-game/game_services.c native-game/course_parent.c native-game/course_screen.c native-game/course_palette.c native-game/course_initialization.c native-game/course_cursor.c'
# Existing shared fixture loaders use these component libraries.
cc $FLAGS native-game/course_palette.c -o build/course-init-check/native_palette.so
cc $FLAGS native-game/course_palette.c native-game/course_initialization.c native-game/course_screen.c native-game/game_services.c -o build/course-init-check/native_initialization.so
cc $FLAGS native-game/course_cursor.c native-game/game_services.c -o build/cursor-check/native_cursor_final.so
cc $FLAGS native-game/course_intro.c native-game/course_palette.c native-game/course_screen.c native-game/game_services.c native-game/task_allocation.c native-game/task_lifecycle.c native-game/course_choice.c native-game/input_state.c -o build/course-intro-check/native_intro.so
cc $FLAGS -I native-game reference/course_intro_application_bridge.c native-game/course_intro.c $COMMON -o build/course-intro-check/native_intro_application.so
for variant in default private; do
 if [ "$variant" = private ]; then
  EXTRA='-DNTM_SCREEN_PRIVATE_ASSETS native-renderer/amiga/intro_copy.c native-renderer/amiga/selector_memory.c'
  cc $FLAGS $EXTRA native-game/course_intro.c native-game/course_palette.c native-game/course_screen.c native-game/game_services.c native-game/task_allocation.c native-game/task_lifecycle.c native-game/course_choice.c native-game/input_state.c -o build/course-intro-check/native_intro_private.so
  cc $FLAGS $EXTRA -I native-game reference/course_intro_application_bridge.c native-game/course_intro.c $COMMON -o build/course-intro-check/native_intro_application_private.so
  export NTM_INTRO_LIBRARY="$ROOT/build/course-intro-check/native_intro_private.so"
  export NTM_INTRO_APPLICATION_LIBRARY="$ROOT/build/course-intro-check/native_intro_application_private.so"
 else
  export NTM_INTRO_LIBRARY="$ROOT/build/course-intro-check/native_intro.so"
  export NTM_INTRO_APPLICATION_LIBRARY="$ROOT/build/course-intro-check/native_intro_application.so"
 fi
 for suite in intro intro_application; do
  python3 "reference/compare_course_$suite.py" > "build/course-intro-check/final-$suite-$variant.json" 2> "build/course-intro-check/final-$suite-$variant.log"
  cat "build/course-intro-check/final-$suite-$variant.json"
 done
done
