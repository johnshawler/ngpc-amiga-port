/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TEE_PLANAR_CACHE_H
#define TEE_PLANAR_CACHE_H
#include "selector_planar_cache.h"
/* Identical keys to the legacy cache, lossless PackBits pictures kept separately.
 * All fifteen keys remain; general rendering handles every mismatch. */
typedef struct __attribute__((aligned(4))) {
    uint8_t maps[4096],sprites[256],palettes[64],controls[16];
    uint8_t referenced[SELECTOR_TILE_FLAGS],tiles[8192];
    const uint8_t *packed;uint32_t packed_size;
} tee_planar_entry;
int tee_planar_find(const nr_scene *,const tee_planar_entry *,unsigned count,
    unsigned hint,selector_planar_stats *,void (*pump)(void));
/* Entire stream validates before output writes. No-op128 is intentionally
 * rejected. Output exactly24320bytes; callback gaps <=639 decoded bytes in
 * each validation/copy pass. Borrowed source may not alias or change output. */
int tee_planar_unpack(const tee_planar_entry *,uint8_t output[SELECTOR_PLANAR_BYTES],void (*pump)(void));
#endif
