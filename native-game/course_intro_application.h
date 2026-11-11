/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_COURSE_INTRO_APPLICATION_H
#define NTM_COURSE_INTRO_APPLICATION_H
#include "course_application.h"
#include "course_intro.h"
#include "course_title.h"
#include "round_record.h"
#include "round_prepare.h"
enum {NTM_INTRO_BOUNDARY_NONE=0,NTM_INTRO_BOUNDARY_TITLE,
      NTM_INTRO_BOUNDARY_PRIOR_SELECTION,NTM_INTRO_BOUNDARY_SPECIAL_RECORD,
      NTM_INTRO_BOUNDARY_ROUND_RECORD,NTM_INTRO_BOUNDARY_TEE_SHOT,NTM_INTRO_BOUNDARY_ROUND_UNSUPPORTED};
typedef struct {
    ntm_course_application selector; /* First: driver/context callback recovery. */
    ntm_course_owner owner;
    ntm_course_records records; /* Caller state; zero means normal golfer0/bank0. */
    ntm_course_intro intro;
    ntm_course_intro_context intro_context;
    ntm_task fallback; /* Reserved inert allocation fallback, never in ring. */
    const ntm_course_intro_resources *courses; /* Three private resource groups. */
    uint32_t selector_exit_update,intro_start_update,first_visible_update;
    uint8_t boundary;
    /* Optional full title path; the original scenery-only entry stays bounded. */
    const ntm_course_title_resources *title_resources;
    const ntm_cursor_assets *title_objects;
    ntm_course_title title;
    ntm_course_title_context title_context;
    ntm_course_cursor title_cursor;
    uint16_t title_x;
    int (*selector_drawing_head)(void *),(*selector_tail)(void *);
    uint32_t title_start_update,tournament_start_update,title_complete_update,tournament_prepare_update;
    const ntm_round_record_resources *round_resources;
    ntm_round_prepare_state round_prepare;
    ntm_round_record round;
    ntm_round_record_context round_context;
    ntm_round_record_drawing round_drawing;
    ntm_round_record_blink round_blink;
    ntm_course_cursor round_cursor;uint16_t round_x;
    uint32_t round_start_update,round_open_update,round_complete_update,round_drawing_update;
    uint8_t round_owner_active;
} ntm_course_intro_application;
/* Preserve normal application initialization and service order. The real outer
 * task is inserted before the selector parent before its first traversal.
 * No restored save progression or title object is implied by default records. */
int ntm_course_intro_application_start(ntm_course_intro_application *app,
    const ntm_course_resources *selector,const ntm_course_intro_resources courses[3]);
/* Continue original title/tournament lifecycle, then stop at the real owner's
 * unconverted round-record request. Default normal input/game mode are zero. */
int ntm_course_intro_application_start_titles(ntm_course_intro_application *app,
    const ntm_course_resources *selector,const ntm_course_intro_resources courses[3],
    const ntm_course_title_resources titles[3],const ntm_cursor_assets objects[3]);
/* Fresh normal single-player record continuation through cleanup. Stops on
 * the owner's later tee-shot request208753. Save/progression variants remain
 * explicit unsupported boundaries. Defaults initialize missing course records. */
int ntm_course_intro_application_start_round_record(ntm_course_intro_application *,
    const ntm_course_resources *,const ntm_course_intro_resources courses[3],
    const ntm_course_title_resources titles[3],const ntm_cursor_assets objects[3],
    const ntm_round_record_resources *);
int ntm_course_intro_application_update(ntm_course_intro_application *app,uint8_t buttons);
#endif
