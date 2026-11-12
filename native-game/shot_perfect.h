/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_PERFECT_H
#define NTM_SHOT_PERFECT_H
#include "shot_scene_objects.h"
typedef struct {ntm_palette_job palettes[2];ntm_screen_asset player_art[6],overlay;} ntm_shot_perfect_resources;
typedef struct {ntm_task_callback first_delay,visible_delay,finish;uint16_t duration;uint8_t kind,phase;} ntm_shot_perfect;
/* Original20c64e, real queued palettes/art and same-call first countdown. */
int ntm_shot_perfect_initialize(ntm_shot_perfect *,ntm_task *,ntm_shot_scene_object_context *,const ntm_shot_perfect_resources *);
/* Original20c6f6/20c707/20c72f/20c740. Returns genuine ntm_task_finish outcome
 * when the displayed notice has actually expired, never sets owner result. */
int ntm_shot_perfect_tick(ntm_shot_perfect *,ntm_task *,ntm_shot_scene_object_context *);
#endif
