#!/bin/bash
# Full reverse-engineering pass over an NGPC ROM. Outputs go to build/ (git-ignored: ROM-derived).
#   tools/analyze.sh [rom.ngc] [trace-script]
set -e
cd "$(dirname "$0")/.."
ROM="${1:-$(ls rom/*.ngc rom/*.ngp 2>/dev/null | head -1)}"
SCRIPT="${2:-tools/ngpc-trace/scripts/ntm-tour.txt}"
[ -f "$ROM" ] || { echo "no ROM found in rom/ (pass one as \$1)"; exit 1; }

make -s -C tools/tlcs900dasm
make -s -C tools/ngpc-trace
mkdir -p build/trace build/disasm build/gfx build/assets

echo "== runtime trace: $SCRIPT"
tools/ngpc-trace/ngpc-trace "$ROM" "$SCRIPT" build/trace 2>&1 | grep -E "coverage|dumped" | tail -3
echo "== disassembly"
tools/tlcs900dasm/ngpc-dis --cov build/trace/coverage.cov --sym build/disasm/game.sym "$ROM" > build/disasm/game.asm
echo "== VRAM decode (checked against emulator frames)"
python3 -I tools/ngpc-gfx/ngpc_vram.py --out build/gfx --rom "$ROM" build/trace/*.vram
echo "== graphics located in ROM"
python3 -I tools/ngpc-gfx/ngpc_assets.py --rom "$ROM" --out build/assets build/trace/*.vram
