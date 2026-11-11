#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="${FIRE_BUILD_ROOT:-$ROOT/build/native-fire}"
SCENARIO=${1:-course0}
case "$SCENARIO" in course0|course1|course2);; *) echo 'usage: fire_run.sh course0|course1|course2' >&2; exit 2;; esac
: "${PUAE_CORE:?Set PUAE_CORE to official libretro PUAE core}"
: "${LIBRETRO_INCLUDE:?Set LIBRETRO_INCLUDE to official libretro.h directory}"
: "${KICKSTART:?Set KICKSTART to authorized A1200 3.1 ROM}"
RUN="${FIRE_RUN_ROOT:-$OUT/runs}/$SCENARIO"
if [ -e "$RUN/run.log" ]; then echo 'Run directory exists; choose a fresh FIRE_RUN_ROOT.' >&2;exit 2;fi
mkdir -p "$RUN/system"
cp "$OUT/build-sources.sha256" "$RUN/target-build-sources.sha256"
sha256sum "$OUT/fire.exe" "$OUT/fire.elf" "$OUT/fire.pool" "$OUT/intro.pool" >"$RUN/target-binaries.sha256"
sha256sum "$ROOT/native-renderer/amiga/fire_host.c" "$ROOT/native-renderer/amiga/fire_run.sh" >"$RUN/host-sources.sha256"
cmp "$OUT/fire.exe" "$OUT/hd/fire.exe";cmp "$OUT/fire.pool" "$OUT/hd/fire.pool";cmp "$OUT/intro.pool" "$OUT/hd/intro.pool"
cp "$KICKSTART" "$RUN/system/kick40068.A1200"
printf 'filesystem2=ro,DH0:NTM_FIRE:%s/,0\n' "$OUT/hd" >"$RUN/fire.uae"
cc -std=c99 -O2 -Wall -Wextra -Werror -I"$LIBRETRO_INCLUDE" "$ROOT/native-renderer/amiga/fire_host.c" -ldl -Wl,--no-as-needed -lm -o "$RUN/host"
timeout 360 "$RUN/host" "$PUAE_CORE" "$RUN/fire.uae" "$RUN/system" "${CAPTURE_FRAMES:-12000}" "$RUN/screen" "$RUN/chip" "$SCENARIO" >"$RUN/run.log" 2>&1
sha256sum --status -c "$RUN/target-binaries.sha256"
sha256sum --status -c "$RUN/host-sources.sha256"
