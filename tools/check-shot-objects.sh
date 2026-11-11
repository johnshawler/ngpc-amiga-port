#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/shot-probe
CC=${CC:-cc}
$CC -std=c99 -O2 -Wall -Wextra -Werror -shared -fPIC native-game/tee_objects.c native-game/game_services.c -o build/shot-probe/objects.so
NTM_TEE_OBJECT_LIBRARY="$ROOT/build/shot-probe/objects.so" python3 reference/compare_shot_objects.py
