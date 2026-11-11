#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/shot-control-check build/shot-scene-check
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror -shared -fPIC'
COMMON='native-game/input_state.c native-game/course_choice.c native-game/task_lifecycle.c native-game/task_allocation.c native-game/random_byte.c native-game/viewport.c native-game/game_services.c native-game/course_parent.c native-game/course_screen.c native-game/course_palette.c native-game/course_initialization.c native-game/course_cursor.c native-game/course_intro.c native-game/course_title.c native-game/round_record.c native-game/round_prepare.c native-game/tee_graphics.c native-game/tee_setup.c native-game/tee_objects.c native-game/hole_prepare.c'
SHOT='native-game/shot_math.c native-game/shot_prepare.c native-game/shot_scene.c native-game/shot_banner.c native-game/shot_raster.c native-game/shot_palette.c native-game/shot_terrain.c'
AIM='native-game/shot_aim.c native-game/shot_aim_setup.c native-game/shot_hud.c native-game/aim_world.c'
OPEN='native-game/shot_scene_objects.c native-game/shot_club.c native-game/shot_markers.c native-game/shot_ready.c native-game/shot_camera.c'
cc $FLAGS $COMMON $SHOT $AIM $OPEN -o build/shot-scene-check/native.so
for variant in ${NTM_SHOT_VARIANTS:-default private}; do
 EXTRA=''
 if [ "$variant" = private ]; then EXTRA='-DNTM_SCREEN_PRIVATE_ASSETS native-renderer/amiga/intro_copy.c native-renderer/amiga/selector_memory.c'; fi
 cc $FLAGS $EXTRA -I native-game reference/shot_control_application_bridge.c $COMMON $SHOT $AIM $OPEN -o "build/shot-control-check/application_$variant.so"
 export NTM_SHOT_APPLICATION_LIBRARY="$ROOT/build/shot-control-check/application_$variant.so"
 python3 reference/compare_shot_control_application.py > "build/shot-control-check/application-$variant.json" 2> "build/shot-control-check/application-$variant.log"
 cat "build/shot-control-check/application-$variant.json"
done
