#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Test-only host adapter for exact original-input replay, never target runtime.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd);cd "$ROOT"
OUT="${CONTROL_BUILD_ROOT:-$ROOT/build/native-control}"
mkdir -p "$OUT" build/course-init-check build/cursor-check build/tee-object-check
function compile() {
 local output="$1";shift
 cc "$@" -o "$output.new.$$"
 mv "$output.new.$$" "$output"
}
FLAGS=(-std=c99 -O2 -Wall -Wextra -Werror -shared -fPIC)
compile build/course-init-check/native_palette.so "${FLAGS[@]}" native-game/course_palette.c
compile build/course-init-check/native_initialization.so "${FLAGS[@]}" native-game/course_palette.c native-game/course_initialization.c native-game/course_screen.c native-game/game_services.c
compile build/cursor-check/native_cursor_final.so "${FLAGS[@]}" native-game/course_cursor.c native-game/game_services.c
compile build/tee-object-check/native.so "${FLAGS[@]}" native-game/tee_objects.c native-game/game_services.c
SOURCES=(reference/shot_control_application_bridge.c native-renderer/amiga/intro_copy.c native-renderer/amiga/selector_memory.c)
for name in input_state course_choice task_lifecycle task_allocation random_byte viewport game_services course_parent course_screen course_palette course_initialization course_cursor course_intro course_title round_record round_prepare tee_graphics tee_setup tee_objects hole_prepare shot_math shot_prepare shot_scene shot_banner shot_raster shot_palette shot_terrain shot_aim shot_aim_setup shot_hud aim_world shot_scene_objects shot_club shot_markers shot_camera shot_ready; do SOURCES+=("native-game/$name.c"); done
compile "$OUT/oracle.so" "${FLAGS[@]}" -DNTM_SCREEN_PRIVATE_ASSETS -I native-game "${SOURCES[@]}"
