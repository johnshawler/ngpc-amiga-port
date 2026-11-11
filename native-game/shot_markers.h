/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_MARKERS_H
#define NTM_SHOT_MARKERS_H
#include "shot_scene_objects.h"
typedef struct {const uint8_t *quarter;size_t quarter_size;const uint8_t *metadata;size_t metadata_size;} ntm_shot_marker_resources;
typedef struct {
    ntm_tee_setup_context *shared;ntm_hole_prepare_state *hole;ntm_shot_aim *aim;
    uint8_t *dirty; /* Original41da, shared with club control. */
    uint8_t held,player_status;
} ntm_shot_marker_context;
typedef struct {ntm_task_callback tick,waiting;uint8_t step,phase;uint16_t limit;} ntm_shot_markers;
/* Original20d1e9 signed fixed16 multiply and20d633 quarter-wave lookup, no
 * floating point. The multiply retains the original small-operands rounding
 * exception: a negative product of two magnitudes<65536 truncates toward0. */
uint32_t ntm_shot_marker_multiply(uint32_t,uint32_t);
uint32_t ntm_shot_marker_direction(const uint8_t quarter[4100],uint16_t angle);
/* Genuine20aee1 initialization and20af8a ticks. State1 transitions to waiting;
 * unconverted non-idle20afa5 action returns2 before invoking that action. */
int ntm_shot_markers_initialize(ntm_shot_markers *,ntm_task *,ntm_shot_marker_context *,const ntm_shot_marker_resources *);
int ntm_shot_markers_tick(ntm_shot_markers *,ntm_task *,ntm_shot_marker_context *,const ntm_shot_marker_resources *);
/* Original20b03b normal (status0) positions both marker objects. Other camera
 * mode explicitly returns2 and does not fabricate a result. */
int ntm_shot_markers_position(ntm_shot_marker_context *,const ntm_shot_marker_resources *);
/* Exact initial delayed child slices. Initializers fall through to the first
 * decrement (hint30->29, camera8->7). A nonnegative decremented word waits,
 * including0;0->65535 reaches the first active callback and returns2,
 * which remains unconverted. Zero hint input_mode instead performs the
 * genuine20132b completion, including priority deferral and free-list links. */
typedef struct {ntm_task_callback delay,active;uint8_t repeat,last,pending,changed,phase;} ntm_shot_hint_delay;
int ntm_shot_hint_initialize(ntm_shot_hint_delay *,ntm_task *,ntm_task_context *,uint8_t input_mode);
int ntm_shot_hint_tick(ntm_shot_hint_delay *,ntm_task *);
typedef struct {ntm_task_callback delay,active;uint8_t phase;} ntm_shot_camera_delay;
int ntm_shot_camera_initialize(ntm_shot_camera_delay *,ntm_task *);
int ntm_shot_camera_tick(ntm_shot_camera_delay *,ntm_task *);
#endif
