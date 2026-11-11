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
    ntm_screen_asset highlights[23]; /* Original20ae10, resolved group00ac. */
} ntm_shot_club_resources;
typedef struct {
    ntm_task_callback wait;
    uint8_t maximum,minimum,label,delta,restore_delay,change_flag,phase;
} ntm_shot_club;
/*20ace7 initialization,20d984/20d9a3 bounds and20adcb redraw, including
 * the genuine timed club highlight when restore_delay is nonzero. */
int ntm_shot_club_initialize(ntm_shot_club *,ntm_task *,
    ntm_shot_scene_object_context *,const ntm_shot_club_resources *);
int ntm_shot_club_redraw(ntm_shot_club *,ntm_shot_scene_object_context *,
    const ntm_shot_club_resources *);
/* Legacy bounded20ad1a entry. Returns2 when a resource/input-aware control
 * call is required; never completes the child. New callers use control below. */
int ntm_shot_club_tick(ntm_shot_club *,ntm_shot_scene_object_context *);
/* Full20ad1a club child. held/pressed are the active player's input bytes0/1,
 * not fresh host samples. Changes request original25001c sound0 through shared->sound_request
 * as(service2,command0,parameter0), separate from250018 selector effects. The
 * original signed reset_d1>=2 returns without changing timers or the club.
 * No swing callback, completion or input transition is fabricated here. */
int ntm_shot_club_control(ntm_shot_club *,ntm_shot_scene_object_context *,
    const ntm_shot_club_resources *,uint8_t held,uint8_t pressed);
#endif
