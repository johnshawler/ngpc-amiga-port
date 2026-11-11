/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_COURSE_SCREEN_H
#define NTM_COURSE_SCREEN_H
#include <stddef.h>
#include <stdint.h>
/* Borrowed, caller-owned decoded asset span. No cartridge access or allocation. */
typedef struct {const uint8_t *data;size_t size;const uint8_t *tiles;size_t tiles_size;} ntm_screen_asset;
typedef struct {uint16_t offset,fill;uint8_t width,height;} ntm_screen_fill;
typedef struct {ntm_screen_asset asset;uint16_t offset,tile;} ntm_screen_stamp;
typedef struct {
    uint8_t fill_status[8];ntm_screen_fill fills[8];
    uint16_t stamp_bytes;ntm_screen_stamp stamps[32];
    uint16_t cached_selection,cursor_x;
    uint8_t displayed_selection;
} ntm_course_screen;
/* Valid selected courses are 0..2. Queues must already be initialized by their
 * owner (free fill slots have status255, empty stamp queue has stamp_bytes0).
 * Full queues silently drop their respective request, as in the original.
 * These producers do NOT change tilemap memory. Assets remain borrowed until
 * their queued request is consumed. Cache gate uses the complete 16-bit word. */
/* Shared bounded producers. Return0 queued,1 queue full,-1 invalid contract.
 * Initialization also uses these for background jobs; drains stay separate. */
int ntm_course_screen_enqueue_fill(ntm_course_screen *screen,uint16_t offset,
    uint8_t width,uint8_t height,uint16_t fill);
int ntm_course_screen_enqueue_stamp(ntm_course_screen *screen,const ntm_screen_asset *asset,
    uint16_t offset,uint16_t tile);
int ntm_course_screen_draw(ntm_course_screen *screen,uint16_t selection,
                          const ntm_screen_asset assets[3]);
int ntm_course_screen_tick(ntm_course_screen *screen,uint16_t selection,
                          const ntm_screen_asset assets[3]);
/* Bounded course assets: normal orientation, 16-bit sentinel map rows.
 * Source header pointer bytes are never dereferenced: tiles is resolved by the
 * asset loader. Writes target caller-owned native scene arrays. */
int ntm_course_screen_stamp(const ntm_screen_stamp *stamp,uint8_t tiles[8192],uint8_t maps[4096]);
/* Separate original scheduling boundaries: drain fills descending7..0 before
 * stamps FIFO. Nonzero dimensions/tile counts/rows and nonempty rows only.
 * All destination writes must fit the two native 32x32 maps and512tiles.
 * Each queue validates fully before mutation; malformed assets return-1.
 * Caller invalidates any dependent compositor caches after these writes. */
int ntm_course_screen_fill_queue(ntm_course_screen *screen,uint8_t maps[4096]);
int ntm_course_screen_stamp_queue(ntm_course_screen *screen,uint8_t tiles[8192],uint8_t maps[4096]);
#endif
