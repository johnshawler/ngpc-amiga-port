#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/native-control-tests";mkdir -p "$OUT"
FLAGS=(-std=c99 -O2 -Wall -Wextra -Werror)
for test in aim_unpack_test aim_unpack_file_test aim_frontend_test; do
 SOURCES=("$ROOT/native-renderer/amiga/aim_unpack.c" "$ROOT/native-renderer/amiga/$test.c")
 if [ "$test" = aim_frontend_test ]; then SOURCES=("$ROOT/native-renderer/amiga/aim_frontend.c" "$ROOT/native-renderer/amiga/$test.c"); fi
 cc "${FLAGS[@]}" "${SOURCES[@]}" -o "$OUT/$test"
 cc "${FLAGS[@]}" -fsanitize=address,undefined -fno-omit-frame-pointer "${SOURCES[@]}" -o "$OUT/$test-san"
 m68k-linux-gnu-gcc "${FLAGS[@]}" -m68020 -static "${SOURCES[@]}" -o "$OUT/$test-m68020"
done
"$OUT/aim_frontend_test"
ASAN_OPTIONS=detect_leaks=0 "$OUT/aim_frontend_test-san"
qemu-m68k -cpu m68020 "$OUT/aim_frontend_test-m68020"
"$OUT/aim_unpack_test"
ASAN_OPTIONS=detect_leaks=0 "$OUT/aim_unpack_test-san"
qemu-m68k -cpu m68020 "$OUT/aim_unpack_test-m68020"
for name in aim_assets intro_packed; do
 SOURCE="${CONTROL_BUILD_ROOT:-$ROOT/build/native-control}/$name"
 "$OUT/aim_unpack_file_test" "$SOURCE.lz" "$SOURCE.raw"
 ASAN_OPTIONS=detect_leaks=0 "$OUT/aim_unpack_file_test-san" "$SOURCE.lz" "$SOURCE.raw"
 qemu-m68k -cpu m68020 "$OUT/aim_unpack_file_test-m68020" "$SOURCE.lz" "$SOURCE.raw"
done
python3 -m unittest discover -s "$ROOT/tests" -p test_aim_pack.py
python3 "$ROOT/native-renderer/amiga/shot_disk.py" "${CONTROL_BUILD_ROOT:-$ROOT/build/native-control}/control.adf" "${CONTROL_BUILD_ROOT:-$ROOT/build/native-control}/control.exe"
python3 "$ROOT/tools/verify-adf-file.py" --adf "${CONTROL_BUILD_ROOT:-$ROOT/build/native-control}/control.adf" --file control.exe --source "${CONTROL_BUILD_ROOT:-$ROOT/build/native-control}/control.exe"
