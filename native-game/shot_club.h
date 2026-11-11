/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_CLUB_H
#define NTM_SHOT_CLUB_H
#include "shot_scene_objects.h"
typedef struct {const uint8_t *data;size_t size;} ntm_shot_club_table;
typedef struct {
    const ntm_shot_aim_resources *aim;
    const ntm_shot_scene_object_resources *scene;
    ntm_shot_club_table powers[12],lies[12][15],overflow_lies[12][15]; /*56,42,42 bytes; overflow is original lie base+0300. */
    ntm_screen_asset labels[23],digits[11],lie_icons[19];
} ntm_shot_club_resources;
typedef struct {
    ntm_task_callback wait;
    uint8_t maximum,minimum,label,delta,restore_delay,change_flag,phase;
} ntm_shot_club;
/*20ace7 initialization,20d984/20d9a3 bounds and20adcb redraw. The later
 * interactive change/highlight path has an explicit unsupported return2. */
int ntm_shot_club_initialize(ntm_shot_club *,ntm_task *,
    ntm_shot_scene_object_context *,const ntm_shot_club_resources *);
int ntm_shot_club_redraw(ntm_shot_club *,ntm_shot_scene_object_context *,
    const ntm_shot_club_resources *);
/*20ad1a's genuine early branch before4416 is set. Returns2 before consuming
 * a still-unconverted active control path; never completes the child. */
int ntm_shot_club_tick(ntm_shot_club *,ntm_shot_scene_object_context *);
#endif
