/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_AIM_APPLICATION_H
#define NTM_SHOT_AIM_APPLICATION_H
#include "shot_application.h"
#include "shot_aim.h"
#include "shot_aim_setup.h"
#include "shot_hud.h"
#include "aim_world.h"
typedef struct {
    const ntm_shot_aim_resources *aim;
    const ntm_aim_world_resources *worlds;size_t world_count; /* course*18+hole */
    const ntm_shot_hud_resources *hud;
    const ntm_shot_aim_setup_resources *setup;
    const ntm_tee_object_assets *objects[54]; /* Actual tag5 art, course*18+hole. */
    ntm_palette_job palette; /* Original command0c. */
} ntm_shot_aim_application_resources;
typedef struct {
    ntm_shot_application base;
    ntm_shot_aim aim;ntm_aim_world_state world;
    uint8_t saved_hud[4],phase;
    const ntm_shot_aim_application_resources *resources;
    ntm_task_callback scene_continuation; /* Optional genuine209830; NULL preserves setup boundary. */
} ntm_shot_aim_application;
/* Call after base start and before first update. Zero-init state first. This
 * installs genuine scene continuation with original2049eb map/palette defaults
 * for a fresh game. It does not rewrite timings or complete missing children.
 * Borrow all resources through the final palette/object head drain. */
int ntm_shot_aim_application_enable(ntm_shot_aim_application *,const ntm_shot_aim_application_resources *);
/* Exact209751 setup through20982f; next real dispatch209830 stays unconverted.
 * BoundaryNTM_SHOT_BOUNDARY_AIM_SETUP means initialized scenery, not aiming. */
#endif
