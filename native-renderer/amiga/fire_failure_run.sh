#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Actual DOS failure boots; never modify the ordinary package or prior evidence.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="${FIRE_BUILD_ROOT:-$ROOT/build/native-fire}"
RUN="${FIRE_FAILURE_ROOT:-$OUT/failure}"
: "${PUAE_CORE:?Set PUAE_CORE}"
: "${LIBRETRO_INCLUDE:?Set LIBRETRO_INCLUDE}"
: "${KICKSTART:?Set KICKSTART to authorized A1200 ROM}"
if [ -e "$RUN" ]; then echo 'Failure run directory exists; choose a fresh FIRE_FAILURE_ROOT.' >&2;exit 2;fi
mkdir -p "$RUN"
cc -std=c99 -O2 -Wall -Wextra -Werror -I"$LIBRETRO_INCLUDE" "$ROOT/native-renderer/amiga/fire_host.c" -ldl -Wl,--no-as-needed -lm -o "$RUN/host"
python3 - "$OUT" "$RUN" "$KICKSTART" <<'PY'
from pathlib import Path
import shutil,sys
source,root,kick=map(Path,sys.argv[1:])
for case in ('missing','corrupt'):
 d=root/case;(d/'hd/S').mkdir(parents=True);(d/'system').mkdir()
 for name in ('fire.exe','intro.pool'):shutil.copyfile(source/'hd'/name,d/'hd'/name)
 shutil.copyfile(source/'hd/S/startup-sequence',d/'hd/S/startup-sequence')
 shutil.copyfile(kick,d/'system/kick40068.A1200')
 if case=='corrupt':
  data=bytearray((source/'fire.pool').read_bytes());data[16]^=1;(d/'hd/fire.pool').write_bytes(data)
 (d/'fire.uae').write_text('filesystem2=ro,DH0:NTM_FIRE:'+str((d/'hd').resolve())+'/,0\n')
PY
for case in missing corrupt; do
 D="$RUN/$case"
 set +e
 timeout 120 "$RUN/host" "$PUAE_CORE" "$D/fire.uae" "$D/system" 6000 "$D/screen" "$D/chip" course0 >"$D/run.log" 2>&1
 status=$?
 set -e
 printf '%s\n' "$status" >"$D/host-status"
 test "$status" = 8 # Host did not see a running fire frontend, as required.
done
python3 - "$RUN" <<'PY'
from pathlib import Path
import hashlib,json,sys
from PIL import Image
root=Path(sys.argv[1]);result={'result':'PASS','cases':{},'checks':['No native hardware-takeover marker','Bounded host remains at stage0','Screens retained for manual AmigaDOS error/returncode inspection']}
for case in ('missing','corrupt'):
 d=root/case;data=(d/'chip').read_bytes();assert len(data)==2097152 and b'NFIR0001' not in data
 assert 'CONTROL stage=0' in (d/'run.log').read_text()
 Image.open(d/'screen').save(d/'screen.png')
 result['cases'][case]={'frames':6000,'host_status':int((d/'host-status').read_text()),'executable_sha256':hashlib.sha256((d/'hd/fire.exe').read_bytes()).hexdigest()}
(root/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
PY
