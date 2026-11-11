/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_ACCURACY_H
#define NTM_SHOT_ACCURACY_H
#include "shot_scene_objects.h"
#include "task_lifecycle.h"
typedef struct {
    ntm_task_callback wait;
    uint16_t position,velocity;
    uint8_t upper,lower,phase;
} ntm_shot_accuracy;
/* Original20bc19, no new resources. Scratch+16/+18 words,+1a/+1b bytes. */
int ntm_shot_accuracy_initialize(ntm_shot_accuracy *,ntm_task *,ntm_shot_scene_object_context *);
/* Original20bc5b..20bd3e. lifecycle4 commits hit classification and performs
 * genuine task finish. Otherwise advances oscillator and object7 marker.
 * held is the current input byte addressed by active player+31, not edge input.
 * label is41dd, distinct from club41dc. Does not initiate swing or ball flight. */
int ntm_shot_accuracy_tick(ntm_shot_accuracy *,ntm_shot_scene_object_context *,
    uint8_t lifecycle,uint8_t label,uint8_t held,ntm_task_context *,ntm_task *);
#endif
