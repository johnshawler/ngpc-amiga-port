#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
OUT=${NTM_POWER_CHECK_DIR:-build/shot-power-cancel-check}
mkdir -p "$OUT"
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror'
SOURCES='native-game/shot_power.c native-game/shot_aim.c native-game/task_allocation.c native-game/task_lifecycle.c native-game/course_choice.c native-game/input_state.c'
cc $FLAGS $SOURCES native-game/test_shot_power.c -o "$OUT/test"
"$OUT/test"
cc $FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer $SOURCES native-game/test_shot_power.c -o "$OUT/test-sanitize"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test-sanitize"
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -msoft-float -ffreestanding -fno-builtin -c native-game/shot_power.c -o "$OUT/power-68020.o"
${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS -m68020 -static $SOURCES native-game/test_shot_power.c -o "$OUT/test-68020"
${QEMU:-qemu-m68k} -cpu m68020 "$OUT/test-68020"
cc $FLAGS -shared -fPIC $SOURCES -o "$OUT/native.so"
export NTM_SHOT_POWER_LIBRARY="$ROOT/$OUT/native.so"
python3 reference/compare_shot_power.py > "$OUT/original.json" 2> "$OUT/original.log"
cat "$OUT/original.json"
