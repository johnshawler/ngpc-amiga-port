#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Uses the existing isolated core; never rebuilds it or shared native libraries.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/cursor-edge-check
CC=${CC:-cc}
CROSS_CC=${CROSS_CC:-m68k-linux-gnu-gcc}
QEMU=${QEMU:-qemu-m68k}
FLAGS='-std=c99 -O2 -Wall -Wextra -Werror'
for name in course_cursor course_title_cursor round_record_cursor; do
 SOURCES="native-game/course_cursor.c native-game/game_services.c native-game/test_$name.c"
 $CC $FLAGS $SOURCES -o "build/cursor-edge-check/test-$name"
 "build/cursor-edge-check/test-$name"
 $CC $FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer $SOURCES -o "build/cursor-edge-check/test-$name-sanitize"
 ASAN_OPTIONS=detect_leaks=0 "build/cursor-edge-check/test-$name-sanitize"
 $CROSS_CC $FLAGS -m68020 -static $SOURCES -o "build/cursor-edge-check/test-$name-68020"
 $QEMU -cpu m68020 "build/cursor-edge-check/test-$name-68020"
done
$CC $FLAGS -shared -fPIC native-game/course_cursor.c native-game/game_services.c -o build/cursor-edge-check/native.so
export NTM_CURSOR_LIBRARY="$ROOT/build/cursor-edge-check/native.so"
for name in course_cursor course_cursor_queue course_title_cursor round_record_cursor course_cursor_edges; do
 python3 "reference/compare_$name.py" > "build/cursor-edge-check/$name.json" 2> "build/cursor-edge-check/$name.log"
 cat "build/cursor-edge-check/$name.json"
done
