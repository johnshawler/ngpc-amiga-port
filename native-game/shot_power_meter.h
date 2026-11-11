/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_POWER_METER_H
#define NTM_SHOT_POWER_METER_H
#include "shot_scene_objects.h"
#include "task_lifecycle.h"
typedef struct {
    const uint8_t *speed_player,*speed_lie;
    size_t speed_player_size,speed_lie_size; /* Signed index windows,257 bytes. */
    ntm_shot_scene_object_resources scale;
    ntm_screen_asset glyphs[9]; /* Table0x9c indices45..53, privately resolved. */
} ntm_shot_power_meter_resources;
typedef struct {
    ntm_task_callback wait;
    uint16_t position,velocity;
    uint8_t upper,lower,hold,phase;
} ntm_shot_power_meter;
/* Original20b70b initializer, installs caller's20b74b adapter. */
int ntm_shot_power_meter_initialize(ntm_shot_power_meter *,ntm_task *,
    ntm_shot_scene_object_context *,const ntm_shot_power_meter_resources *);
/* Original20b7c8 arithmetic, no drawing; signed-byte bounds, wrapped words. */
int ntm_shot_power_meter_advance(ntm_shot_power_meter *,ntm_hole_prepare_state *,
    const ntm_shot_player_state *,const ntm_shot_power_meter_resources *);
/* Original20b74b: lifecycle2 updates; all other values genuinely finish task.
 * Corrupt power >120 stops BEFORE mutation rather than writing outside meter. */
int ntm_shot_power_meter_tick(ntm_shot_power_meter *,ntm_shot_scene_object_context *,
    const ntm_shot_power_meter_resources *,uint8_t lifecycle,ntm_task_context *,ntm_task *);
#endif
