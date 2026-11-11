#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/shot-golfer-features-check
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror'
SOURCES='native-game/shot_golfer_features.c native-game/shot_aim.c native-game/shot_math.c'
cc $FLAGS $SOURCES native-game/test_shot_golfer_features.c -o build/shot-golfer-features-check/test
build/shot-golfer-features-check/test
cc $FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer $SOURCES native-game/test_shot_golfer_features.c -o build/shot-golfer-features-check/test-sanitize
ASAN_OPTIONS=detect_leaks=0 build/shot-golfer-features-check/test-sanitize
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -msoft-float -ffreestanding -fno-builtin -c native-game/shot_golfer_features.c -o build/shot-golfer-features-check/golfer-features-68020.o
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -static $SOURCES native-game/test_shot_golfer_features.c -o build/shot-golfer-features-check/test-68020
${QEMU:-qemu-m68k} -cpu m68020 build/shot-golfer-features-check/test-68020
cc $FLAGS -shared -fPIC $SOURCES -o build/shot-golfer-features-check/native.so
export NTM_SHOT_GOLFER_FEATURES_LIBRARY="$ROOT/build/shot-golfer-features-check/native.so"
python3 reference/compare_shot_golfer_features.py > build/shot-golfer-features-check/oracle.json 2> build/shot-golfer-features-check/oracle.log
cat build/shot-golfer-features-check/oracle.json
