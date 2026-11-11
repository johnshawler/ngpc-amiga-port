/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_APPLICATION_H
#define NTM_SHOT_APPLICATION_H
#include "tee_setup_application.h"
#include "shot_scene.h"
#include "shot_banner.h"
#include "shot_palette.h"
typedef struct {
    const ntm_shot_prepare_resources *preparation;
    const ntm_terrain_resources *terrain;
    const ntm_shot_banner_resources *banner;
    const ntm_tee_object_assets *objects;
    const ntm_shot_palette_job *exit_palettes; /* Exact commands6c then70. */
} ntm_shot_resources;
enum {NTM_SHOT_BOUNDARY_NONE=0,NTM_SHOT_BOUNDARY_SCENE_SETUP,NTM_SHOT_BOUNDARY_UNSUPPORTED,NTM_SHOT_BOUNDARY_AIM_SETUP,NTM_SHOT_BOUNDARY_AIM_OPEN};
typedef struct {
    ntm_tee_setup_application base; /* First for existing scheduled services. */
    ntm_shot_scene scene;ntm_shot_scene_context scene_context;
    ntm_shot_banner banner;ntm_shot_banner_animation animation;ntm_shot_banner_exit exit;
    ntm_shot_banner_context banner_context;ntm_shot_raster raster;ntm_shot_palette palette;
    ntm_task extra_tasks[4];const ntm_shot_resources *resources;
    uint8_t started,boundary;uint32_t start_update,banner_update,complete_update;
    ntm_task_callback aim_continuation; /* Optional real209751 continuation; NULL keeps old boundary. */
} ntm_shot_application;
/* Opt-in coherent first tee only. Real task storage extends the original pool;
 * no child uses a fabricated completion or an intentionally shared fallback.
 * Raster scanline service is explicit: callers preserve its timing/latch state
 * and use shot_raster_step for display rows, not a frame-end scalar scroll. */
int ntm_shot_application_start(ntm_shot_application *,const ntm_course_resources *,
    const ntm_course_intro_resources courses[3],const ntm_course_title_resources titles[3],
    const ntm_cursor_assets title_objects[3],const ntm_round_record_resources *,
    const ntm_tee_setup_resources *,const ntm_hole_prepare_resources *,const ntm_shot_resources *);
int ntm_shot_application_update(ntm_shot_application *,uint8_t buttons);
#endif
