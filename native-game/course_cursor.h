/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_COURSE_CURSOR_H
#define NTM_COURSE_CURSOR_H
#include "course_initialization.h"
typedef struct {const uint8_t *data;size_t size;uint8_t frames;} ntm_cursor_animation;
typedef struct {
    ntm_cursor_animation animations[2]; /* record word10=0 or4 */
    const uint8_t *shapes,*tiles;size_t shapes_size,tiles_size;
} ntm_cursor_assets;
typedef struct {uint16_t shape_offset,tile;uint8_t count;} ntm_cursor_tile_job;
typedef struct {
    uint8_t frame,delay,list_dirty_high,uploaded_bank;
    uint16_t bounds[4];
    uint8_t bank1[2][256],palettes[2][2][64];
    ntm_cursor_tile_job jobs[64];
} ntm_course_cursor_engine;
/* One active registered cursor only; normal/accept animation selectors0/4,
 * single-component noncollision frames, color sprites. Supports signed offsets,
 * clipping, flips, hide/freeze/restart, forced tile refresh and removal.
 * Bank0 sprite arrays are display.pools: display reset and cursor share storage.
 * Resources are borrowed, already resolved spans; embedded pointers are ignored.
 * Tail prepares buffers/jobs; head applies them. Neither function advances input
 * or assumes a display rate. Caller preserves original head/tail enable flags. */
int ntm_course_cursor_tail(ntm_course_cursor_engine *engine,ntm_course_cursor *cursor,
    ntm_display_reset_state *display,uint16_t x,const uint16_t scroll[2],
    uint8_t *global_flags,const ntm_cursor_assets *assets);
int ntm_course_cursor_head(ntm_course_cursor_engine *engine,ntm_display_reset_state *display,
    uint8_t tiles[8192],uint8_t sprites[256],uint8_t palettes[64],const ntm_cursor_assets *assets);
#endif
