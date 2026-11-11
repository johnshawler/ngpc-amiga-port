/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_COURSE_INTRO_APPLICATION_H
#define NTM_COURSE_INTRO_APPLICATION_H
#include "course_application.h"
#include "course_intro.h"
enum {NTM_INTRO_BOUNDARY_NONE=0,NTM_INTRO_BOUNDARY_TITLE,
      NTM_INTRO_BOUNDARY_PRIOR_SELECTION,NTM_INTRO_BOUNDARY_SPECIAL_RECORD,
      NTM_INTRO_BOUNDARY_ROUND_RECORD};
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
} ntm_course_intro_application;
/* Preserve normal application initialization and service order. The real outer
 * task is inserted before the selector parent before its first traversal.
 * No restored save progression or title object is implied by default records. */
int ntm_course_intro_application_start(ntm_course_intro_application *app,
    const ntm_course_resources *selector,const ntm_course_intro_resources courses[3]);
int ntm_course_intro_application_update(ntm_course_intro_application *app,uint8_t buttons);
#endif
