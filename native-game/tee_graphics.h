/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_TEE_GRAPHICS_H
#define NTM_TEE_GRAPHICS_H
#include "course_screen.h"
/* Original2004e2, full-count/full-index variant. A resolved header contains a
 * count, ignored embedded pointer, and little-endian byte offsets into tiles.
 * This copies indexed16-byte tiles only, without drawing a map or queuing work.
 * All source/destination ranges validate before mutation. */
int ntm_tee_copy_tiles(const ntm_screen_asset *,uint16_t offset,uint8_t tiles[8192]);
/* Original205119/205141,1..4decimal places, leading blanks except last place.
 * Scratch is the original first four bytes at5bcb represented as a LE value,
 * not host byte storage. Bytes beyond digits retain their previous values.
 * Formatting keeps the requested least-significant digits, even on overflow.
 * The high color byte and text base are added modulo65536 to digit+16. */
int ntm_tee_draw_number(uint16_t value,uint8_t digits,uint8_t color,
    uint16_t text_base,uint16_t offset,uint32_t *scratch,uint8_t maps[4096]);
#endif
