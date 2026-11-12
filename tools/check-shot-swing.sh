#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
OUT=build/shot-swing-check;mkdir -p "$OUT"
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror'
cc $FLAGS native-game/shot_swing.c native-game/test_shot_swing.c -o "$OUT/test"
"$OUT/test"
cc $FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer native-game/shot_swing.c native-game/test_shot_swing.c -o "$OUT/test-sanitize"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test-sanitize"
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -msoft-float -ffreestanding -fno-builtin -c native-game/shot_swing.c -o "$OUT/swing-68020.o"
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -static native-game/shot_swing.c native-game/test_shot_swing.c -o "$OUT/test-68020"
${QEMU:-qemu-m68k} -cpu m68020 "$OUT/test-68020"
cc $FLAGS -shared -fPIC native-game/shot_swing.c -o "$OUT/native.so"
NTM_SHOT_SWING_LIBRARY="$ROOT/$OUT/native.so" python3 reference/compare_shot_swing.py >"$OUT/oracle.json" 2>"$OUT/oracle.log"
cat "$OUT/oracle.json"
