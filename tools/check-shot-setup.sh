#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Private input oracle, reusing the existing original core without rebuilding it.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/shot-check build/course-init-check build/cursor-check build/tee-object-check
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror -shared -fPIC'
COMMON='native-game/input_state.c native-game/course_choice.c native-game/task_lifecycle.c native-game/task_allocation.c native-game/random_byte.c native-game/viewport.c native-game/game_services.c native-game/course_parent.c native-game/course_screen.c native-game/course_palette.c native-game/course_initialization.c native-game/course_cursor.c native-game/course_intro.c native-game/course_title.c native-game/round_record.c native-game/round_prepare.c native-game/tee_graphics.c native-game/tee_setup.c native-game/tee_objects.c native-game/hole_prepare.c'
SHOT='native-game/shot_math.c native-game/shot_prepare.c native-game/shot_scene.c native-game/shot_banner.c native-game/shot_raster.c native-game/shot_palette.c native-game/shot_terrain.c'
cc $FLAGS native-game/course_palette.c -o build/course-init-check/native_palette.so
cc $FLAGS native-game/course_palette.c native-game/course_initialization.c native-game/course_screen.c native-game/game_services.c -o build/course-init-check/native_initialization.so
cc $FLAGS native-game/course_cursor.c native-game/game_services.c -o build/cursor-check/native_cursor_final.so
cc $FLAGS native-game/tee_objects.c native-game/game_services.c -o build/tee-object-check/native.so
cc $FLAGS native-game/shot_math.c -o build/shot-check/native_math.so
cc $FLAGS native-game/shot_math.c native-game/shot_prepare.c native-game/random_byte.c -o build/shot-check/native_prepare.so
export NTM_SHOT_LIBRARY="$ROOT/build/shot-check/native_math.so"
export NTM_SHOT_PREPARE_LIBRARY="$ROOT/build/shot-check/native_prepare.so"
for suite in shot_math shot_prepare; do
 python3 "reference/compare_$suite.py" > "build/shot-check/final-$suite.json" 2> "build/shot-check/final-$suite.log"
 cat "build/shot-check/final-$suite.json"
done
for variant in ${NTM_SHOT_VARIANTS:-default private}; do
 EXTRA=''
 if [ "$variant" = private ]; then EXTRA='-DNTM_SCREEN_PRIVATE_ASSETS native-renderer/amiga/intro_copy.c native-renderer/amiga/selector_memory.c'; fi
 cc $FLAGS $EXTRA $COMMON $SHOT -o "build/shot-check/native_banner_$variant.so"
 cc $FLAGS $EXTRA -I native-game reference/shot_application_bridge.c $COMMON $SHOT -o "build/shot-check/native_application_$variant.so"
 export NTM_SHOT_BANNER_LIBRARY="$ROOT/build/shot-check/native_banner_$variant.so"
 export NTM_INTRO_LIBRARY="$NTM_SHOT_BANNER_LIBRARY"
 export NTM_SHOT_APPLICATION_LIBRARY="$ROOT/build/shot-check/native_application_$variant.so"
 for suite in shot_banner shot_application; do
  python3 "reference/compare_$suite.py" > "build/shot-check/final-$suite-$variant.json" 2> "build/shot-check/final-$suite-$variant.log"
  cat "build/shot-check/final-$suite-$variant.json"
 done
done
