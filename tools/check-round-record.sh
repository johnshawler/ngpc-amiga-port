#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Uses the already-built isolated original oracle, never rebuilds the core.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/round-record-check build/course-init-check build/cursor-check
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror -shared -fPIC'
COMMON='native-game/input_state.c native-game/course_choice.c native-game/task_lifecycle.c native-game/task_allocation.c native-game/random_byte.c native-game/viewport.c native-game/game_services.c native-game/course_parent.c native-game/course_screen.c native-game/course_palette.c native-game/course_initialization.c native-game/course_cursor.c native-game/course_intro.c native-game/course_title.c native-game/round_record.c native-game/round_prepare.c'
cc $FLAGS native-game/course_palette.c -o build/course-init-check/native_palette.so
cc $FLAGS native-game/course_palette.c native-game/course_initialization.c native-game/course_screen.c native-game/game_services.c -o build/course-init-check/native_initialization.so
cc $FLAGS native-game/course_cursor.c native-game/game_services.c -o build/cursor-check/native_cursor_final.so
for variant in default private; do
 EXTRA=''
 if [ "$variant" = private ]; then EXTRA='-DNTM_SCREEN_PRIVATE_ASSETS native-renderer/amiga/intro_copy.c native-renderer/amiga/selector_memory.c'; fi
 cc $FLAGS $EXTRA $COMMON -o "build/round-record-check/native_round_$variant.so"
 cc $FLAGS $EXTRA -I native-game reference/course_intro_application_bridge.c $COMMON -o "build/round-record-check/native_round_application_$variant.so"
 export NTM_INTRO_LIBRARY="$ROOT/build/round-record-check/native_round_$variant.so"
 export NTM_ROUND_LIBRARY="$NTM_INTRO_LIBRARY"
 export NTM_ROUND_APPLICATION_LIBRARY="$ROOT/build/round-record-check/native_round_application_$variant.so"
 for suite in round_record round_record_application; do
  python3 "reference/compare_$suite.py" > "build/round-record-check/final-$suite-$variant.json" 2> "build/round-record-check/final-$suite-$variant.log"
  cat "build/round-record-check/final-$suite-$variant.json"
 done
done
python3 reference/compare_round_record_cursor.py > build/round-record-check/final-cursor.json 2> build/round-record-check/final-cursor.log
cat build/round-record-check/final-cursor.json
