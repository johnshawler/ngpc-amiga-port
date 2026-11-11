#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Private original comparison; original core and caller cartridge remain local.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/shot-aim-check build/course-init-check build/cursor-check build/tee-object-check
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror -shared -fPIC'
COMMON='native-game/input_state.c native-game/course_choice.c native-game/task_lifecycle.c native-game/task_allocation.c native-game/random_byte.c native-game/viewport.c native-game/game_services.c native-game/course_parent.c native-game/course_screen.c native-game/course_palette.c native-game/course_initialization.c native-game/course_cursor.c native-game/course_intro.c native-game/course_title.c native-game/round_record.c native-game/round_prepare.c native-game/tee_graphics.c native-game/tee_setup.c native-game/tee_objects.c native-game/hole_prepare.c'
SHOT='native-game/shot_math.c native-game/shot_prepare.c native-game/shot_scene.c native-game/shot_banner.c native-game/shot_raster.c native-game/shot_palette.c native-game/shot_terrain.c'
AIM='native-game/shot_aim.c native-game/shot_aim_setup.c native-game/shot_hud.c native-game/aim_world.c'
cc $FLAGS native-game/course_palette.c -o build/course-init-check/native_palette.so
cc $FLAGS native-game/course_palette.c native-game/course_initialization.c native-game/course_screen.c native-game/game_services.c -o build/course-init-check/native_initialization.so
cc $FLAGS native-game/course_cursor.c native-game/game_services.c -o build/cursor-check/native_cursor_final.so
cc $FLAGS native-game/tee_objects.c native-game/game_services.c -o build/tee-object-check/native.so
cc $FLAGS native-game/shot_aim.c -o build/shot-aim-check/helpers.so
export NTM_SHOT_AIM_LIBRARY="$ROOT/build/shot-aim-check/helpers.so"
python3 reference/compare_shot_aim.py > build/shot-aim-check/helpers.json 2> build/shot-aim-check/helpers.log
cat build/shot-aim-check/helpers.json
for variant in ${NTM_SHOT_VARIANTS:-default private}; do
 EXTRA=''
 if [ "$variant" = private ]; then EXTRA='-DNTM_SCREEN_PRIVATE_ASSETS native-renderer/amiga/intro_copy.c native-renderer/amiga/selector_memory.c'; fi
 cc $FLAGS $EXTRA $COMMON $SHOT $AIM -o "build/shot-aim-check/setup_$variant.so"
 cc $FLAGS $EXTRA -I native-game reference/shot_aim_application_bridge.c $COMMON $SHOT $AIM -o "build/shot-aim-check/application_$variant.so"
 export NTM_INTRO_LIBRARY="$ROOT/build/shot-aim-check/setup_$variant.so"
 export NTM_SHOT_AIM_SETUP_LIBRARY="$ROOT/build/shot-aim-check/setup_$variant.so"
 export NTM_SHOT_APPLICATION_LIBRARY="$ROOT/build/shot-aim-check/application_$variant.so"
 for suite in shot_aim_setup shot_aim_application; do
  python3 "reference/compare_$suite.py" > "build/shot-aim-check/$suite-$variant.json" 2> "build/shot-aim-check/$suite-$variant.log"
  cat "build/shot-aim-check/$suite-$variant.json"
 done
done
