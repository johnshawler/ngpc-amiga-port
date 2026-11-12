/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_SWING_H
#define NTM_SHOT_SWING_H
#include "shot_scene_objects.h"
typedef struct {
    ntm_task_callback waiting,hit_delay,hit_reset;
    uint8_t phase,boundary;
} ntm_shot_swing;
typedef struct {
    ntm_shot_scene_object_context *scene;
    uint8_t *overlay;
    uint8_t suppress_sound; /* Original447e, not a host audio option. */
} ntm_shot_swing_context;
enum {NTM_SHOT_SWING_HIT=1,NTM_SHOT_SWING_MISS=2};
/* Exact209aa2: lifecycle4, clear real task result, overlay255, conditional
 * service0 command65532, install209ad3. Does not run/complete any child. */
int ntm_shot_swing_initialize(ntm_shot_swing *,ntm_task *,ntm_shot_swing_context *);
/* Exact209ad3/209af4. Wait for the real child result. Positive result freezes
 * slot16 and executes the genuine same-call delay4->3. Negative result stops
 * before209b4b miss continuation; expired hit delay stops before209b05.
 * Return2 is an explicit partial-update boundary; it cannot be resumed here.
 * Display reset, miss child, parent completion and flight are later work. */
int ntm_shot_swing_tick(ntm_shot_swing *,ntm_task *,ntm_shot_swing_context *);
#endif
