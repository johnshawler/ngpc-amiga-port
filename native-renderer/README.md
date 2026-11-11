# Native tile-to-Amiga bitplane routine

This is a new, deliberately bounded graphics primitive for a native game
reimplementation, not a recovered runtime or an NGPC emulator. It needs no ROM,
CPU interpreter, heap, operating-system calls, floating point or external runtime
symbols. It currently has no game-loop integration or display driver.

## Implemented contract

- NGPC 8x8, packed 2-bit tiles: each row is a little-endian word; bits 15:14
  are its leftmost texel. A tile occupies 16 bytes, 512 tiles occupy 8192 bytes.
- Descriptor bits 0..8 select tile, 9..12 select one of 16 palettes, 14 flips
  vertically, 15 flips horizontally. Bit 13 is ignored by this primitive.
- Texel zero is transparent, independently of the remapping for palette entry
  zero. Opaque texels can map to Amiga pen zero and clear existing plane bits.
- Caller-supplied 64-byte pen map converts one NGPC palette bank to any of 256
  AGA pens. A separate helper converts NGPC 12-bit BGR to Amiga 12-bit RGB.
  Hardware palette programming and allocation of pens across banks are not done.
- Eight separated Amiga bitplanes, lowest pen bit in plane zero. Each row starts
  on an even byte stride; bit 7 of the first byte is the leftmost pixel. Explicit
  byte writes produce the same Chip RAM layout on little- and big-endian CPUs.
- Signed placement, clipping at all four edges, partial last bytes, transparency
  and masked merge preserve the rest of the destination and row padding.
- Whole-tile rejection occurs before addition, including INT_MIN/INT_MAX inputs.
  All descriptor values are valid. Invalid buffer geometry/pointers/lengths
  return -1 without writes. Caller must supply correctly sized, disjoint arrays.

A 160x152 viewport at eight planes and stride 20 uses 24,320 bytes of Chip RAM;
320x256 at stride 40 uses 81,920 bytes. Tile and pen inputs use 8,256 bytes. The
routine allocates no framebuffer or cache. These are buffer-size calculations,
not measured total-game memory requirements. The caller must allocate actual
bitplanes in Chip RAM, configure AGA/DMA and arbitrate ownership while drawing.
Stock 2 MiB Chip and no Fast RAM remains the target; no Fast RAM is assumed.

This first implementation favors explicit, verifiable masks. It is not an
optimized blitter/assembly path, and is not evidence of 50 Hz or 100% game speed.

## Reproduction

From the repository root:

```sh
make -C native-renderer test
make -C native-renderer sanitize
python3 -m unittest discover -s tests -p 'test_native_renderer*' -v
# With an existing GCC/Linux m68k cross toolchain and qemu-user on PATH:
make -C native-renderer m68020
```

The sanitizer target uses address/undefined-behavior checks with leak checking
turned off because this execution environment is ptraced and LeakSanitizer
cannot run there. Neither renderer nor fixture allocates heap memory itself.

The C suite checks all 65,536 packed row values with both horizontal directions
against an independent byte/pair oracle (131,072 row cases); all 65,536 descriptor
values through the integrated renderer; 4,000 additional clipped 159/160x152
viewport draws; and all 65,536 input values to the RGB12 converter. Synthetic
pen maps include nonzero transparent mappings and opaque pen zero. Whole output
buffers, guards and padded rows are compared after every tile. Invalid arguments
and extreme coordinates are checked. The fixture contains no commercial data.

`m68020` additionally compiles a freestanding soft-float renderer object and
requires its undefined-symbol list to be empty. Its static Linux test executable
runs with `qemu-m68k -cpu m68020`. That executable is **not an
Amiga executable**. QEMU user mode proves instruction/endianness behavior, not
Chip RAM placement, DMA, PAL timing, Amiga bootability or hardware throughput.

## Reference and remaining work

Semantics were checked by reading the pinned upstream Beetle NGP source at
`a50d5ac288a81f2104ddf43195a4efdd15c72227`, specifically
`mednafen/ngp/gfx.c`: `drawColourPattern`, `draw_colour_scroll1`,
`draw_colour_scroll2`, and the color LUT initialization. See
<https://github.com/libretro/beetle-ngp-libretro/blob/a50d5ac288a81f2104ddf43195a4efdd15c72227/mednafen/ngp/gfx.c>.
The implementation and synthetic oracle are new code, not copied core routines;
they are marked GPL-2.0-or-later, compatible with that reference. The tests do
not claim to run the upstream renderer or compare actual game frames.

Caller integration still must resolve sprite positions/wrap, scroll maps,
window rectangles, z-order and same-depth ties, all palette banks, negative
color mode, display scaling, and per-scanline changes. This primitive implements
none of those implicitly. Callers must present tiles in already resolved draw
order; it does not implement a z-buffer. A frame-boundary GPU snapshot from the
NGR1 reference exporter is not a raster history and cannot prove pixel parity
when the game changes registers, palettes or tile data during active display.

Target-validation blocker: no Amiga-side harness/display integration has been
implemented for this new primitive, so it has not been booted under the stock
PAL A1200 configuration or timed against the game reference. The next checkpoint
needs that harness and a verified palette/order adapter, then actual Chip-RAM
rendering validation. Linux cross execution alone cannot close that gap.
