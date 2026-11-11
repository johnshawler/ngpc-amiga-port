#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
D=build/course-screen-reflection-check
mkdir -p "$D"
bash tools/build-reference.sh --screen-probe > "$D/probe-build.log"
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror'
for backend in default private; do
    EXTRA=''
    if [ "$backend" = private ]; then EXTRA=-DNTM_SCREEN_PRIVATE_ASSETS; fi
    S='native-game/course_screen.c native-game/test_course_screen_reflection.c'
    cc $FLAGS $EXTRA $S -o "$D/test-$backend"
    "$D/test-$backend"
    cc $FLAGS $EXTRA -fsanitize=address,undefined -fno-omit-frame-pointer $S -o "$D/sanitize-$backend"
    ASAN_OPTIONS=detect_leaks=0 "$D/sanitize-$backend"
    ${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS $EXTRA -m68020 -msoft-float -ffreestanding -fno-builtin -c native-game/course_screen.c -o "$D/screen-$backend-68020.o"
    ${CROSS_CC:-m68k-linux-gnu-gcc} $FLAGS $EXTRA -m68020 -static $S -o "$D/test-$backend-68020"
    ${QEMU:-qemu-m68k} -cpu m68020 "$D/test-$backend-68020"
    cc $FLAGS $EXTRA -shared -fPIC $S -o "$D/native-$backend.so"
    NTM_SCREEN_REFLECTION_LIBRARY="$ROOT/$D/native-$backend.so" python3 reference/compare_course_screen_reflection.py > "$D/oracle-$backend.json" 2> "$D/oracle-$backend.log"
    cat "$D/oracle-$backend.json"
done
# Existing normal stamping, producer and queue regression suites.
cc $FLAGS native-game/course_screen.c native-game/test_course_screen.c -o "$D/test-existing"
"$D/test-existing"
bash tools/check-course-screen.sh
