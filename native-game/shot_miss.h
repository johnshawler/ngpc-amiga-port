/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_MISS_H
#define NTM_SHOT_MISS_H
#include "shot_scene_objects.h"
typedef struct {ntm_screen_asset notice;} ntm_shot_miss_resources;
typedef struct {ntm_task_callback delay,continuation;uint8_t phase;} ntm_shot_miss;
/* Exact210a3b. Change slot5.byte15 only when it is3 and the wrapped unsigned
 * x-minus-scroll-minus-left lies within width, inclusively. */
int ntm_shot_miss_object_gate(ntm_shot_scene_object_context *,uint16_t left,uint16_t width);
/* Exact2109d5: miss_flag is original413b, not the swing overlay. Queues the
 * notice, copies terrain, then executes the genuine first delay tick. */
int ntm_shot_miss_initialize(ntm_shot_miss *,ntm_task *,ntm_shot_scene_object_context *,
    uint8_t *miss_flag,const ntm_shot_miss_resources *);
/* Exact210a27. Returns0 while waiting,2 at the partial-update boundary before
 * shared210b74. Installs original210a38 continuation but does not fake its
 * nested task, scoring effects, owner result or completion. */
int ntm_shot_miss_tick(ntm_shot_miss *,ntm_task *);
#endif
