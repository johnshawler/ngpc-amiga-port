#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/result-application-check
FLAGS='-I native-game -std=c99 -O2 -Wall -Wextra -Werror'
COMMON='native-game/input_state.c native-game/course_choice.c native-game/task_lifecycle.c native-game/task_allocation.c native-game/random_byte.c native-game/viewport.c native-game/game_services.c native-game/course_parent.c native-game/course_screen.c native-game/course_palette.c native-game/course_initialization.c native-game/course_cursor.c native-game/course_intro.c native-game/course_title.c native-game/round_record.c native-game/round_prepare.c native-game/tee_graphics.c native-game/tee_setup.c native-game/tee_objects.c native-game/hole_prepare.c'
COMMON=$(printf '%s' "$COMMON" | sed "s|native-game/tee_objects.c|${NTM_SWING_OBJECT_SOURCE:-native-game/tee_objects.c}|")
SHOT='native-game/shot_math.c native-game/shot_prepare.c native-game/shot_scene.c native-game/shot_banner.c native-game/shot_raster.c native-game/shot_palette.c native-game/shot_terrain.c'
AIM='native-game/shot_aim.c native-game/shot_aim_setup.c native-game/shot_hud.c native-game/aim_world.c'
OPEN='native-game/shot_scene_objects.c native-game/shot_club.c native-game/shot_markers.c native-game/shot_ready.c native-game/shot_camera.c'
FIRE='native-game/shot_fire.c native-game/shot_spin.c native-game/shot_trajectory.c native-game/shot_golfer.c native-game/shot_projection.c native-game/shot_golfer_features.c native-game/shot_golfer_animation.c native-game/shot_golfer_drawing.c'
POWER='native-game/shot_perfect.c native-game/shot_swing_exit.c native-game/shot_miss.c native-game/shot_swing.c native-game/shot_trajectory_swing.c native-game/shot_power.c native-game/shot_power_meter.c native-game/shot_accuracy.c'
FIXTURE='native-game/test_shot_result_application.c native-game/shot_control_application.c'
cc $FLAGS $FIXTURE $COMMON $SHOT $AIM $OPEN $FIRE $POWER -o build/result-application-check/test
build/result-application-check/test
cc $FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer $FIXTURE $COMMON $SHOT $AIM $OPEN $FIRE $POWER -o build/result-application-check/test-sanitize
ASAN_OPTIONS=detect_leaks=0 build/result-application-check/test-sanitize
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -static $FIXTURE $COMMON $SHOT $AIM $OPEN $FIRE $POWER -o build/result-application-check/test-68020
${QEMU:-qemu-m68k} -cpu m68020 build/result-application-check/test-68020
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -msoft-float -ffreestanding -fno-builtin -c native-game/shot_result_application.c -o build/result-application-check/result-application-68020.o
if [ "${NTM_RESULT_SKIP_ORACLE:-0}" = 1 ]; then exit 0; fi
for variant in ${NTM_SHOT_VARIANTS:-default private}; do
 EXTRA=''
 if [ "$variant" = private ]; then EXTRA='-DNTM_SCREEN_PRIVATE_ASSETS native-renderer/amiga/intro_copy.c native-renderer/amiga/selector_memory.c'; fi
 cc $FLAGS -shared -fPIC $EXTRA -I native-game reference/shot_result_application_bridge.c $COMMON $SHOT $AIM $OPEN $FIRE $POWER -o "build/result-application-check/application_$variant.so"
 export NTM_SHOT_APPLICATION_LIBRARY="$ROOT/build/result-application-check/application_$variant.so"
 python3 reference/compare_shot_result_application.py > "build/result-application-check/application-$variant.json" 2> "build/result-application-check/application-$variant.log"
 cat "build/result-application-check/application-$variant.json"
 NTM_SWING_START=1 NTM_POWER_FLOWS=2 NTM_SWING_AIM_HIT=1 python3 reference/compare_shot_result_application.py > "build/result-application-check/application-hit-$variant.json" 2> "build/result-application-check/application-hit-$variant.log"
 cat "build/result-application-check/application-hit-$variant.json"
 NTM_SWING_START=1 NTM_POWER_FLOWS=2 NTM_RESULT_PERFECT_INPUT=1 python3 reference/compare_shot_result_application.py > "build/result-application-check/perfect-$variant.json" 2> "build/result-application-check/perfect-$variant.log"
 cat "build/result-application-check/perfect-$variant.json"
done
