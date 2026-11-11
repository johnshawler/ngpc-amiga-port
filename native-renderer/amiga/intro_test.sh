#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/native-intro"
mkdir -p "$OUT"
SOURCES=()
for name in input_state course_choice task_lifecycle task_allocation random_byte viewport game_services course_parent course_screen course_driver course_palette course_initialization course_cursor course_application course_intro course_title course_intro_application; do SOURCES+=("$ROOT/native-game/$name.c"); done
SOURCES+=("$ROOT/native-renderer/scene_renderer.c" "$ROOT/native-renderer/tile_renderer.c")
FLAGS=(-std=c99 -O2 -Wall -Wextra -Werror)
for test in test_course_intro_application test_course_title_application; do
TEST_SOURCE="$ROOT/native-game/$test.c"

cc "${FLAGS[@]}" "${SOURCES[@]}" "$TEST_SOURCE" -o "$OUT/test-application"
"$OUT/test-application"
cc "${FLAGS[@]}" -fsanitize=address,undefined -fno-omit-frame-pointer "${SOURCES[@]}" "$TEST_SOURCE" -o "$OUT/test-application-sanitize"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test-application-sanitize"
m68k-linux-gnu-gcc "${FLAGS[@]}" -m68020 -static "${SOURCES[@]}" "$TEST_SOURCE" -o "$OUT/test-application-68020"
qemu-m68k -cpu m68020 "$OUT/test-application-68020"

# Exercise the opt-in target copy branch using the same non-overlapping contract.
PRIVATE=("$ROOT/native-renderer/amiga/intro_copy.c" "$ROOT/native-renderer/amiga/selector_memory.c")
cc "${FLAGS[@]}" -DNTM_SCREEN_PRIVATE_ASSETS "${PRIVATE[@]}" "${SOURCES[@]}" "$TEST_SOURCE" -o "$OUT/test-private-copy"
"$OUT/test-private-copy"
cc "${FLAGS[@]}" -DNTM_SCREEN_PRIVATE_ASSETS "${PRIVATE[@]}" -fsanitize=address,undefined -fno-omit-frame-pointer "${SOURCES[@]}" "$TEST_SOURCE" -o "$OUT/test-private-copy-sanitize"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test-private-copy-sanitize"
m68k-linux-gnu-gcc "${FLAGS[@]}" -m68020 -static -DNTM_SCREEN_PRIVATE_ASSETS -DSELECTOR_FAST_MEMORY "${PRIVATE[@]}" "${SOURCES[@]}" "$TEST_SOURCE" "$ROOT/native-renderer/amiga/selector_memory.s" -o "$OUT/test-private-copy-68020"
qemu-m68k -cpu m68020 "$OUT/test-private-copy-68020"
done
