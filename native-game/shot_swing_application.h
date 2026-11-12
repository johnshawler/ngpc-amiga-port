/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_SWING_APPLICATION_H
#define NTM_SHOT_SWING_APPLICATION_H
#include "shot_power_application.h"
#include "shot_swing.h"
#include "shot_trajectory_swing.h"
typedef struct {
    const ntm_shot_power_application_resources *power;
    const ntm_shot_trajectory_swing_resources *trajectory;
} ntm_shot_swing_application_resources;
typedef struct {
    ntm_shot_power_application base;
    ntm_shot_swing swing;
    ntm_shot_swing_context context;
    ntm_shot_trajectory_swing trajectory;
    const ntm_shot_swing_application_resources *resources;
    uint8_t enabled,boundary,suppress_sound;
} ntm_shot_swing_application;
enum {NTM_SWING_APPLICATION_HIT_RESET=5,NTM_SWING_APPLICATION_MISS_CHILD=6};
/* Opt-in instead of power enable. Retains original scheduler ordering and
 * genuine child result publication. Stops before hit reset209b05/miss209b4b;
 * neither boundary is ball flight. Resource tables and suppress447e are owned
 * by caller. No baseline target or old power application is modified. */
int ntm_shot_swing_application_enable(ntm_shot_swing_application *,
    const ntm_shot_swing_application_resources *);
int ntm_shot_swing_application_update(ntm_shot_swing_application *,uint8_t);
#endif
