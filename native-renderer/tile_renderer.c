/* SPDX-License-Identifier: GPL-2.0-or-later
 * New portable implementation of documented tile semantics; no core code copied.
 */
#include "tile_renderer.h"

void nr_decode_row(const uint8_t row_le[2], int hflip,
                   const uint8_t pens[4], uint8_t planes[8], uint8_t *opaque)
{
    unsigned word = (unsigned)row_le[0] | ((unsigned)row_le[1] << 8);
    unsigned i, p;
    *opaque = 0;
    for (p = 0; p < 8; ++p) planes[p] = 0;
    for (i = 0; i < 8; ++i) {
        unsigned texel = (word >> (hflip ? 2*i : 14-2*i)) & 3;
        unsigned mask = 128u >> i;
        if (!texel) continue;
        *opaque |= (uint8_t)mask;
        for (p = 0; p < 8; ++p)
            if (pens[texel] & (1u << p)) planes[p] |= (uint8_t)mask;
    }
}

int nr_draw_tile(nr_bitmap *dst, const uint8_t *tiles, size_t tiles_size,
                 uint16_t descriptor, const uint8_t pens[64], int x, int y)
{
    size_t plane_size;
    unsigned row;
    if (!dst || !dst->data || !tiles || !pens || tiles_size < 8192 ||
        !dst->width || dst->width > 320 || !dst->height || dst->height > 256 ||
        !dst->stride || dst->stride > 4096 || (dst->stride & 1) ||
        dst->width > dst->stride*8) return -1;
    plane_size = (size_t)dst->stride * dst->height;
    if (dst->capacity < plane_size * 8) return -1;
    /* Reject before x/y arithmetic, including extreme int values. */
    if (x <= -8 || y <= -8 || x >= (int)dst->width || y >= (int)dst->height)
        return 0;
    for (row = 0; row < 8; ++row) {
        int sy = y + (int)row;
        unsigned sr = (descriptor & 0x4000) ? 7-row : row;
        uint8_t bits[8], opaque;
        unsigned i, p;
        /* Two destination bytes suffice for an eight-pixel row. Build masks
         * first, then merge each plane byte, retaining transparent pixels. */
        uint8_t masks[2] = {0, 0}, values[8][2] = {{0}};
        unsigned first = x < 0 ? 0 : (unsigned)x / 8;
        if (sy < 0 || sy >= (int)dst->height) continue;
        nr_decode_row(tiles + (descriptor & 511)*16 + sr*2,
                      descriptor & 0x8000, pens + ((descriptor >> 9)&15)*4,
                      bits, &opaque);
        for (i = 0; i < 8; ++i) {
            int sx = x + (int)i;
            unsigned slot, mask;
            if (sx < 0 || sx >= (int)dst->width || !(opaque & (128u >> i))) continue;
            slot = (unsigned)sx/8 - first;
            mask = 128u >> ((unsigned)sx & 7);
            masks[slot] |= (uint8_t)mask;
            for (p = 0; p < 8; ++p)
                if (bits[p] & (128u >> i)) values[p][slot] |= (uint8_t)mask;
        }
        for (p = 0; p < 8; ++p) {
            size_t offset = p*plane_size + (size_t)sy*dst->stride + first;
            for (i = 0; i < 2; ++i) if (masks[i])
                dst->data[offset+i] = (uint8_t)((dst->data[offset+i] & ~masks[i]) | values[p][i]);
        }
    }
    return 0;
}

uint16_t nr_amiga_rgb12(uint16_t c)
{
    return (uint16_t)(((c & 15) << 8) | (c & 0xf0) | ((c >> 8) & 15));
}
