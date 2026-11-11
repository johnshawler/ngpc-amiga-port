/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_parent.h"
int ntm_course_parent_initialize(ntm_task_context *context,ntm_task *parent,
    ntm_task_callback renderer,ntm_task *fallback,ntm_course_parent_state *state)
{
    if(!context || !parent || !parent->next || !state || !renderer || !state->wait_callback)return -1;
    (void)ntm_task_allocate(context,parent,renderer,fallback);
    (void)ntm_task_allocate(context,parent,ntm_course_task,fallback);
    parent->result=0;parent->callback=state->wait_callback;state->phase=NTM_PARENT_WAIT;
    return 0;
}
int ntm_course_parent_begin(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state)
{
    if(!context || !parent || !state || state->phase!=NTM_PARENT_WAIT)return -1;
    if(!parent->result)return NTM_PARENT_NONE;
    if(parent->result==2){state->phase=NTM_PARENT_CANCEL_SOUND;return NTM_PARENT_SOUND_ONE;}
    if(ntm_task_remove_descendants(context,parent)<0)return -1;
    if(parent->result==3){state->random_remaining=30;state->phase=NTM_PARENT_RANDOM_PENDING;return NTM_PARENT_RANDOM;}
    state->phase=NTM_PARENT_ACCEPT_SOUND;return NTM_PARENT_SOUND_ZERO;
}
int ntm_course_parent_resume(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state)
{
    if(!context || !parent || !state)return -1;
    switch(state->phase) {
    case NTM_PARENT_CANCEL_SOUND:
        if(ntm_task_remove_descendants(context,parent)<0)return -1;
        state->phase=NTM_PARENT_CANCEL_RENDER;return NTM_PARENT_RENDER;
    case NTM_PARENT_ACCEPT_SOUND:
        state->phase=NTM_PARENT_ACCEPT_RENDER;return NTM_PARENT_RENDER;
    case NTM_PARENT_CANCEL_RENDER:
        if(!state->cancel_delay_callback)return -1;
        parent->delay=4; /* Store5, then same-call signed decrement to4. */
        parent->callback=state->cancel_delay_callback;state->phase=NTM_PARENT_DELAY;return NTM_PARENT_NONE;
    case NTM_PARENT_ACCEPT_RENDER:
        if(!state->accept_delay_callback)return -1;
        state->global_word=4;state->global_flags|=128;
        parent->delay=29; /* Store30, then same-call signed decrement to29. */
        parent->callback=state->accept_delay_callback;state->phase=NTM_PARENT_DELAY;return NTM_PARENT_NONE;
    default:return -1;
    }
}
