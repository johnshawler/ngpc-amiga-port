/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_TRAJECTORY_H
#define NTM_SHOT_TRAJECTORY_H
#include "shot_scene_objects.h"
typedef struct {
 const uint8_t *metadata,*sprite_bases,*hud_bases;
 size_t metadata_size,sprite_bases_size,hud_bases_size;
 ntm_palette_job palettes[6];
 ntm_screen_asset hud[6][3];
} ntm_shot_trajectory_resources;
typedef struct {
 ntm_task_callback wait;
 uint16_t hud_base,sprite_base,hud_delta;
 uint8_t counter,last_label,phase;
} ntm_shot_trajectory;
/*20b3aa, including20db22/20db62 and same-call20b432/20b49e.
 * label is original41dd, not the club index41dc. Resources are borrowed.
 * Tick returns2 before lifecycle4 enters real swing animation20b43f.
 * No lifecycle transition, scheduler completion or animation is fabricated. */
int ntm_shot_trajectory_initialize(ntm_shot_trajectory *,ntm_task *,
 ntm_shot_scene_object_context *,const ntm_shot_trajectory_resources *,uint8_t label);
int ntm_shot_trajectory_tick(ntm_shot_trajectory *,ntm_shot_scene_object_context *,
 const ntm_shot_trajectory_resources *,uint8_t label);
#endif
