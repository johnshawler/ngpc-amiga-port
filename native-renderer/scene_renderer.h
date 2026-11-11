/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NR_SCENE_RENDERER_H
#define NR_SCENE_RENDERER_H
#include "tile_renderer.h"
/* Latched color-mode state, not raw register bytes. Arrays are fixed-size,
 * disjoint from output, and owned by caller. Pens index an AGA 256-color bank:
 * sprite 0..63, scroll1 64..127, scroll2 128..191, background 240..247,
 * window 248..255. Caller loads all 256 source colors, including unused ones.
 * Palette inversion belongs to the Copper/color adapter, not this compositor.
 */
typedef struct {
    const uint8_t *tiles;       /* 8192 bytes */
    const uint8_t *maps;        /* 4096 bytes: two LE 32x32 descriptor maps */
    const uint8_t *sprites;     /* 256 bytes: 64 LE descriptor/x/y entries */
    const uint8_t *sprite_pal;  /* 64 bytes */
    uint8_t scroll_x[2], scroll_y[2], sprite_x, sprite_y;
    uint8_t window_x, window_y, window_w, window_h;
    uint8_t swap, background, outside;
} nr_scene;
/* Produce one 160-pixel native row. Output is untouched for invalid arguments.
 * Call with the state latched for THIS row; frame-end snapshots cannot establish
 * raster parity. All layers enabled; color mode only, display active.
 * No allocations, depth buffer, CPU emulation or ROM access.
 */
int nr_scene_row(const nr_scene *scene, unsigned y, uint8_t out[160]);
/* Optional caller-owned cache: initialize with nr_scene_cache_init. Cache is
 * private to one rendering stream. Content comparison invalidates automatically
 * after ANY sprite descriptor, position, palette or global offset change.
 * Tile/maps/window/scroll are read live on each row. No frame-latch assumption. */
typedef struct {
    uint16_t offsets[457]; /* 152 rows x 3 priorities, plus terminal offset */
    uint8_t entries[512];  /* each of 64 sprites occupies at most eight rows */
    int16_t x[64], y[64];
    uint16_t desc[64];
    uint8_t raw[320], sx, sy, valid;
    uint32_t rebuilds, hits;
} nr_scene_cache;
void nr_scene_cache_init(nr_scene_cache *cache);
int nr_scene_row_cached(const nr_scene *scene, unsigned y, uint8_t out[160],
                        nr_scene_cache *cache);

/* Store one pen row in a 160x152 eight-plane Amiga bitmap, stride >=20/even.
 * Other rows and stride padding are untouched. */
int nr_store_scene_row(nr_bitmap *dst, unsigned y, const uint8_t pens[160]);
/* Native 32-bit packed transpose; same contract as scalar store above. */
int nr_store_scene_row_packed(nr_bitmap *dst,unsigned y,const uint8_t pens[160]);
/* Optional dirty 8-pixel blocks. Cache owns a shadow of ONE bitmap; do not alter
 * its destination pixels externally without reinitializing the cache. Palette
 * writes need no invalidation because stored pixels are palette indices.
 * Pointer/stride changes invalidate automatically. All arrays must be disjoint. */
typedef struct {
    uint8_t pens[24320];
    uint32_t valid[152];
    uint8_t *owner;
    unsigned stride;
    uint32_t converted_blocks,skipped_blocks;
} nr_planar_cache;
void nr_planar_cache_init(nr_planar_cache *cache);
int nr_store_scene_row_dirty(nr_bitmap *dst,unsigned y,const uint8_t pens[160],nr_planar_cache *cache);
#endif
