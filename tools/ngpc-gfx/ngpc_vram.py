#!/usr/bin/env python3
"""Decode Neo Geo Pocket Color video RAM dumps (from tools/ngpc-trace `dump NAME`).

For each NAME.vram (0x8000-0xBFFF) this writes, next to it or into --out:
  NAME_screen.png   160x152 screen recomposed from VRAM (layers, scroll, window, sprites)
  NAME_scr1.png     full 256x256 scroll plane 1 (transparent where colour index 0)
  NAME_scr2.png     full 256x256 scroll plane 2
  NAME_tiles.png    all 512 character tiles, each coloured with the first palette it is used with
  NAME.json         palettes (12-bit 0x0BGR), registers, sprite table, tile usage
and, if NAME.ppm (the emulator frame) exists, checks the recomposition against it.

With --rom, also reports how many in-use tiles appear byte-for-byte in the ROM, i.e. whether
the game's graphics are stored uncompressed.

Layout and rendering order follow the Mednafen NGP core (mednafen/ngp/gfx.c):
  tile rows are little-endian u16, leftmost pixel in bits 15-14;
  map entry: bit15 H flip, bit14 V flip, bits12-9 palette, bits8-0 tile;
  sprite: u16 (same flag layout, bits12-11 priority, bit10 H chain, bit9 V chain) + X + Y;
  chained sprites are positioned relative to the previous sprite; equal priority: lower index wins.
"""
import argparse
import json
import os
import sys

from PIL import Image

BASE = 0x8000


class Vram:
    def __init__(self, data):
        if len(data) != 0x4000:
            raise ValueError("expected a 16 KiB dump of 0x8000-0xBFFF")
        self.d = data

    def u8(self, a):
        return self.d[a - BASE]

    def u16(self, a):
        return self.d[a - BASE] | self.d[a - BASE + 1] << 8

    # ---- registers
    @property
    def regs(self):
        return {
            "win_x": self.u8(0x8002), "win_y": self.u8(0x8003),
            "win_w": self.u8(0x8004), "win_h": self.u8(0x8005),
            "lcd_ctl": self.u8(0x8012), "spr_ofs_x": self.u8(0x8020), "spr_ofs_y": self.u8(0x8021),
            "prio": self.u8(0x8030),
            "scr1_x": self.u8(0x8032), "scr1_y": self.u8(0x8033),
            "scr2_x": self.u8(0x8034), "scr2_y": self.u8(0x8035),
            "bg_ctl": self.u8(0x8118),
        }

    # ---- palettes: 0 = sprites, 1 = scroll 1, 2 = scroll 2
    def colour(self, bank, pal, idx):
        return self.u16(0x8200 + bank * 0x80 + pal * 8 + idx * 2) & 0x0FFF

    def bg_colour(self):
        return self.u16(0x83E0 + (self.u8(0x8118) & 7) * 2) & 0x0FFF

    def window_colour(self):
        return self.u16(0x83F0 + (self.u8(0x8012) & 7) * 2) & 0x0FFF

    # ---- tiles
    def tile_row(self, tile, row):
        return self.u16(0xA000 + tile * 16 + row * 2)

    def tile_px(self, tile, x, y, hflip=False, vflip=False):
        if vflip:
            y = 7 - y
        if hflip:
            x = 7 - x
        return (self.tile_row(tile, y) >> (14 - 2 * x)) & 3

    def map_entry(self, plane, col, row):
        e = self.u16((0x9000 if plane == 1 else 0x9800) + (row * 32 + col) * 2)
        return {"tile": e & 0x1FF, "pal": (e >> 9) & 15, "h": bool(e & 0x8000), "v": bool(e & 0x4000)}

    def sprites(self):
        out, lx, ly = [], 0, 0
        for n in range(64):
            e = self.u16(0x8800 + n * 4)
            x, y = self.u8(0x8802 + n * 4), self.u8(0x8803 + n * 4)
            if e & 0x0400:
                x = lx + x
            if e & 0x0200:
                y = ly + y
            lx, ly = x, y
            out.append({"n": n, "tile": e & 0x1FF, "pal": self.u8(0x8C00 + n) & 15,
                        "prio": (e >> 11) & 3, "h": bool(e & 0x8000), "v": bool(e & 0x4000),
                        "hchain": bool(e & 0x0400), "vchain": bool(e & 0x0200), "x": x, "y": y})
        return out


