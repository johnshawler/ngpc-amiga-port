/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_SWING_EXIT_H
#define NTM_SHOT_SWING_EXIT_H
#include "shot_scene_objects.h"
#include "tee_objects.h"
#include "task_allocation.h"
typedef struct {
    ntm_task_callback perfect_entry,perfect_wait,miss_entry,miss_wait;
    uint8_t phase;
} ntm_shot_swing_exit;
/* Genuine209b05/209b4b continuations: reset, descendant removal and allocation.
 * Children are allocated, never dispatched or completed artificially here.
 * Uses aim.reset_d3 (41d3) and aim.reset_e6 (41e6). Caller-owned links/resources.
 * Return values are real task lifecycle results; negative values reject input. */
int ntm_shot_swing_exit_hit(ntm_shot_swing_exit *,ntm_task *,ntm_shot_scene_object_context *);
int ntm_shot_swing_exit_miss(ntm_shot_swing_exit *,ntm_task *,ntm_shot_scene_object_context *);
int ntm_shot_swing_exit_tick(ntm_shot_swing_exit *,ntm_task *,ntm_shot_scene_object_context *);
#endif
