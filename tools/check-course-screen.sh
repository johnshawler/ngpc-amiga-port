#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build/course-screen-check
bash tools/build-reference.sh --screen-probe > build/course-screen-check/probe-build.log
cc -std=c99 -O2 -Wall -Wextra -Werror -shared -fPIC native-game/course_screen.c -o build/course-screen-check/native_screen.so
for suite in stamp pipeline producer; do
    python3 "reference/compare_course_screen_$suite.py" > "build/course-screen-check/final-$suite.json" 2> "build/course-screen-check/final-$suite.log"
    cat "build/course-screen-check/final-$suite.json"
done
