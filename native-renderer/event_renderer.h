/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NR_EVENT_RENDERER_H
#define NR_EVENT_RENDERER_H
#include "scene_renderer.h"
/* Explicit-write cache. EVERY changed tile/map/sprite byte must be notified.
 * Reset/load/bulk replacement requires invalidate_all. No pointer ownership
 * inference: use one cache per scene stream. RGB palette changes do not affect
 * pens. Latched scene registers are compared per row, preserving raster effects.
 * Dependencies conservatively cover whole tiles, not only selected tile words. */
typedef struct {
    nr_scene_cache sprites;
    uint8_t tiles[152][64], maps[152][256], regs[152][13];
    uint8_t pixels[152][160], valid[152];
    uint32_t composed, reused, notifications;
} nr_event_cache;
void nr_event_init(nr_event_cache *c);
void nr_event_invalidate_all(nr_event_cache *c);
/* kind 0 tile byte [0,8192), 1 map byte [0,4096), 2 any sprite/palette byte.
 * Invalid kind/offset fails conservatively by invalidating all rows. */
void nr_event_write(nr_event_cache *c,unsigned kind,unsigned offset);
int nr_scene_row_event(const nr_scene *s,unsigned y,uint8_t out[160],nr_event_cache *c);
#endif
