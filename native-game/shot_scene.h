/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_SCENE_H
#define NTM_SHOT_SCENE_H
#include "shot_prepare.h"
#include "shot_terrain.h"
#include "tee_setup.h"
typedef struct {
    ntm_task_callback banner_entry,banner_wait;
    ntm_shot_player_state player;
    uint16_t terrain_detail;
    uint8_t player_status,phase;
} ntm_shot_scene;
typedef struct {
    ntm_tee_setup_context *shared;ntm_hole_prepare_state *hole;
    const ntm_shot_prepare_resources *preparation;const ntm_terrain_resources *terrain;
    uint8_t *random_cursor;const uint8_t *random_window;
} ntm_shot_scene_context;
/* Original2096a4 initial state, real preparation/terrain/wind and actual banner
 * allocation. The caller must supply the converted banner entry, not a stub.
 * Existing large-map identity is not guessed: initial tee classification needs
 * no map, later non-tee queries require their actual resolved resource. */
int ntm_shot_scene_initialize(ntm_shot_scene *,ntm_task *,ntm_shot_scene_context *);
/* Original209749 wait. Returns1 at209751 before unconverted scene/aim setup,
 * only after the real banner child has signaled completion. Stable thereafter. */
int ntm_shot_scene_wait(ntm_shot_scene *,ntm_task *);
#endif
