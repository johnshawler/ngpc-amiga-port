/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_TRAJECTORY_SWING_H
#define NTM_SHOT_TRAJECTORY_SWING_H
#include "shot_trajectory.h"
typedef struct {
    const uint8_t *putter_frames,*terrain_sounds,*club_sounds,*hud_bases;
    size_t putter_frames_size,terrain_sounds_size,club_sounds_size,hud_bases_size;
    ntm_screen_asset hud[6][3][15];
} ntm_shot_trajectory_swing_resources;
typedef struct {
    ntm_task_callback animate;
    uint8_t timer,result,phase;
} ntm_shot_trajectory_swing;
/* Real20b43f entry and same-call20b47f/20b50b. The original trajectory counter,
 * HUD bases/delta and last_label bytes remain shared; last_label becomes reload.
 * suppress_sound is original447e. label is41dd, not club41dc. Sound callbacks
 * preserve service0/2 request identity; they do not synthesize audio. */
int ntm_shot_trajectory_swing_enter(ntm_shot_trajectory_swing *,ntm_shot_trajectory *,
    ntm_task_context *,ntm_task *,ntm_shot_scene_object_context *,
    const ntm_shot_trajectory_swing_resources *,uint8_t suppress_sound,uint8_t label);
/* Real20b47f tick: exact HUD frame/queue, hit or missed-result publication to
 * task->owner, and actual task finish/defer/recycle. Counter0..14 is the real
 * animation range; signed>=15 produces miss255. Unsupported negative counters
 * stop before mutation rather than addressing an unbounded asset table. */
int ntm_shot_trajectory_swing_tick(ntm_shot_trajectory_swing *,ntm_shot_trajectory *,
    ntm_task_context *,ntm_task *,ntm_shot_scene_object_context *,
    const ntm_shot_trajectory_swing_resources *,uint8_t suppress_sound,uint8_t label);
#endif
