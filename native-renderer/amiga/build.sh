#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/native-display"
EXTRA=()
case "${1:-}" in
    '') ;;
    --scene) OUT="$ROOT/build/native-scene-display"; EXTRA=(-DNR_SCENE) ;;
    *) echo 'usage: build.sh [--scene]' >&2; exit 2 ;;
esac
: "${AMIGA_BIN:?Set AMIGA_BIN to official vscode-amiga-debug/bin/linux tools}"
mkdir -p "$OUT"
FLAGS=(-Wall -Wextra -Werror -m68020 -msoft-float -Os -ffreestanding -fno-builtin -fno-pic -fno-stack-protector -fomit-frame-pointer -fno-asynchronous-unwind-tables)
m68k-linux-gnu-gcc "${FLAGS[@]}" "${EXTRA[@]}" -c "$ROOT/native-renderer/amiga/display.c" -o "$OUT/display.o"
m68k-linux-gnu-gcc "${FLAGS[@]}" -c "$ROOT/native-renderer/tile_renderer.c" -o "$OUT/renderer.o"
m68k-linux-gnu-as -m68020 "$ROOT/native-renderer/amiga/entry.s" -o "$OUT/entry.o"
m68k-linux-gnu-ld -Ttext=0 -e _start --emit-relocs "$OUT/entry.o" "$OUT/display.o" "$OUT/renderer.o" -o "$OUT/display.elf"
test -z "$(m68k-linux-gnu-nm -u "$OUT/display.elf")"
"$AMIGA_BIN/elf2hunk" "$OUT/display.elf" "$OUT/display.exe" -s
"$AMIGA_BIN/exe2adf" -i "$OUT/display.exe" -a "$OUT/display.adf" -l NGPC_DISPLAY
m68k-linux-gnu-size "$OUT/display.elf"
