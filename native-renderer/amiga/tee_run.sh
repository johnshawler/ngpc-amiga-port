#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/native-tee"
SCENARIO=${1:-course0}
case "$SCENARIO" in course0|course1|course2|cancel|random|skip0|skip1|skip2|skipboth|recordskip0|recordskip1|recordskip2|recordheld|teeskip0|teeskip1|teeskip2|teeheld|teeseen0|teeseen1|teeseen2);; *) echo 'usage: tee_run.sh [course0|course1|course2|cancel|random|skip0|skip1|skip2|skipboth|recordskip0|recordskip1|recordskip2|recordheld|teeskip0|teeskip1|teeskip2|teeheld|teeseen0|teeseen1|teeseen2]' >&2; exit 2;; esac
: "${PUAE_CORE:?Set PUAE_CORE to official libretro PUAE core}"
: "${LIBRETRO_INCLUDE:?Set LIBRETRO_INCLUDE to official libretro.h directory}"
: "${KICKSTART:?Set KICKSTART to authorized local A1200 3.1 ROM}"
RUN="${TEE_RUN_ROOT:-$OUT}/$SCENARIO"
mkdir -p "$RUN/system"
sha256sum "$ROOT"/native-renderer/amiga/{intro*,round*,selector*,tee*} "$ROOT"/native-renderer/{scene_renderer,tile_renderer}.{c,h} "$ROOT"/native-game/*.{c,h} "$ROOT/tools/export-course-intro-assets.py" >"$RUN/authored-sources.sha256"
cp "$KICKSTART" "$RUN/system/kick40068.A1200"
cc -std=c99 -O2 -Wall -Wextra -Werror -I"$LIBRETRO_INCLUDE" "$ROOT/native-renderer/amiga/tee_host.c" -ldl -Wl,--no-as-needed -lm -o "$RUN/host"
timeout 180 "$RUN/host" "$PUAE_CORE" "$OUT/tee.adf" "$RUN/system" "${CAPTURE_FRAMES:-6000}" "$RUN/screen" "$RUN/chip" "$SCENARIO" >"$RUN/run.log" 2>&1
python3 "$ROOT/native-renderer/amiga/tee_verify.py" --directory "$RUN" --scenario "$SCENARIO"
