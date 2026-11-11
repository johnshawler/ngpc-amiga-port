#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PIN=a50d5ac288a81f2104ddf43195a4efdd15c72227
SRC="$ROOT/third_party/ngpc/beetle-ngp-libretro"
OUT="$ROOT/reference/build"
MODE=original
case "${1:-}" in
  "") ;;
  --instrumented) MODE=instrumented; OUT="$OUT/instrumented" ;;
  --scene) MODE=scene; OUT="$OUT/scene" ;;
  *) echo "usage: $0 [--instrumented|--scene]" >&2; exit 2 ;;
esac
[ "$(git -C "$SRC" rev-parse HEAD)" = "$PIN" ] || { echo 'Incorrect reference-core pin' >&2; exit 1; }
mkdir -p "$OUT"
# Each export has a fresh, uniquely owned directory. Never reuse/delete user sources.
STAGE=$(mktemp -d "$OUT/.stage.XXXXXXXX")
trap 'rm -rf -- "$STAGE"' EXIT HUP INT TERM
mkdir "$STAGE/core-source"
git -C "$SRC" archive "$PIN" | tar -x -C "$STAGE/core-source"
if [ "$MODE" = instrumented ]; then
    cat "$ROOT/reference/state-export.c.inc" >> "$STAGE/core-source/libretro.c"
fi
if [ "$MODE" = scene ]; then
    cp "$ROOT/native-renderer/scene_renderer.c" "$ROOT/native-renderer/scene_renderer.h" "$ROOT/native-renderer/tile_renderer.h" "$STAGE/core-source/mednafen/ngp/"
    python3 - "$STAGE/core-source/mednafen/ngp/gfx.c" "$ROOT/reference/scene-check.c.inc" <<'PYHOOK'
from pathlib import Path
import sys
p=Path(sys.argv[1]); source=p.read_text()
anchor='bool ngpgfx_draw(ngpgfx_t *gfx, void *data, bool skip)'
hook='      switch (surface->depth)'
assert source.count(anchor)==1 and source.count(hook)==1
source=source.replace(anchor,Path(sys.argv[2]).read_text()+'\n'+anchor)
source=source.replace(hook,'      reference_check_scene(gfx, cfb_scanline);\n\n'+hook)
p.write_text(source)
PYHOOK
fi
export LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=946684800
make -C "$STAGE/core-source" -j"${JOBS:-2}" platform=unix CC=gcc CXX=g++ GIT_VERSION='" a50d5ac"' > "$STAGE/build.log" 2>&1 || {
    mv "$STAGE/build.log" "$OUT/last-failed-build.log"
    echo "Build failed; see $OUT/last-failed-build.log" >&2
    exit 1
}
cp "$STAGE/core-source/mednafen_ngp_libretro.so" "$STAGE/mednafen_ngp_libretro.so"
gcc -std=c11 -O2 -fPIC -shared -Wall -Wextra -Werror "$ROOT/reference/clock.c" -o "$STAGE/clock.so"
{ printf 'source_commit=%s\nmode=%s\n' "$PIN" "$MODE"; gcc --version | head -1; g++ --version | head -1; make --version | head -1; uname -sm; (cd "$STAGE" && sha256sum mednafen_ngp_libretro.so clock.so); } > "$STAGE/build-manifest.txt"
# Same-filesystem renames install only completed files atomically. Manifest last.
# This is per-file atomicity, not a transactional multi-artifact update.
for name in mednafen_ngp_libretro.so clock.so build.log build-manifest.txt; do
    mv "$STAGE/$name" "$OUT/$name"
done
cat "$OUT/build-manifest.txt"
