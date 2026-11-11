# Native color scene composition

This is a new implementation, not recovered pre-loss golf code. It adds the
NGPC color drawing rules used by golf to the recovery's native graphics path:

- Two 256x256 scrolling tile maps, wrapped fine/coarse scroll, tile flips.
- All 64 sprites, relative coordinate chains (including disabled anchors),
  global sprite offsets and clipped/wrapped edges.
- Three sprite priorities interleaved with the two swappable planes.
- Lower-index sprite precedence, transparent texels and rectangular window.
- Direct 8-plane Amiga row output with no heap allocation or depth buffer.

The compositor uses resolved painter order. Its independent synthetic test uses
per-destination-pixel map lookups and a depth buffer with ascending sprite order.
It compares 4,000 seeded randomized rows plus 8,192 directed overlap/chain/wrap
rows. Planar output checks every byte including other rows and stride padding.
Invalid-input checks establish no output mutation. Source semantics were checked
against the pinned GPL2+ NeoPop/Mednafen gfx.c, not against commercial captures.

Run `make scene scene-sanitize scene-m68020` in this directory. The m68020 test
runs a Linux test executable under QEMU and checks that the freestanding native
object has no undefined symbols. This is CPU compatibility, not Amiga timing.
The Python unittest suite includes the host scene test without requiring ROMs.

## Scope and limitations

The API accepts already-latched color state per scanline. Its 256 pen numbers
map directly to the 256 source color entries; an AGA palette adapter must load
those colors and handle inversion/raster changes. No frame-end dump is assumed
to reproduce prior raster state. Display blanking, monochrome, bus contention,
scanline state capture, original-game pixel parity and dynamic Amiga display
integration remain unverified. All layers are enabled.

The pre-existing synthetic stock PAL Amiga fixture is unchanged. This new
compositor has not yet been integrated into that executable or measured on its
68020/AGA target. No speed, gameplay, audio or playable-golf claim follows.
