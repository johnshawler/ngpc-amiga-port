#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PIN=a50d5ac288a81f2104ddf43195a4efdd15c72227
SRC="$ROOT/third_party/ngpc/beetle-ngp-libretro"
OUT="$ROOT/reference/build"
[ "$(git -C "$SRC" rev-parse HEAD)" = "$PIN" ] || { echo 'Incorrect reference-core pin' >&2; exit 1; }
mkdir -p "$OUT"
# Each export has a fresh, uniquely owned directory. Never reuse/delete user sources.
STAGE=$(mktemp -d "$OUT/.stage.XXXXXXXX")
trap 'rm -rf -- "$STAGE"' EXIT HUP INT TERM
mkdir "$STAGE/core-source"
git -C "$SRC" archive "$PIN" | tar -x -C "$STAGE/core-source"
export LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=946684800
make -C "$STAGE/core-source" -j"${JOBS:-2}" platform=unix CC=gcc CXX=g++ GIT_VERSION='" a50d5ac"' > "$STAGE/build.log" 2>&1 || {
    mv "$STAGE/build.log" "$OUT/last-failed-build.log"
    echo "Build failed; see $OUT/last-failed-build.log" >&2
    exit 1
}
cp "$STAGE/core-source/mednafen_ngp_libretro.so" "$STAGE/mednafen_ngp_libretro.so"
gcc -std=c11 -O2 -fPIC -shared -Wall -Wextra -Werror "$ROOT/reference/clock.c" -o "$STAGE/clock.so"
{ printf 'source_commit=%s\n' "$PIN"; gcc --version | head -1; g++ --version | head -1; make --version | head -1; uname -sm; (cd "$STAGE" && sha256sum mednafen_ngp_libretro.so clock.so); } > "$STAGE/build-manifest.txt"
# Same-filesystem renames install only completed files atomically. Manifest last.
# This is per-file atomicity, not a transactional multi-artifact update.
for name in mednafen_ngp_libretro.so clock.so build.log build-manifest.txt; do
    mv "$STAGE/$name" "$OUT/$name"
done
cat "$OUT/build-manifest.txt"
