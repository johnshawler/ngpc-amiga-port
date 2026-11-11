#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/native-tee-tests"
mkdir -p "$OUT"
FLAGS=(-std=c99 -O2 -Wall -Wextra -Werror)
SOURCES=("$ROOT/native-renderer/amiga/tee_planar_cache.c" "$ROOT/native-renderer/amiga/selector_memory.c")
for test in tee_cache_test tee_cache_private_test; do
  EXTRA=(); if [[ $test == tee_cache_private_test ]]; then EXTRA=(-I"$ROOT/build/native-tee"); fi
  cc "${FLAGS[@]}" "${EXTRA[@]}" "${SOURCES[@]}" "$ROOT/native-renderer/amiga/$test.c" -o "$OUT/$test"
  "$OUT/$test"
  cc "${FLAGS[@]}" "${EXTRA[@]}" -fsanitize=address,undefined -fno-omit-frame-pointer "${SOURCES[@]}" "$ROOT/native-renderer/amiga/$test.c" -o "$OUT/$test-sanitize"
  ASAN_OPTIONS=detect_leaks=0 "$OUT/$test-sanitize"
  m68k-linux-gnu-gcc "${FLAGS[@]}" "${EXTRA[@]}" -m68020 -static -DSELECTOR_FAST_MEMORY "${SOURCES[@]}" "$ROOT/native-renderer/amiga/selector_memory.s" "$ROOT/native-renderer/amiga/$test.c" -o "$OUT/$test-m68020"
  qemu-m68k -cpu m68020 "$OUT/$test-m68020"
done
