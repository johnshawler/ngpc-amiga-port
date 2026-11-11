/* SPDX-License-Identifier: GPL-2.0-or-later. Native state-level game consumer. */
#include "course_choice.h"
int ntm_course_choice_update(ntm_course_choice *s,uint8_t current,uint8_t pressed,uint8_t gate)
{
    unsigned candidate,direction;
    if(!s)return -1;
    if(!gate && (pressed&32)) {s->result=2;return NTM_COURSE_FINISH;}
    if(pressed&16) {s->result=(current&64)?3:1;return NTM_COURSE_FINISH;}
    direction=pressed&12;
    if(direction==4)candidate=(s->selection-1u)&255u;
    else if(direction==8)candidate=(s->selection+1u)&255u;
    else return NTM_COURSE_IDLE;
    if(candidate>2)return NTM_COURSE_IDLE;
    s->selection=(uint8_t)candidate;return NTM_COURSE_MOVE_SOUND;
}
int ntm_course_choice_tick(uint8_t controls[16],ntm_course_choice *s,uint8_t sample,uint8_t gate)
{
    if(!controls || !s)return -1;
    ntm_input_update(controls,sample);
    return ntm_course_choice_update(s,controls[0],controls[1],gate);
}
