#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/native-intro"
SCENARIO=${1:-course0}
case "$SCENARIO" in course0|course1|course2|cancel|random);; *) echo 'usage: intro_run.sh [course0|course1|course2|cancel|random]' >&2; exit 2;; esac
: "${PUAE_CORE:?Set PUAE_CORE to official libretro PUAE core}"
: "${LIBRETRO_INCLUDE:?Set LIBRETRO_INCLUDE to official libretro.h directory}"
: "${KICKSTART:?Set KICKSTART to authorized local A1200 3.1 ROM}"
RUN="$OUT/$SCENARIO"
mkdir -p "$RUN/system"
sha256sum "$ROOT"/native-renderer/amiga/{intro*,selector*} "$ROOT"/native-renderer/{scene_renderer,tile_renderer}.{c,h} "$ROOT"/native-game/*.{c,h} "$ROOT/tools/export-course-intro-assets.py" >"$RUN/authored-sources.sha256"
cp "$KICKSTART" "$RUN/system/kick40068.A1200"
cc -std=c99 -O2 -Wall -Wextra -Werror -I"$LIBRETRO_INCLUDE" "$ROOT/native-renderer/amiga/intro_host.c" -ldl -Wl,--no-as-needed -lm -o "$RUN/host"
timeout 180 "$RUN/host" "$PUAE_CORE" "$OUT/intro.adf" "$RUN/system" "${CAPTURE_FRAMES:-4000}" "$RUN/screen" "$RUN/chip" "$SCENARIO" >"$RUN/run.log" 2>&1
python3 "$ROOT/native-renderer/amiga/intro_verify.py" --directory "$RUN" --scenario "$SCENARIO"
