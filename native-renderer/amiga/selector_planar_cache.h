/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef SELECTOR_PLANAR_CACHE_H
#define SELECTOR_PLANAR_CACHE_H
#include "../scene_renderer.h"
#define SELECTOR_PLANAR_BYTES 24320u
#define SELECTOR_TILE_FLAGS 64u
/* All maps/sprite records and controls are exact. Only tile bytes referenced
 * by either complete map or any sprite record are compared. Unreferenced tiles
 * cannot be read by nr_scene_row, so navigation-history residue is irrelevant.
 * Palette RGB is intentionally separate: the bitmap contains pen indices. */
typedef struct __attribute__((aligned(4))) {
    uint8_t maps[4096],sprites[256],palettes[64],controls[16];
    uint8_t referenced[SELECTOR_TILE_FLAGS],tiles[8192];
    uint8_t planar[SELECTOR_PLANAR_BYTES];
} selector_planar_entry;
typedef struct {uint32_t searches,hits,misses,candidates,tile_bytes;} selector_planar_stats;
void selector_planar_key(const nr_scene *scene,selector_planar_entry *entry);
/* Returns exact matching entry index, or -1 without modifying scene/output.
 * Hints only alter search order. Pump callback may update a separate application,
 * but must not mutate the immutable scene snapshot or generated cache. */
int selector_planar_find(const nr_scene *scene,const selector_planar_entry *entries,
    unsigned count,unsigned hint,selector_planar_stats *stats,void (*pump)(void));
#endif
