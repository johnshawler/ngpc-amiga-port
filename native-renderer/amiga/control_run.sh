#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="${CONTROL_BUILD_ROOT:-$ROOT/build/native-control}"
SCENARIO=${1:-course0}
case "$SCENARIO" in course0|course1|course2|idle|cancel|random|skip0|skip1|skip2|skipboth|recordskip0|recordskip1|recordskip2|recordheld|teeskip0|teeskip1|teeskip2|teeheld|teeseen0|teeseen1|teeseen2|shotseen0|shotseen1|shotseen2|shotskip0|shotskip1|shotskip2|shotheld|raster0|raster1|raster2);; *) echo 'usage: control_run.sh [course0|course1|course2|cancel|random|skip0|skip1|skip2|skipboth|recordskip0|recordskip1|recordskip2|recordheld|teeskip0|teeskip1|teeskip2|teeheld|teeseen0|teeseen1|teeseen2]' >&2; exit 2;; esac
: "${PUAE_CORE:?Set PUAE_CORE to official libretro PUAE core}"
: "${LIBRETRO_INCLUDE:?Set LIBRETRO_INCLUDE to official libretro.h directory}"
: "${KICKSTART:?Set KICKSTART to authorized local A1200 3.1 ROM}"
RUN="${CONTROL_RUN_ROOT:-$OUT}/$SCENARIO"
if compgen -G "$RUN/chip.*.bin" >/dev/null; then echo "Run directory already contains captures; select a fresh CONTROL_RUN_ROOT." >&2; exit 2; fi
mkdir -p "$RUN/system"
cp "$OUT/build-sources.sha256" "$RUN/target-build-sources.sha256"
sha256sum "$OUT/control.exe" "$OUT/control.elf" "$OUT/control.adf" >"$RUN/target-binaries.sha256"
sha256sum "$ROOT"/native-renderer/amiga/{intro*,round*,selector*,tee*,shot*,aim*,control*} "$ROOT"/native-renderer/{scene_renderer,tile_renderer}.{c,h} "$ROOT"/native-game/*.{c,h} "$ROOT/tools/export-course-intro-assets.py" "$ROOT/tools/export-shot-assets.py" "$ROOT/tools/export-aim-assets.py" "$ROOT/tools/aim_pack.py" "$ROOT/tools/pack-aim-startup.py" "$ROOT/reference/compare_control_target.py" >"$RUN/authored-sources.sha256"
cp "$KICKSTART" "$RUN/system/kick40068.A1200"
cc -std=c99 -O2 -Wall -Wextra -Werror -I"$LIBRETRO_INCLUDE" "$ROOT/native-renderer/amiga/control_host.c" -ldl -Wl,--no-as-needed -lm -o "$RUN/host"
timeout 360 "$RUN/host" "$PUAE_CORE" "$OUT/control.adf" "$RUN/system" "${CAPTURE_FRAMES:-12000}" "$RUN/screen" "$RUN/chip" "$SCENARIO" >"$RUN/run.log" 2>&1
bash "$ROOT/native-renderer/amiga/control_reference_build.sh"
NTM_SHOT_APPLICATION_LIBRARY="$OUT/oracle.so" python3 "$ROOT/reference/compare_control_target.py" --directory "$RUN" >"$RUN/original.log" 2>&1
python3 "$ROOT/native-renderer/amiga/control_verify.py" --directory "$RUN" --scenario "$SCENARIO"
