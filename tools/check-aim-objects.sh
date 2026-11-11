#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/aim-object-check
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -shared -fPIC native-game/tee_objects.c native-game/game_services.c -o build/aim-object-check/native.so
NTM_TEE_OBJECT_LIBRARY="$ROOT/build/aim-object-check/native.so" python3 reference/compare_aim_objects.py
