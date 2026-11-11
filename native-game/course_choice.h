/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_COURSE_CHOICE_H
#define NTM_COURSE_CHOICE_H
#include <stdint.h>
#include "input_state.h"
typedef struct {uint8_t selection,result;} ntm_course_choice;
enum {NTM_COURSE_IDLE=0,NTM_COURSE_MOVE_SOUND=1,NTM_COURSE_FINISH=2};
/* Native course-selector transition. Effects are requests, not implementations
 * of the sound service or task-removal scheduler. MOVE_SOUND means argument2.
 * FINISH means result was written and the current selector task should finish.
 * Configuration gate!=0 disables B cancel. Inputs are game button bits. */
int ntm_course_choice_update(ntm_course_choice *state,uint8_t current,uint8_t pressed,uint8_t gate);
/* One ACTIVE-menu control tick. Caller owns menu lifetime, mapping, cadence,
 * rendering and effect dispatch. This is not a complete game loop. */
int ntm_course_choice_tick(uint8_t controls[16],ntm_course_choice *state,uint8_t sample,uint8_t gate);
#endif
