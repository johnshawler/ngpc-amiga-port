/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_FIRE_APPLICATION_H
#define NTM_SHOT_FIRE_APPLICATION_H
#include "shot_control_application.h"
#include "shot_fire.h"
#include "shot_spin.h"
#include "shot_trajectory.h"
#include "shot_golfer_drawing.h"
typedef struct {
    const ntm_shot_fire_resources *parent;
    const ntm_shot_spin_resources *spin;
    const ntm_shot_spin_draw_resources *spin_draw;
    const ntm_shot_trajectory_resources *trajectory;
    const ntm_shot_golfer_resources *golfer;
    const ntm_shot_golfer_drawing_resources *drawing; /* one per real hole */
    const ntm_terrain_resources *terrain; /* matching real large/close maps */
    size_t hole_count;
    const ntm_tee_object_assets *trajectory_objects,*pin_objects,*green_objects;
} ntm_shot_fire_application_resources;
typedef struct {
    ntm_shot_control_application base;
    ntm_shot_fire fire;
    ntm_shot_spin spin;
    ntm_shot_trajectory trajectory;
    ntm_shot_golfer golfer;
    ntm_shot_golfer_drawing drawing;
    ntm_shot_fire_context fire_context;
    ntm_shot_golfer_context golfer_context;
    const ntm_shot_fire_application_resources *resources;
    ntm_task extra_tasks[3];
    uint32_t fire_update,drawing_update;
    uint8_t enabled,boundary;
} ntm_shot_fire_application;
enum {NTM_FIRE_APPLICATION_NONE=0,NTM_FIRE_APPLICATION_POWER=1,
    NTM_FIRE_APPLICATION_CANCEL=2,NTM_FIRE_APPLICATION_CHILD=3};
/* Opt-in, after opening enable and before control enable/any update. Extends
 * genuine task capacity without altering the preserved pre-fire target. */
int ntm_shot_fire_application_enable(ntm_shot_fire_application *,
    const ntm_shot_fire_application_resources *);
/* Use this update entry point after enable. Real drawing head, input, scroll,
 * scheduler and tail order; power/cancel stop before their unconverted actions,
 * without executing the rest of that partial update. No ball flight. */
int ntm_shot_fire_application_update(ntm_shot_fire_application *,uint8_t buttons);
#endif
