/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NATIVE_TILE_RENDERER_H
#define NATIVE_TILE_RENDERER_H
#include <stddef.h>
#include <stdint.h>

/* Separate contiguous planes, plane 0 = least significant pen bit.
 * stride is even (Amiga DMA word rows), width <= stride*8. Caller owns Chip
 * allocation and display setup. No allocation or hardware register access here.
 * All input and output arrays must be disjoint. */
typedef struct {
    uint8_t *data;
    size_t capacity;
    unsigned width, height, stride;
} nr_bitmap;

/* row_le is two bytes, low byte first. Output bit 7 is the leftmost pixel.
 * zero texels are transparent even if pens[0] is nonzero. */
void nr_decode_row(const uint8_t row_le[2], int hflip,
                   const uint8_t pens[4], uint8_t planes[8], uint8_t *opaque);

/* Draw one NGPC descriptor: tile bits 0..8, palette 9..12, V/H flip 14/15.
 * Bit 13 is ignored. tiles is 512*16 bytes; pens is 16*4 hardware pen numbers.
 * x,y are signed SCREEN coordinates, not NGPC wrapped sprite coordinates.
 * Returns 0 for success (including wholly clipped), -1 for invalid arguments.
 * Target dimensions are bounded to 320x256; no writes occur on failure.
 * No z-buffer: caller must submit in resolved back-to-front order. */
int nr_draw_tile(nr_bitmap *dst, const uint8_t *tiles, size_t tiles_size,
                 uint16_t descriptor, const uint8_t pens[64], int x, int y);

/* NGPC 0x0BGR to Amiga COLORxx 0x0RGB, ignoring reserved high bits. */
uint16_t nr_amiga_rgb12(uint16_t ngpc_bgr12);
#endif
