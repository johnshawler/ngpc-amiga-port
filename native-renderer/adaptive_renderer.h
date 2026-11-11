/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NR_ADAPTIVE_RENDERER_H
#define NR_ADAPTIVE_RENDERER_H
#include "fine_event_renderer.h"
/* Explicit-write contract is inherited from fine_event_renderer. One caller
 * per scene stream. Unknown rows start with the packed baseline. Live register changes select a dependency-free row draw;
 * two consecutive stable observations are required before caching resumes.
 * Return 1 means caller should use packed output and invalidate that row of any
 * separate dirty-planar shadow. Return 0 means normal fine-cache output. */
typedef struct {
    nr_fine_event_cache fine;
    uint8_t previous[152][13],seen[152],cooldown[152];
    uint32_t fallback_rows,cached_rows;
} nr_adaptive_cache;
void nr_adaptive_init(nr_adaptive_cache *c);
void nr_adaptive_invalidate_all(nr_adaptive_cache *c);
void nr_adaptive_write(nr_adaptive_cache *c,unsigned kind,unsigned offset);
int nr_scene_row_adaptive(const nr_scene *s,unsigned y,uint8_t out[160],nr_adaptive_cache *c);
#endif
