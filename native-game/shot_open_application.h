/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_OPEN_APPLICATION_H
#define NTM_SHOT_OPEN_APPLICATION_H
#include "shot_aim_application.h"
#include "shot_club.h"
#include "shot_markers.h"
typedef struct {
    const ntm_shot_scene_object_resources *scene;
    const ntm_shot_club_resources *club;
    const ntm_shot_marker_resources *markers;
    const ntm_tee_object_assets *objects;
} ntm_shot_open_resources;
typedef struct {
    ntm_shot_aim_application base;
    ntm_shot_scene_object_context scene_context;
    ntm_shot_marker_context marker_context;
    ntm_shot_club club;ntm_shot_markers markers;
    ntm_shot_hint_delay hint;ntm_shot_camera_delay camera;
    const ntm_shot_open_resources *resources;
    uint8_t saved_map[32],phase,handlers_installed,input_dispatch;
    uint32_t open_update;
} ntm_shot_open_application;
/* Enable after both base start and aim enable, before the first update. The
 * real209830 child allocation order, same-call waits, child init/ticks and
 * viewport write2098af run on the existing scheduler. Stops after that update
 * with2098ee installed and its original delay3, not a playable shot. */
int ntm_shot_open_application_enable(ntm_shot_open_application *,const ntm_shot_open_resources *);
#endif
