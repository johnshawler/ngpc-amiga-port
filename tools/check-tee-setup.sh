#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Reuses the existing isolated original core; never rebuilds a running core.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/tee-check build/course-init-check build/cursor-check build/tee-object-check
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror -shared -fPIC'
COMMON='native-game/input_state.c native-game/course_choice.c native-game/task_lifecycle.c native-game/task_allocation.c native-game/random_byte.c native-game/viewport.c native-game/game_services.c native-game/course_parent.c native-game/course_screen.c native-game/course_palette.c native-game/course_initialization.c native-game/course_cursor.c native-game/course_intro.c native-game/course_title.c native-game/round_record.c native-game/round_prepare.c native-game/tee_graphics.c native-game/tee_setup.c native-game/tee_objects.c native-game/hole_prepare.c'
cc $FLAGS native-game/course_palette.c -o build/course-init-check/native_palette.so
cc $FLAGS native-game/course_palette.c native-game/course_initialization.c native-game/course_screen.c native-game/game_services.c -o build/course-init-check/native_initialization.so
cc $FLAGS native-game/course_cursor.c native-game/game_services.c -o build/cursor-check/native_cursor_final.so
cc $FLAGS native-game/tee_objects.c native-game/game_services.c -o build/tee-object-check/native.so
for variant in default private; do
 EXTRA=''
 if [ "$variant" = private ]; then EXTRA='-DNTM_SCREEN_PRIVATE_ASSETS native-renderer/amiga/intro_copy.c native-renderer/amiga/selector_memory.c'; fi
 cc $FLAGS $EXTRA $COMMON -o "build/tee-check/native_tee_$variant.so"
 cc $FLAGS $EXTRA -I native-game reference/tee_setup_application_bridge.c $COMMON -o "build/tee-check/native_application_$variant.so"
 export NTM_TEE_LIBRARY="$ROOT/build/tee-check/native_tee_$variant.so"
 export NTM_INTRO_LIBRARY="$NTM_TEE_LIBRARY"
 export NTM_TEE_APPLICATION_LIBRARY="$ROOT/build/tee-check/native_application_$variant.so"
 for suite in tee_setup tee_setup_application; do
  python3 "reference/compare_$suite.py" > "build/tee-check/final-$suite-$variant.json" 2> "build/tee-check/final-$suite-$variant.log"
  cat "build/tee-check/final-$suite-$variant.json"
 done
done
