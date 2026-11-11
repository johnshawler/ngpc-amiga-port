/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_GOLFER_DRAWING_H
#define NTM_SHOT_GOLFER_DRAWING_H
#include "shot_golfer_animation.h"
#include "shot_golfer_features.h"
typedef struct {
    ntm_shot_golfer_features features;
    uint16_t center_x,half_width;
    uint8_t phase;
} ntm_shot_golfer_drawing;
typedef struct {
    ntm_shot_golfer_feature_resources features;
    const ntm_shot_golfer_animation_resources *animation;
    ntm_screen_asset rim,green[4];
    const uint8_t *eligible,*depth_records,*clip_records,*metadata;
    size_t eligible_size,depth_records_size,clip_records_size,metadata_size;
    /* Byte index of projected x0 in the bounded original clip-record span. */
    size_t clip_origin;
} ntm_shot_golfer_drawing_resources;
/* Exact20fa2f, including true signed division and original fractional multiply.
 * Input/output words are original wrapped bit patterns. No64-bit runtime. */
int ntm_shot_golfer_project_feature(uint16_t lateral,uint16_t depth,
    uint16_t *center,uint32_t *lateral_scratch);
/* Complete20a5aa..20a5c8 initial drawing callback. Executes actual20a9ce,
 *20a94a,20ab9f; installs the supplied real20a5c0 adapter and performs its
 * same-call animation tick. Does NOT invoke that function pointer directly.
 * Rims and green art draw immediately before real clipped row copies. Pending
 * stamp jobs remain pending until their genuine head. Tag52 is real slot18.
 * All required resource/geometry validation precedes output changes. Borrowed
 * asset bytes must not alias destination video/state. No scheduler/input pump. */
int ntm_shot_golfer_draw_initialize(ntm_shot_golfer_drawing *,ntm_shot_golfer *,
    ntm_task *,ntm_shot_scene_object_context *,const ntm_shot_golfer_drawing_resources *,
    ntm_task_callback animation);
#endif
