#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/shot-probe
CC=${CC:-cc}; CROSS_CC=${CROSS_CC:-m68k-linux-gnu-gcc}; QEMU=${QEMU:-qemu-m68k}
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror'
SOURCES='native-game/aim_world.c native-game/test_aim_world.c'
$CC $FLAGS $SOURCES -o build/shot-probe/test-aim-world
build/shot-probe/test-aim-world
$CC $FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer $SOURCES -o build/shot-probe/test-aim-world-san
ASAN_OPTIONS=detect_leaks=0 build/shot-probe/test-aim-world-san
$CROSS_CC $FLAGS -m68020 -static $SOURCES -o build/shot-probe/test-aim-world-m68k
$QEMU -cpu m68020 build/shot-probe/test-aim-world-m68k
$CC $FLAGS -shared -fPIC native-game/aim_world.c -o build/shot-probe/aim-world.so
python3 reference/compare_aim_world.py
