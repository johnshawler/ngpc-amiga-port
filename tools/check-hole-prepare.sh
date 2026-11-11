#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/hole-prepare-check
cc -std=c99 -O2 -Wall -Wextra -Werror -fPIC -shared reference/hole_prepare_math_bridge.c native-game/random_byte.c -o build/hole-prepare-check/native.so
python3 reference/compare_hole_prepare.py >build/hole-prepare-check/original.json
cat build/hole-prepare-check/original.json
