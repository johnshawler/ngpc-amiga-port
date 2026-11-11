#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/native-selector"
EXTRA=(-DSELECTOR_PLANAR_CACHE)
case "${1:-}" in
    '') ;;
    --uncached) OUT="$ROOT/build/native-selector-uncached"; EXTRA=() ;;
    *) echo 'usage: selector_build.sh [--uncached]' >&2; exit 2 ;;
esac
: "${AMIGA_BIN:?Set AMIGA_BIN to official vscode-amiga-debug/bin/linux tools}"
mkdir -p "$OUT"
python3 "$ROOT/tools/export-course-selector-assets.py" --output "$OUT/selector_assets.inc"
FLAGS=(-std=c99 -Wall -Wextra -Werror -m68020 -msoft-float -Os -ffreestanding -fno-builtin -fno-pic -fno-stack-protector -fomit-frame-pointer -fno-asynchronous-unwind-tables)
SOURCES=(native-renderer/amiga/selector_main.c native-renderer/amiga/selector_frontend.c native-renderer/amiga/selector_planar_cache.c native-renderer/amiga/selector_memory.c native-renderer/scene_renderer.c native-renderer/tile_renderer.c)
for name in input_state course_choice task_lifecycle task_allocation random_byte viewport game_services course_parent course_screen course_driver course_palette course_initialization course_cursor course_application; do SOURCES+=("native-game/$name.c"); done
if [ ${#EXTRA[@]} -ne 0 ]; then
    GENERATOR=("$ROOT/native-renderer/amiga/selector_generate_cache.c")
    for source in "${SOURCES[@]:1}"; do GENERATOR+=("$ROOT/$source"); done
    cc -std=c99 -O2 -Wall -Wextra -Werror -I"$OUT" "${GENERATOR[@]}" -o "$OUT/generate-cache"
    "$OUT/generate-cache" "$OUT/selector_planar.inc"
fi
OBJECTS=()
for source in "${SOURCES[@]}"; do
    object="$OUT/$(basename "${source%.c}").o"
    m68k-linux-gnu-gcc "${FLAGS[@]}" "${EXTRA[@]}" -DSELECTOR_FAST_MEMORY -I"$OUT" -c "$ROOT/$source" -o "$object"
    OBJECTS+=("$object")
done
m68k-linux-gnu-as -m68020 "$ROOT/native-renderer/amiga/selector_memory.s" -o "$OUT/memory-asm.o"
OBJECTS+=("$OUT/memory-asm.o")
m68k-linux-gnu-as -m68020 "$ROOT/native-renderer/amiga/selector_entry.s" -o "$OUT/entry.o"
m68k-linux-gnu-ld -Ttext=0 -e _start --emit-relocs "$OUT/entry.o" "${OBJECTS[@]}" -o "$OUT/selector.elf"
test -z "$(m68k-linux-gnu-nm -u "$OUT/selector.elf")"
"$AMIGA_BIN/elf2hunk" "$OUT/selector.elf" "$OUT/selector.exe" -s
"$AMIGA_BIN/exe2adf" -i "$OUT/selector.exe" -a "$OUT/selector.adf" -l NATIVE_SELECTOR
python3 "$ROOT/tools/verify-adf-file.py" --adf "$OUT/selector.adf" --file selector.exe --source "$OUT/selector.exe"
m68k-linux-gnu-size "$OUT/selector.elf"
