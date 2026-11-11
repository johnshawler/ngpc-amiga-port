#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="${FIRE_BUILD_ROOT:-$ROOT/build/native-fire}"
EXTRA=(-DSELECTOR_PLANAR_CACHE -DNTM_SCREEN_PRIVATE_ASSETS -DNTM_INTRO_PLANAR_WORDS)
: "${AMIGA_BIN:?Set AMIGA_BIN to official vscode-amiga-debug/bin/linux tools}"
mkdir -p "$OUT"
python3 "$ROOT/tools/export-course-selector-assets.py" --output "$OUT/selector_assets.inc"
python3 "$ROOT/tools/export-course-intro-assets.py" --round --output "$OUT/intro_assets.inc"
python3 "$ROOT/tools/export-fire-assets.py" --output "$OUT"
FLAGS=(-std=c99 -Wall -Wextra -Werror -m68020 -msoft-float -Os -ffreestanding -fno-builtin -fno-pic -fno-stack-protector -fomit-frame-pointer -fno-asynchronous-unwind-tables -fstack-usage)
SOURCES=(native-renderer/amiga/fire_main.c native-renderer/amiga/fire_boot.c native-renderer/amiga/fire_stream.c native-renderer/amiga/aim_frontend.c native-renderer/amiga/shot_frontend.c native-renderer/amiga/selector_frontend.c native-renderer/amiga/tee_planar_cache.c native-renderer/amiga/selector_memory.c native-renderer/amiga/intro_copy.c native-renderer/scene_renderer.c native-renderer/tile_renderer.c)
for name in input_state course_choice task_lifecycle task_allocation random_byte viewport game_services course_parent course_screen course_driver course_palette course_initialization course_cursor course_application course_intro course_title round_record round_prepare course_intro_application hole_prepare tee_graphics tee_objects tee_setup tee_setup_application shot_math shot_terrain shot_prepare shot_raster shot_palette shot_scene shot_banner shot_application shot_aim shot_aim_setup shot_hud aim_world shot_aim_application shot_scene_objects shot_club shot_markers shot_open_application shot_camera shot_ready shot_control_application shot_fire shot_spin shot_trajectory shot_golfer shot_projection shot_golfer_animation shot_golfer_features shot_golfer_drawing shot_fire_application; do SOURCES+=("native-game/$name.c"); done
# Detect a mixed build if a shared source/header changes during compilation.
INPUTS=("$ROOT/native-renderer/amiga/fire_build.sh" "$ROOT/native-renderer/amiga/fire_entry.s" "$ROOT/tools/export-fire-assets.py" "$ROOT/native-renderer/amiga/selector_main.c" "$ROOT/native-renderer/amiga/selector_entry.s" "$ROOT/native-renderer/amiga/selector_memory.s" "$ROOT/native-renderer/amiga/selector_generate_cache.c" "$ROOT"/native-game/*.h "$ROOT"/native-renderer/*.h "$ROOT"/native-renderer/amiga/*.h "$ROOT/tools/export-course-selector-assets.py" "$ROOT/tools/export-course-intro-assets.py" "$ROOT/tools/export-aim-assets.py" "$ROOT/native-renderer/amiga/tee_pack_cache.py" "$ROOT/tools/aim_pack.py" "$ROOT/tools/pack-aim-startup.py")
for source in "${SOURCES[@]}"; do INPUTS+=("$ROOT/$source"); done
sha256sum "${INPUTS[@]}" >"$OUT/build-sources.sha256"
if [ ${#EXTRA[@]} -ne 0 ]; then
    GENERATOR=("$ROOT/native-renderer/amiga/selector_generate_cache.c")
    for source in "${SOURCES[@]:1}"; do
        case "$source" in *fire_boot.c|*fire_stream.c) continue;; *tee_planar_cache.c) source=native-renderer/amiga/selector_planar_cache.c;; esac
        GENERATOR+=("$ROOT/$source"); done
    cc -std=c99 -O2 -Wall -Wextra -Werror -I"$OUT" "${GENERATOR[@]}" -o "$OUT/generate-cache"
    "$OUT/generate-cache" "$OUT/selector_planar.inc"
    python3 "$ROOT/native-renderer/amiga/tee_pack_cache.py" "$OUT/selector_planar.inc" "$OUT/tee_planar.inc"
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
m68k-linux-gnu-as -m68020 "$ROOT/native-renderer/amiga/fire_entry.s" -o "$OUT/entry.o"
m68k-linux-gnu-ld -Ttext=0 -e _start --emit-relocs "$OUT/entry.o" "${OBJECTS[@]}" -o "$OUT/fire.elf"
test -z "$(m68k-linux-gnu-nm -u "$OUT/fire.elf")"
"$AMIGA_BIN/elf2hunk" "$OUT/fire.elf" "$OUT/fire.exe" -s
# First package is explicitly a hard-disk directory, not an unproved DD fit.
mkdir -p "$OUT/hd/S"
cp "$OUT/fire.exe" "$OUT/fire.pool" "$OUT/intro.pool" "$OUT/hd/"
printf 'cd SYS:\nfire.exe\n' >"$OUT/hd/S/startup-sequence"
wc -c "$OUT/fire.exe" "$OUT/fire.pool" "$OUT/intro.pool"
m68k-linux-gnu-size "$OUT/fire.elf"
sha256sum --status -c "$OUT/build-sources.sha256"
