#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD="${FIRE_BUILD_ROOT:-$ROOT/build/native-fire}"
OUT="$ROOT/build/native-fire-tests";mkdir -p "$OUT"
FLAGS=(-std=c99 -O2 -Wall -Wextra -Werror)
for test in stream boot; do
 SOURCES=("$ROOT/native-renderer/amiga/fire_stream.c" "$ROOT/native-renderer/amiga/fire_${test}_test.c")
 if [ "$test" = boot ]; then SOURCES+=("$ROOT/native-renderer/amiga/fire_boot.c"); else SOURCES+=("$ROOT/native-renderer/amiga/aim_unpack.c"); fi
 cc "${FLAGS[@]}" "${SOURCES[@]}" -o "$OUT/$test"
 cc "${FLAGS[@]}" -fsanitize=address,undefined -fno-omit-frame-pointer "${SOURCES[@]}" -o "$OUT/$test-san"
 m68k-linux-gnu-gcc "${FLAGS[@]}" -m68020 -static "${SOURCES[@]}" -o "$OUT/$test-m68020"
 "$OUT/$test"
 ASAN_OPTIONS=detect_leaks=0 "$OUT/$test-san"
 qemu-m68k -cpu m68020 "$OUT/$test-m68020"
done
for name in fire intro; do
 "$OUT/stream" "$BUILD/$name.pool" "$BUILD/$name.raw"
 ASAN_OPTIONS=detect_leaks=0 "$OUT/stream-san" "$BUILD/$name.pool" "$BUILD/$name.raw"
 qemu-m68k -cpu m68020 "$OUT/stream-m68020" "$BUILD/$name.pool" "$BUILD/$name.raw"
done
python3 "$ROOT/reference/check_fire_target_assets.py" --directory "$BUILD"
