/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_TEE_SETUP_APPLICATION_H
#define NTM_TEE_SETUP_APPLICATION_H
#include "course_intro_application.h"
#include "tee_setup.h"
#include "hole_prepare.h"
enum {NTM_TEE_BOUNDARY_NONE=0,NTM_TEE_BOUNDARY_SHOT_SCENE,NTM_TEE_BOUNDARY_UNSUPPORTED};
typedef struct {
    ntm_course_intro_application base; /* Must be first for scheduled callbacks. */
    ntm_tee_owner owner;ntm_tee_setup overview;ntm_tee_handicap handicap;
    ntm_tee_setup_context context;ntm_hole_prepare_state hole;
    ntm_course_cursor objects[32];uint16_t x[32];ntm_tee_object_engine engine;
    const ntm_tee_object_assets *object_resources[32];
    const ntm_tee_setup_resources *resources;const ntm_hole_prepare_resources *hole_resources;
    uint8_t saved_hole_scores[54],handicaps[18];
    int (*previous_head)(void *),(*previous_tail)(void *);
    uint8_t started,boundary;
    uint32_t start_update,overview_update,open_update,hold_update,complete_update;
} ntm_tee_setup_application;
/* Opt-in normal fresh-record path. Caller owns all assets and state; source
 * export remains separate. Defaults use independently observed fresh saved
 * scores10, difficulty2/previous0, no handicaps and English language1. Change those owned fields after
 * start for a supported saved state. Context may supply sound-request callback.
 * Stops before unconverted2096a4. No shot physics, aim or swing is implied.
 * Existing selector/title/round APIs and boundaries remain unchanged. */
int ntm_tee_setup_application_start(ntm_tee_setup_application *,
    const ntm_course_resources *,const ntm_course_intro_resources courses[3],
    const ntm_course_title_resources titles[3],const ntm_cursor_assets title_objects[3],
    const ntm_round_record_resources *,const ntm_tee_setup_resources *,const ntm_hole_prepare_resources *);
int ntm_tee_setup_application_update(ntm_tee_setup_application *,uint8_t buttons);
#endif
