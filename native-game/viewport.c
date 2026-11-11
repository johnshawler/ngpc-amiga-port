/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "viewport.h"
int ntm_viewport_fade(ntm_task *task,uint8_t viewport[4],const uint8_t window[256])
{
    unsigned index;if(!task || !viewport || !window)return -1;
    index=task->substate;viewport[0]=window[index^128u];
    viewport[2]=window[(uint8_t)(index+1u)^128u];
    task->substate=(uint8_t)(index+2u);return task->substate<14;
}
int ntm_course_setup(uint8_t mapped,uint8_t *index,uint8_t order[3])
{
    if(mapped>2 || !index || !order)return -1;
    *index=0;order[0]=mapped;order[1]=(uint8_t)((mapped+1u)%3u);order[2]=(uint8_t)((mapped+2u)%3u);return 0;
}
