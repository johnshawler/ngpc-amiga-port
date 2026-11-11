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
  --control) MODE=control; OUT="$OUT/control" ;;
  --control-probe) MODE=control-probe; OUT="$OUT/control-probe" ;;
  --consumer-probe) MODE=consumer-probe; OUT="$OUT/consumer-probe" ;;
  --course) MODE=course; OUT="$OUT/course" ;;
  --lifecycle) MODE=lifecycle; OUT="$OUT/lifecycle" ;;
  *) echo "usage: $0 [--instrumented|--scene|--control|--control-probe|--consumer-probe|--course|--lifecycle]" >&2; exit 2 ;;
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
    cp "$ROOT/native-renderer/scene_renderer.c" "$ROOT/native-renderer/scene_renderer.h" "$ROOT/native-renderer/event_renderer.c" "$ROOT/native-renderer/event_renderer.h" "$ROOT/native-renderer/fine_event_renderer.c" "$ROOT/native-renderer/fine_event_renderer.h" "$ROOT/native-renderer/adaptive_renderer.c" "$ROOT/native-renderer/adaptive_renderer.h" "$ROOT/native-renderer/tile_renderer.h" "$STAGE/core-source/mednafen/ngp/"
    python3 - "$STAGE/core-source/mednafen/ngp/gfx.c" "$ROOT/reference/scene-check.c.inc" <<'PYHOOK'
from pathlib import Path
import sys
p=Path(sys.argv[1]); source=p.read_text()
anchor='void ngpgfx_reset(ngpgfx_t *gfx)'
hook='      switch (surface->depth)'
assert source.count(anchor)==1 and source.count(hook)==1
source=source.replace(anchor,Path(sys.argv[2]).read_text()+'\n'+anchor)
source=source.replace(hook,'      reference_check_scene(gfx, cfb_scanline);\n\n'+hook)
write='void ngpgfx_write8(ngpgfx_t *gfx, uint32 address, uint8 data)\n{'
assert source.count(write)==1
source=source.replace(write,write+'\n   reference_scene_write(gfx,address,data);')
reset='void ngpgfx_reset(ngpgfx_t *gfx)'
# The event observer must also see resets and save-state bulk replacement.
assert source.count(reset)==1
source=source.replace(reset+'\n{',reset+'\n{\n   nr_event_invalidate_all(&event_cache);nr_fine_event_invalidate_all(&fine_cache);nr_adaptive_invalidate_all(&adaptive_cache);')
state='   if(!MDFNSS_StateAction(sm, load, data_only, StateRegs, "GFX", false))'
assert source.count(state)==1
source=source.replace(state,'   if(load){nr_event_invalidate_all(&event_cache);nr_fine_event_invalidate_all(&fine_cache);nr_adaptive_invalidate_all(&adaptive_cache);}\n'+state)
p.write_text(source)
PYHOOK
fi
if [ "$MODE" = consumer-probe ]; then
    python3 "$ROOT/reference/consumer-probe.py" "$STAGE/core-source"
fi
if [ "$MODE" = control-probe ]; then
    python3 "$ROOT/reference/control-probe.py" "$STAGE/core-source"
fi
if [ "$MODE" = control ] || [ "$MODE" = course ] || [ "$MODE" = lifecycle ]; then
    cp "$ROOT/native-game/input_state.c" "$ROOT/native-game/input_state.h" "$ROOT/native-game/course_choice.c" "$ROOT/native-game/course_choice.h" "$STAGE/core-source/mednafen/ngp/TLCS-900h/"
    if [ "$MODE" = lifecycle ]; then
        cp "$ROOT/native-game/task_lifecycle.c" "$ROOT/native-game/task_lifecycle.h" "$STAGE/core-source/mednafen/ngp/TLCS-900h/"
    fi
    python3 - "$STAGE/core-source/mednafen/ngp/TLCS-900h/TLCS900h_interpret.c" "$ROOT/reference/control-check.c.inc" "$MODE" "$ROOT/reference/course-check.c.inc" "$ROOT/reference/lifecycle-check.c.inc" <<'PYCONTROL'
from pathlib import Path
import sys
p=Path(sys.argv[1]);s=p.read_text()
a='int32 TLCS900h_interpret(void)\n{'
assert s.count(a)==1
extra=Path(sys.argv[4]).read_text() if sys.argv[3] in ('course','lifecycle') else ''
call='reference_course_before();' if extra else ''
if sys.argv[3]=='lifecycle':
    extra+='\n'+Path(sys.argv[5]).read_text()
    call+='reference_lifecycle_before();'
s=s.replace(a,Path(sys.argv[2]).read_text()+'\n'+extra+'\n'+a+'\n    reference_control_before();'+call)
p.write_text(s)
PYCONTROL
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
