#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/shot-club-control-check
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror -shared -fPIC'
SOURCES='native-game/shot_club.c native-game/shot_aim.c native-game/shot_scene_objects.c native-game/tee_objects.c native-game/game_services.c native-game/course_screen.c'
for variant in ${NTM_SHOT_CLUB_VARIANTS:-default private}; do
 EXTRA=''
 if [ "$variant" = private ]; then EXTRA='-DNTM_SCREEN_PRIVATE_ASSETS native-renderer/amiga/intro_copy.c native-renderer/amiga/selector_memory.c'; fi
 cc $FLAGS $EXTRA $SOURCES -o "build/shot-club-control-check/native_$variant.so"
 export NTM_SHOT_CLUB_LIBRARY="$ROOT/build/shot-club-control-check/native_$variant.so"
 for suite in shot_club shot_club_control; do
  python3 "reference/compare_$suite.py" > "build/shot-club-control-check/$suite-$variant.json" 2> "build/shot-club-control-check/$suite-$variant.log"
  cat "build/shot-club-control-check/$suite-$variant.json"
 done
done
