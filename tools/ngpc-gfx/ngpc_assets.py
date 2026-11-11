#!/usr/bin/env python3
"""Locate NGPC graphics in the cartridge ROM using VRAM dumps, and export them.

Neo Turf Masters stores its tiles uncompressed (see ngpc_vram.py --rom), so every block of
consecutive character-RAM tiles that a screen loaded can be found verbatim in the ROM. This tool
matches runs of consecutive VRAM tiles to consecutive ROM tiles and writes:

  assets.json             one entry per ROM graphics block: ROM address, tile count, which dump and
                          VRAM tile slot it was seen in, and the palette (12-bit 0x0BGR) used there
  blk_<romaddr>.png       the block as an indexed 4-colour PNG (16 tiles per row), with that palette;
                          index 0 is transparent
  rom_coverage.txt        merged ROM ranges known to hold tile graphics

Usage: ngpc_assets.py --rom ROM.ngc --out DIR dump1.vram [dump2.vram ...]
"""
import argparse
import json
import os
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ngpc_vram import Vram, tile_usage  # noqa: E402

ROM_BASE = 0x200000
MIN_RUN = 4          # tiles; shorter runs are too ambiguous (blank / repeated tiles)


def tile_bytes(v, t):
    o = 0xA000 - 0x8000 + t * 16
    return bytes(v.d[o:o + 16])


def find_all(rom, data, limit=64):
    out, i = [], rom.find(data)
    while i >= 0 and len(out) < limit:
        out.append(i)
        i = rom.find(data, i + 1)
    return out


def runs_in_dump(v, rom):
    """Yield (vram_start, rom_offset, count): maximal runs where VRAM tile t+k == ROM tile at off+16k."""
    t = 0
    while t < 512:
        data = tile_bytes(v, t)
        if data.count(0) == 16 or data.count(data[0]) == 16:
            t += 1
            continue
        best = (0, None)
        for off in find_all(rom, data):
            n = 1
            while t + n < 512 and rom[off + 16 * n: off + 16 * n + 16] == tile_bytes(v, t + n):
                n += 1
            if n > best[0]:
                best = (n, off)
        if best[0] >= MIN_RUN:
            yield t, best[1], best[0]
            t += best[0]
        else:
            t += 1


def palette_for(v, use, vram_start, count):
    """Most common (bank, palette) among the run's tiles that are actually on screen."""
    votes = {}
    for t in range(vram_start, vram_start + count):
        if t in use:
            votes[use[t]] = votes.get(use[t], 0) + 1
    if not votes:
        return None
    bank, pal = max(votes, key=votes.get)
    return {"bank": ["sprite", "scr1", "scr2"][bank], "palette": pal,
            "colours": [v.colour(bank, pal, i) for i in range(4)]}


def export_png(rom, off, count, colours, path):
    cols = 16
    rows = (count + cols - 1) // cols
    img = Image.new("P", (cols * 8, rows * 8), 0)
    px = img.load()
    for k in range(count):
        ox, oy = (k % cols) * 8, (k // cols) * 8
        for y in range(8):
            row = rom[off + 16 * k + 2 * y] | rom[off + 16 * k + 2 * y + 1] << 8
            for x in range(8):
                px[ox + x, oy + y] = (row >> (14 - 2 * x)) & 3
    pal = []
    for c in (colours or [0x000, 0x555, 0xAAA, 0xFFF]):
        pal += [(c & 15) * 17, (c >> 4 & 15) * 17, (c >> 8 & 15) * 17]
    img.putpalette(pal + [0] * (768 - len(pal)))
    img.info["transparency"] = 0
    img.save(path, transparency=0)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rom", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("vram", nargs="+")
    args = ap.parse_args()
    rom = open(args.rom, "rb").read()
    os.makedirs(args.out, exist_ok=True)

    blocks = {}   # rom offset -> entry
    for path in args.vram:
        stem = os.path.splitext(os.path.basename(path))[0]
        v = Vram(open(path, "rb").read())
        use = tile_usage(v)
        for vstart, off, n in runs_in_dump(v, rom):
            e = blocks.get(off)
            seen = {"dump": stem, "vram_tile": vstart, "tiles": n, "palette": palette_for(v, use, vstart, n)}
            if e is None or n > e["tiles"]:
                prev = e["seen"] if e else []
                blocks[off] = {"rom_addr": "0x%06x" % (ROM_BASE + off), "tiles": n, "seen": prev + [seen]}
            else:
                e["seen"].append(seen)

    # Drop blocks wholly contained in a larger block found elsewhere.
    offs = sorted(blocks)
    keep = []
    for o in offs:
        n = blocks[o]["tiles"]
        if any(p != o and p <= o and o + 16 * n <= p + 16 * blocks[p]["tiles"] for p in offs):
            continue
        keep.append(o)

    entries = []
    for o in keep:
        e = blocks[o]
        pal = next((s["palette"] for s in e["seen"] if s["palette"] and s["tiles"] == e["tiles"]), None) \
            or next((s["palette"] for s in e["seen"] if s["palette"]), None)
        name = "blk_%06x.png" % (ROM_BASE + o)
        export_png(rom, o, e["tiles"], pal["colours"] if pal else None, os.path.join(args.out, name))
        entries.append({**e, "png": name, "palette": pal})
    json.dump(entries, open(os.path.join(args.out, "assets.json"), "w"), indent=1)

    # merged ranges
    ranges = []
    for o in keep:
        a, b = o, o + 16 * blocks[o]["tiles"]
        if ranges and a <= ranges[-1][1]:
            ranges[-1][1] = max(ranges[-1][1], b)
        else:
            ranges.append([a, b])
    total = sum(b - a for a, b in ranges)
    with open(os.path.join(args.out, "rom_coverage.txt"), "w") as f:
        for a, b in ranges:
            f.write("%06x-%06x  %6d bytes  %4d tiles\n" % (ROM_BASE + a, ROM_BASE + b, b - a, (b - a) // 16))
        f.write("total %d bytes (%d tiles) in %d ranges\n" % (total, total // 16, len(ranges)))
    print("%d graphics blocks, %d tiles (%d KiB) located in ROM across %d ranges"
          % (len(entries), total // 16, total // 1024, len(ranges)))


if __name__ == "__main__":
    sys.exit(main())
