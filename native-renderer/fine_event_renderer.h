/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NR_FINE_EVENT_RENDERER_H
#define NR_FINE_EVENT_RENDERER_H
#include "scene_renderer.h"

/* Explicit-write cache with exact selected tile-row-word dependencies.
 * EVERY changed tile/map/sprite descriptor/sprite palette byte must be notified
 * before the next row is requested. Reset, load, bulk replacement, changing
 * input arrays or switching scene streams requires invalidate_all. The caller
 * owns correctly sized, disjoint arrays and serializes writes and row requests.
 * No pointer ownership inference or unannounced-write detection is provided.
 * RGB palette changes do not affect pens; sprite palette-index changes do.
 * Thirteen latched scene registers are compared on every row request, preserving
 * raster effects. Sprite descriptor writes invalidate all rows because disabled
 * sprites can anchor relative chains. Palette-index writes only invalidate rows
 * with a potentially visible opaque texel from that sprite. Occluded texels may
 * conservatively retain dependencies. Both bytes of a selected tile row count;
 * an unselected row of that tile does not. Transparency does NOT discard tile
 * or map dependencies: a subsequent write may make a texel visible.
 *
 * Dense selected-word bits make each tile write O(152), with no dependency-list
 * search. Each scroll plane selects one map row, so a row number and 32 column
 * bits replace the baseline's 2,048 map-entry bits. No allocation or hashes.
 * Public valid/regs retain event-cache layout semantics for adaptive callers.
 */
typedef struct {
    nr_scene_cache sprites;
    uint32_t tile_words[152][128]; /* 512 tiles x eight row words */
    uint32_t map_columns[152][2], sprite_palettes[152][2];
    uint8_t map_rows[152][2], regs[152][13];
    uint8_t pixels[152][160], valid[152];
    uint32_t composed, reused, notifications;
} nr_fine_event_cache;

void nr_fine_event_init(nr_fine_event_cache *cache);
/* Preserve top-level counters; discard every dependency and sprite preparation.
 * init also resets all counters. Null cache pointers are harmless. */
void nr_fine_event_invalidate_all(nr_fine_event_cache *cache);
/* kind 0: tile byte [0,8192); kind 1: map byte [0,4096);
 * kind 2: sprite descriptor/position byte [0,256);
 * kind 3: sprite palette-index byte [0,64), NOT an RGB color byte.
 * Invalid kind/offset conservatively invalidates all rows. Each call counts one
 * notification, including invalid arguments. Unchanged-byte notifications are
 * safe but may waste work. Every changed byte in a multi-byte write is notified.
 */
void nr_fine_event_write(nr_fine_event_cache *cache,unsigned kind,unsigned offset);
/* Same row/output and invalid-argument contract as nr_scene_row. */
int nr_scene_row_fine_event(const nr_scene *scene,unsigned y,uint8_t out[160],
                            nr_fine_event_cache *cache);
#endif