def rgb(c):
    """12-bit 0x0BGR -> 8-bit RGB tuple."""
    return ((c & 15) * 17, (c >> 4 & 15) * 17, (c >> 8 & 15) * 17)


def render_plane(v, plane):
    img = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    px = img.load()
    bank = plane
    for row in range(32):
        for col in range(32):
            e = v.map_entry(plane, col, row)
            for y in range(8):
                for x in range(8):
                    i = v.tile_px(e["tile"], x, y, e["h"], e["v"])
                    if i:
                        px[col * 8 + x, row * 8 + y] = rgb(v.colour(bank, e["pal"], i)) + (255,)
    return img


def render_screen(v):
    """Recompose the 160x152 screen the way mednafen/ngp/gfx.c draw_scanline_colour() does."""
    r = v.regs
    W, H = 160, 152
    win = rgb(v.window_colour())
    bg = rgb(v.bg_colour())
    out = [[win] * W for _ in range(H)]
    z = [[0] * W for _ in range(H)]
    swap = bool(r["prio"] & 0x80)
    back, front = (1, 2) if swap else (2, 1)
    DEPTH = {"back": 3, "front": 5}   # sprites use priority*2 = 2, 4, 6

    wx0, wy0 = r["win_x"], r["win_y"]
    wx1, wy1 = min(wx0 + r["win_w"], W), wy0 + r["win_h"]
    inwin = lambda x, y: wx0 <= x < wx1 and wy0 <= y < wy1

    def plot(x, y, colour, depth):
        if 0 <= x < W and 0 <= y < H and inwin(x, y) and depth > z[y][x]:
            z[y][x] = depth
            out[y][x] = colour

    for y in range(H):
        for x in range(W):
            if inwin(x, y):
                out[y][x] = bg

    for plane, depth in ((back, DEPTH["back"]), (front, DEPTH["front"])):
        sx, sy = r["scr%d_x" % plane], r["scr%d_y" % plane]
        for y in range(H):
            line = (y + sy) & 255
            for col in range(32):
                e = v.map_entry(plane, col, line >> 3)
                x0 = ((col << 3) - sx) & 255
                if x0 > 0xF8:
                    x0 -= 256
                for tx in range(8):
                    i = v.tile_px(e["tile"], tx, line & 7, e["h"], e["v"])
                    if i:
                        plot(x0 + tx, y, rgb(v.colour(plane, e["pal"], i)), depth)

    for s in v.sprites():
        if not s["prio"]:
            continue
        x, y = (s["x"] + r["spr_ofs_x"]), (s["y"] + r["spr_ofs_y"])
        x = x - 256 if 248 < x < 256 else x & 255
        y = y - 256 if 248 < y < 256 else y & 255
        for ty in range(8):
            for tx in range(8):
                i = v.tile_px(s["tile"], tx, ty, s["h"], s["v"])
                if i:
                    # first-drawn (lower index) wins at equal depth: plot only if strictly deeper
                    plot(x + tx, y + ty, rgb(v.colour(0, s["pal"], i)), s["prio"] * 2)

    img = Image.new("RGB", (W, H))
    img.putdata([c for row in out for c in row])
    return img


def tile_usage(v):
    """tile -> (bank, palette) of first use: scroll planes first, then visible sprites."""
    use = {}
    for plane in (1, 2):
        for row in range(32):
            for col in range(32):
                e = v.map_entry(plane, col, row)
                use.setdefault(e["tile"], (plane, e["pal"]))
    for s in v.sprites():
        if s["prio"]:
            use.setdefault(s["tile"], (0, s["pal"]))
    return use


