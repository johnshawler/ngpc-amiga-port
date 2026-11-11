#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/tee-object-check
CC=${CC:-cc}
CROSS_CC=${CROSS_CC:-m68k-linux-gnu-gcc}
QEMU=${QEMU:-qemu-m68k}
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror'
SOURCES='native-game/tee_objects.c native-game/game_services.c native-game/test_tee_objects.c'
$CC $FLAGS $SOURCES -o build/tee-object-check/test
build/tee-object-check/test
$CC $FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer $SOURCES -o build/tee-object-check/test-sanitize
ASAN_OPTIONS=detect_leaks=0 build/tee-object-check/test-sanitize
$CROSS_CC $FLAGS -m68020 -msoft-float -ffreestanding -fno-builtin -c native-game/tee_objects.c -o build/tee-object-check/tee-objects-68020.o
$CROSS_CC $FLAGS -m68020 -static $SOURCES -o build/tee-object-check/test-68020
$QEMU -cpu m68020 build/tee-object-check/test-68020
$CC $FLAGS -shared -fPIC native-game/tee_objects.c native-game/game_services.c -o build/tee-object-check/native.so
python3 reference/compare_tee_objects.py > build/tee-object-check/oracle.json 2> build/tee-object-check/oracle.log
cat build/tee-object-check/oracle.json
