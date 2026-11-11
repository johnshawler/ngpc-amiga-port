/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_GOLFER_FEATURES_H
#define NTM_SHOT_GOLFER_FEATURES_H
#include "shot_aim.h"
typedef struct {
    uint16_t origin_x,origin_y,extent;
    uint32_t lateral_bias,depth_bias,lateral_scratch,depth_scratch;
    uint8_t nearby[81],view[81];
} ntm_shot_golfer_features;
typedef struct {
    const uint8_t *features;size_t features_size;
    const ntm_shot_aim_resources *aim;
} ntm_shot_golfer_feature_resources;
/* Exact21081d:2100ca rectangle collection,21082a signed direction transform
 * and view filtering,21098c nearest-feature selection. Borrow the current
 * hole's genuine id/x/y records (five bytes each, byte255 terminator).
 * Preserves stale scratch/list bytes, including an empty list's trailing bytes.
 * Validates the entire plan before mutation. Does not draw or advance a task. */
int ntm_shot_golfer_features_select(ntm_shot_golfer_features *,
    const ntm_hole_prepare_state *,const ntm_shot_golfer_feature_resources *);
#endif