def render_tiles(v, use):
    img = Image.new("RGB", (32 * 9, 16 * 9), (40, 40, 40))
    px = img.load()
    grey = [(0, 0, 0), (85, 85, 85), (170, 170, 170), (255, 255, 255)]
    for t in range(512):
        ox, oy = (t % 32) * 9, (t // 32) * 9
        for y in range(8):
            for x in range(8):
                i = v.tile_px(t, x, y)
                if t in use:
                    c = rgb(v.colour(use[t][0], use[t][1], i)) if i else (255, 0, 255)
                else:
                    c = grey[i]
                px[ox + x, oy + y] = c
    return img.resize((img.width * 2, img.height * 2), Image.NEAREST)


def compare(screen, ppm_path):
    ref = Image.open(ppm_path).convert("RGB")
    if ref.size != screen.size:
        return {"ok": False, "reason": "size %s vs %s" % (ref.size, screen.size)}
    a, b = list(zip(*[iter(screen.tobytes())] * 3)), list(zip(*[iter(ref.tobytes())] * 3))
    # emulator output passes through RGB565/555; compare at 4 bits per channel
    bad = sum(1 for p, q in zip(a, b) if any(abs(x - y) > 12 for x, y in zip(p, q)))
    return {"ok": bad == 0, "mismatched_pixels": bad, "total": len(a)}


def rom_check(v, use, rom):
    found = missing = 0
    for t in sorted(use):
        data = v.d[0xA000 - BASE + t * 16: 0xA000 - BASE + t * 16 + 16]
        if data.count(0) == 16:
            continue
        if rom.find(data) >= 0:
            found += 1
        else:
            missing += 1
    return {"in_use_nonblank_tiles": found + missing, "found_verbatim_in_rom": found}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("vram", nargs="+", help="NAME.vram files from ngpc-trace")
    ap.add_argument("--out", help="output directory (default: next to each dump)")
    ap.add_argument("--rom", help="cartridge ROM, to check whether VRAM tiles are stored uncompressed")
    args = ap.parse_args()
    rom = open(args.rom, "rb").read() if args.rom else None
    ok_all = True
    for path in args.vram:
        stem = os.path.splitext(os.path.basename(path))[0]
        outdir = args.out or os.path.dirname(path) or "."
        os.makedirs(outdir, exist_ok=True)
        v = Vram(open(path, "rb").read())
        use = tile_usage(v)
        screen = render_screen(v)
        screen.save(os.path.join(outdir, stem + "_screen.png"))
        render_plane(v, 1).save(os.path.join(outdir, stem + "_scr1.png"))
        render_plane(v, 2).save(os.path.join(outdir, stem + "_scr2.png"))
        render_tiles(v, use).save(os.path.join(outdir, stem + "_tiles.png"))
        info = {
            "regs": v.regs,
            "palettes": {name: [[v.colour(bank, p, i) for i in range(4)] for p in range(16)]
                         for bank, name in ((0, "sprite"), (1, "scr1"), (2, "scr2"))},
            "bg_colour": v.bg_colour(), "window_colour": v.window_colour(),
            "sprites": [s for s in v.sprites() if s["prio"]],
            "tiles_used": len(use),
        }
        line = "%s: %d tiles referenced, %d visible sprites" % (stem, len(use), len(info["sprites"]))
        ppm = os.path.splitext(path)[0] + ".ppm"
        if os.path.exists(ppm):
            info["check"] = compare(screen, ppm)
            line += ", recomposition %s (%d/%d px differ)" % (
                "MATCHES emulator" if info["check"].get("ok") else "DIFFERS from emulator",
                info["check"].get("mismatched_pixels", -1), info["check"].get("total", 0))
            ok_all &= info["check"].get("ok", False)
        if rom is not None:
            info["rom"] = rom_check(v, use, rom)
            line += ", %d/%d non-blank tiles verbatim in ROM" % (
                info["rom"]["found_verbatim_in_rom"], info["rom"]["in_use_nonblank_tiles"])
        json.dump(info, open(os.path.join(outdir, stem + ".json"), "w"), indent=1)
        print(line)
    return 0 if ok_all else 1


if __name__ == "__main__":
    sys.exit(main())
