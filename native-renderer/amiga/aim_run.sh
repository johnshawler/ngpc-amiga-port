#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/native-aim"
SCENARIO=${1:-course0}
case "$SCENARIO" in course0|course1|course2|cancel|random|skip0|skip1|skip2|skipboth|recordskip0|recordskip1|recordskip2|recordheld|teeskip0|teeskip1|teeskip2|teeheld|teeseen0|teeseen1|teeseen2|shotseen0|shotseen1|shotseen2|shotskip0|shotskip1|shotskip2|shotheld|raster0|raster1|raster2);; *) echo 'usage: shot_run.sh [course0|course1|course2|cancel|random|skip0|skip1|skip2|skipboth|recordskip0|recordskip1|recordskip2|recordheld|teeskip0|teeskip1|teeskip2|teeheld|teeseen0|teeseen1|teeseen2]' >&2; exit 2;; esac
: "${PUAE_CORE:?Set PUAE_CORE to official libretro PUAE core}"
: "${LIBRETRO_INCLUDE:?Set LIBRETRO_INCLUDE to official libretro.h directory}"
: "${KICKSTART:?Set KICKSTART to authorized local A1200 3.1 ROM}"
RUN="${AIM_RUN_ROOT:-$OUT}/$SCENARIO"
if compgen -G "$RUN/chip.*.bin" >/dev/null; then echo "Run directory already contains captures; select a fresh AIM_RUN_ROOT." >&2; exit 2; fi
mkdir -p "$RUN/system"
sha256sum "$ROOT"/native-renderer/amiga/{intro*,round*,selector*,tee*,shot*,aim*} "$ROOT"/native-renderer/{scene_renderer,tile_renderer}.{c,h} "$ROOT"/native-game/*.{c,h} "$ROOT/tools/export-course-intro-assets.py" "$ROOT/tools/export-shot-assets.py" "$ROOT/tools/export-aim-assets.py" "$ROOT/tools/aim_pack.py" "$ROOT/tools/pack-aim-startup.py" "$ROOT/reference/compare_aim_target.py" >"$RUN/authored-sources.sha256"
cp "$KICKSTART" "$RUN/system/kick40068.A1200"
cc -std=c99 -O2 -Wall -Wextra -Werror -I"$LIBRETRO_INCLUDE" "$ROOT/native-renderer/amiga/aim_host.c" -ldl -Wl,--no-as-needed -lm -o "$RUN/host"
timeout 240 "$RUN/host" "$PUAE_CORE" "$OUT/aim.adf" "$RUN/system" "${CAPTURE_FRAMES:-7000}" "$RUN/screen" "$RUN/chip" "$SCENARIO" >"$RUN/run.log" 2>&1
bash "$ROOT/native-renderer/amiga/aim_reference_build.sh"
NTM_SHOT_APPLICATION_LIBRARY="$OUT/oracle.so" python3 "$ROOT/reference/compare_aim_target.py" --directory "$RUN" >"$RUN/original.log"
python3 "$ROOT/native-renderer/amiga/aim_verify.py" --directory "$RUN" --scenario "$SCENARIO"
