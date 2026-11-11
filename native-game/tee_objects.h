/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_TEE_OBJECTS_H
#define NTM_TEE_OBJECTS_H
#include "course_cursor.h"
#define NTM_TEE_OBJECT_LIMIT 32
#define NTM_TEE_ANIMATION_LIMIT 40
/* Borrowed, resolved spans. Embedded cartridge pointers are not followed. */
typedef struct {
    ntm_cursor_animation animations[NTM_TEE_ANIMATION_LIMIT];
    const uint8_t *shapes,*tiles;
    size_t shapes_size,tiles_size;
} ntm_tee_object_assets;
typedef struct {
    const uint8_t *shapes,*tiles;
    size_t shapes_size,tiles_size;
    uint16_t shape_offset,tile;
    uint8_t count;
} ntm_tee_tile_job;
typedef struct {
    uint8_t frame[32],delay[32],list_dirty_high,uploaded_bank;
    uint16_t bounds[32][4];
    uint8_t bank1[2][256],palettes[2][2][64];
    ntm_tee_tile_job jobs[64];
} ntm_tee_object_engine;
/* Original2007ca append:0 appended,1 full,-1 invalid contract. Does not reset
 * frame/delay/bounds, other records, or display buffers. Caller resets display
 * before setup and keeps record IDs/tags synchronized with that display. */
int ntm_tee_object_register(ntm_display_reset_state *display,ntm_course_cursor *object);
/* Original200824:0 removed,1 absent,-1 unsupported/invalid. Unlike the
 * traversal's removal pass, explicit removal does not dirty/sort the list. */
int ntm_tee_object_remove(ntm_display_reset_state *display,ntm_course_cursor *object);
/* Display reset with only record ID/tag synchronization; all other record and
 * engine state is deliberately retained, just as original2006cf. */
int ntm_tee_objects_reset(ntm_display_reset_state *display,ntm_course_cursor objects[32]);
/* Original201192 then200c7b. Supports color-mode tee tags29..32/index52 first-tee tag24/index56 and shot tag5/indices68,72,76,
 * their bounded animation selections, at most two noncollision components per
 * frame, empty sentinel shapes, flips, clipping, freeze/restart/removal.
 * Hide is supported for single-component frames only; hidden multi-component
 * frames follow a different original parser and are rejected.
 * Arrays use original object IDs0..31; only listed objects require resources.
 * All validation precedes mutation. Frame-tail queues tile jobs with their own
 * borrowed resource identity; resources remain alive until the matching head.
 * Does not advance the scheduler, input or display rate. */
int ntm_tee_objects_tail(ntm_tee_object_engine *engine,ntm_course_cursor objects[32],
    ntm_display_reset_state *display,const uint16_t x[32],const uint16_t scroll[2],
    uint8_t *global_flags,const ntm_tee_object_assets *const assets[32]);
int ntm_tee_objects_head(ntm_tee_object_engine *engine,ntm_display_reset_state *display,
    uint8_t tiles[8192],uint8_t sprites[256],uint8_t palettes[64]);
#endif
