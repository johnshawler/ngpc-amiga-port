/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_CONTROL_APPLICATION_H
#define NTM_SHOT_CONTROL_APPLICATION_H
#include "shot_open_application.h"
#include "shot_camera.h"
#include "shot_ready.h"
typedef struct {
    ntm_shot_open_application base;
    ntm_shot_camera_state camera;
    uint8_t enabled,ready,partial_boundary;
    uint32_t ready_update;
} ntm_shot_control_application;
/* Enable after opening enable, before first update. Runs the genuine main-loop
 * world-scroll service and ready/camera/club controls. No synthetic end timer.
 * Fire and information-overlay/reset dispatch stop at explicit partial-update
 * boundaries; they never execute a fabricated swing/overlay. */
int ntm_shot_control_application_enable(ntm_shot_control_application *);
#endif
