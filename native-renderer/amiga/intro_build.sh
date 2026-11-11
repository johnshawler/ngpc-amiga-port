#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/native-intro"
EXTRA=(-DSELECTOR_PLANAR_CACHE -DNTM_SCREEN_PRIVATE_ASSETS -DNTM_INTRO_PLANAR_WORDS)
: "${AMIGA_BIN:?Set AMIGA_BIN to official vscode-amiga-debug/bin/linux tools}"
mkdir -p "$OUT"
python3 "$ROOT/tools/export-course-selector-assets.py" --output "$OUT/selector_assets.inc"
python3 "$ROOT/tools/export-course-intro-assets.py" --output "$OUT/intro_assets.inc"
FLAGS=(-std=c99 -Wall -Wextra -Werror -m68020 -msoft-float -Os -ffreestanding -fno-builtin -fno-pic -fno-stack-protector -fomit-frame-pointer -fno-asynchronous-unwind-tables)
SOURCES=(native-renderer/amiga/intro_main.c native-renderer/amiga/selector_frontend.c native-renderer/amiga/selector_planar_cache.c native-renderer/amiga/selector_memory.c native-renderer/amiga/intro_copy.c native-renderer/scene_renderer.c native-renderer/tile_renderer.c)
for name in input_state course_choice task_lifecycle task_allocation random_byte viewport game_services course_parent course_screen course_driver course_palette course_initialization course_cursor course_application course_intro course_title round_record round_prepare course_intro_application; do SOURCES+=("native-game/$name.c"); done
# Detect a mixed build if a shared source/header changes during compilation.
INPUTS=("$ROOT/native-renderer/amiga/selector_main.c" "$ROOT/native-renderer/amiga/selector_entry.s" "$ROOT/native-renderer/amiga/selector_memory.s" "$ROOT/native-renderer/amiga/selector_generate_cache.c" "$ROOT"/native-game/*.h "$ROOT"/native-renderer/*.h "$ROOT"/native-renderer/amiga/*.h "$ROOT/tools/export-course-selector-assets.py" "$ROOT/tools/export-course-intro-assets.py")
for source in "${SOURCES[@]}"; do INPUTS+=("$ROOT/$source"); done
sha256sum "${INPUTS[@]}" >"$OUT/build-sources.sha256"
if [ ${#EXTRA[@]} -ne 0 ]; then
    GENERATOR=("$ROOT/native-renderer/amiga/selector_generate_cache.c")
    for source in "${SOURCES[@]:1}"; do GENERATOR+=("$ROOT/$source"); done
    cc -std=c99 -O2 -Wall -Wextra -Werror -I"$OUT" "${GENERATOR[@]}" -o "$OUT/generate-cache"
    "$OUT/generate-cache" "$OUT/selector_planar.inc"
fi
OBJECTS=()
for source in "${SOURCES[@]}"; do
    object="$OUT/$(basename "${source%.c}").o"
    OPT=(); case "$source" in native-renderer/scene_renderer.c) OPT=(-O2);; esac
    m68k-linux-gnu-gcc "${FLAGS[@]}" "${OPT[@]}" "${EXTRA[@]}" -DSELECTOR_FAST_MEMORY -I"$OUT" -c "$ROOT/$source" -o "$object"
    OBJECTS+=("$object")
done
m68k-linux-gnu-as -m68020 "$ROOT/native-renderer/amiga/selector_memory.s" -o "$OUT/memory-asm.o"
OBJECTS+=("$OUT/memory-asm.o")
m68k-linux-gnu-as -m68020 "$ROOT/native-renderer/amiga/selector_entry.s" -o "$OUT/entry.o"
m68k-linux-gnu-ld -Ttext=0 -e _start --emit-relocs "$OUT/entry.o" "${OBJECTS[@]}" -o "$OUT/intro.elf"
test -z "$(m68k-linux-gnu-nm -u "$OUT/intro.elf")"
"$AMIGA_BIN/elf2hunk" "$OUT/intro.elf" "$OUT/intro.exe" -s
"$AMIGA_BIN/exe2adf" -i "$OUT/intro.exe" -a "$OUT/intro.adf" -l NATIVE_INTRO
m68k-linux-gnu-size "$OUT/intro.elf"
sha256sum --status -c "$OUT/build-sources.sha256"
