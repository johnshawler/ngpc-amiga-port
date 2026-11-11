#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/native-selector"
SCENARIO=${1:-accept}
case "${2:-}" in
    '') ;;
    --uncached) OUT="$ROOT/build/native-selector-uncached" ;;
    *) echo 'usage: selector_run.sh [accept|cancel|random] [--uncached]' >&2; exit 2 ;;
esac
case "$SCENARIO" in accept|cancel|random);; *) echo 'usage: selector_run.sh [accept|cancel|random] [--uncached]' >&2; exit 2;; esac
: "${PUAE_CORE:?Set PUAE_CORE to official libretro PUAE core}"
: "${LIBRETRO_INCLUDE:?Set LIBRETRO_INCLUDE to official libretro.h directory}"
: "${KICKSTART:?Set KICKSTART to authorized local A1200 3.1 ROM}"
RUN="$OUT/$SCENARIO"
mkdir -p "$RUN/system"
sha256sum "$ROOT"/native-renderer/amiga/selector* "$ROOT"/native-renderer/{scene_renderer,tile_renderer}.{c,h} "$ROOT"/native-game/*.{c,h} "$ROOT/tools/export-course-selector-assets.py" >"$RUN/authored-sources.sha256"
cp "$KICKSTART" "$RUN/system/kick40068.A1200"
cc -std=c99 -O2 -Wall -Wextra -Werror -I"$LIBRETRO_INCLUDE" "$ROOT/native-renderer/amiga/selector_host.c" -ldl -Wl,--no-as-needed -lm -o "$RUN/host"
timeout 180 "$RUN/host" "$PUAE_CORE" "$OUT/selector.adf" "$RUN/system" "${CAPTURE_FRAMES:-3000}" "$RUN/screen" "$RUN/chip" "$SCENARIO" >"$RUN/run.log" 2>&1
python3 "$ROOT/native-renderer/amiga/selector_verify.py" --directory "$RUN" --scenario "$SCENARIO"
