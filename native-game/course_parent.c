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
    case NTM_PARENT_SETUP_PENDING:
        if(!state->accept_exit_callback)return -1;
        state->phase=NTM_PARENT_SERVICE54_PENDING;return NTM_PARENT_SERVICE54;
    case NTM_PARENT_SERVICE54_PENDING:
        if(!state->accept_exit_callback)return -1;
        parent->callback=state->accept_exit_callback;state->phase=NTM_PARENT_ACCEPT_EXIT;return NTM_PARENT_NONE;
    case NTM_PARENT_EXIT_CANCEL_PENDING:
    case NTM_PARENT_EXIT_ACCEPT_PENDING:
        if(!parent->owner)return -1;
        parent->owner->result=state->phase==NTM_PARENT_EXIT_CANCEL_PENDING?252:0;
        if(ntm_task_remove_descendants(context,parent)<0)return -1;
        {int result=ntm_task_finish(context,parent);if(result<0)return -1;state->finish_outcome=(uint8_t)result;}
        state->phase=NTM_PARENT_DONE;return NTM_PARENT_NONE;
    case NTM_PARENT_CANCEL_SOUND:
        if(ntm_task_remove_descendants(context,parent)<0)return -1;
        state->phase=NTM_PARENT_CANCEL_RENDER;return NTM_PARENT_RENDER;
    case NTM_PARENT_ACCEPT_SOUND:
        state->phase=NTM_PARENT_ACCEPT_RENDER;return NTM_PARENT_RENDER;
    case NTM_PARENT_RANDOM_RENDER:
        state->phase=NTM_PARENT_RANDOM_SOUND;return NTM_PARENT_SOUND_TWO;
    case NTM_PARENT_RANDOM_SOUND:
        if(!state->random_delay_callback)return -1;
        parent->delay=1; /* Store2, then same-call decrement. */
        parent->callback=state->random_delay_callback;state->phase=NTM_PARENT_RANDOM_DELAY;return NTM_PARENT_NONE;
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
int ntm_course_parent_supply_random(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state,uint8_t value)
{
    if(!context || !parent || !state || state->phase!=NTM_PARENT_RANDOM_PENDING)return -1;
    context->selection=(uint8_t)(value%3u);
    state->phase=NTM_PARENT_RANDOM_RENDER;return NTM_PARENT_RENDER;
}
int ntm_course_parent_tick(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state)
{
    static const uint8_t mapping[3]={2,0,1};
    unsigned kind;
    if(!context || !parent || !state)return -1;
    if(state->phase==NTM_PARENT_ACCEPT_EXIT) {
        if(!state->accept_exit_callback || parent->callback!=state->accept_exit_callback)return -1;
        state->viewport[0]=state->viewport[1]=state->viewport[2]=0;state->viewport[3]=152;
        state->phase=NTM_PARENT_EXIT_ACCEPT_PENDING;return NTM_PARENT_EXIT;
    }
    if(state->phase==NTM_PARENT_RANDOM_DELAY && parent->callback==state->random_delay_callback)kind=2;
    else if(state->phase==NTM_PARENT_FADE_DELAY && parent->callback==state->fade_callback)kind=3;
    else if(state->phase==NTM_PARENT_DELAY && parent->callback==state->cancel_delay_callback)kind=0;
    else if(state->phase==NTM_PARENT_DELAY && parent->callback==state->accept_delay_callback)kind=1;
    else return -1;
    if((kind==0 && !state->fade_callback) || (kind==1 && (!state->setup_callback || context->selection>2)) ||
       (kind==2 && !state->random_cycle_callback))return -1;
    parent->delay=(uint16_t)(parent->delay-1u);
    if(!(parent->delay&0x8000))return NTM_PARENT_NONE;
    if(kind==0) {
        parent->substate=0;parent->delay=0;parent->callback=state->fade_callback;
        state->phase=NTM_PARENT_FADE_DELAY;return NTM_PARENT_NONE;
    }
    if(kind==1) {
        parent->callback=state->setup_callback;state->mapped_course=mapping[context->selection];
        state->phase=NTM_PARENT_SETUP_PENDING;return NTM_PARENT_COURSE_SETUP;
    }
    if(kind==3){state->phase=NTM_PARENT_FADE_PENDING;return NTM_PARENT_FADE;}
    parent->callback=state->random_cycle_callback;
    state->random_remaining=(uint8_t)(state->random_remaining-1u);
    if(state->random_remaining){state->phase=NTM_PARENT_RANDOM_PENDING;return NTM_PARENT_RANDOM;}
    state->phase=NTM_PARENT_ACCEPT_SOUND;return NTM_PARENT_SOUND_ZERO;
}
int ntm_course_parent_generate_random(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state,uint8_t *cursor,const uint8_t window[256])
{
    int value;
    if(!context || !parent || !state || state->phase!=NTM_PARENT_RANDOM_PENDING)return -1;
    value=ntm_random_next(cursor,window);if(value<0)return -1;
    return ntm_course_parent_supply_random(context,parent,state,(uint8_t)value);
}
int ntm_course_parent_supply_fade(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state,int busy)
{
    if(!context || !parent || !state || state->phase!=NTM_PARENT_FADE_PENDING)return -1;
    if(busy){state->phase=NTM_PARENT_FADE_DELAY;return NTM_PARENT_NONE;}
    if(!state->cancel_exit_callback)return -1;
    parent->callback=state->cancel_exit_callback;state->phase=NTM_PARENT_EXIT_CANCEL_PENDING;return NTM_PARENT_EXIT;
}
int ntm_course_parent_fade(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state,const uint8_t window[256])
{
    int busy;
    if(!context || !parent || !state || state->phase!=NTM_PARENT_FADE_PENDING || !state->cancel_exit_callback)return -1;
    busy=ntm_viewport_fade(parent,state->viewport,window);if(busy<0)return -1;
    return ntm_course_parent_supply_fade(context,parent,state,busy);
}
int ntm_course_parent_setup(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state)
{
    if(!context || !parent || !state || state->phase!=NTM_PARENT_SETUP_PENDING || !state->accept_exit_callback)return -1;
    if(ntm_course_setup(state->mapped_course,&state->course_index,state->course_order)<0)return -1;
    return ntm_course_parent_resume(context,parent,state);
}
int ntm_course_parent_service54(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state,ntm_service54_state *queue,const uint8_t payload[9])
{
    if(!context || !parent || !state || state->phase!=NTM_PARENT_SERVICE54_PENDING || !state->accept_exit_callback)return -1;
    if(ntm_service54(queue,payload)<0)return -1;
    return ntm_course_parent_resume(context,parent,state);
}
int ntm_course_parent_exit(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state,ntm_display_reset_state *display)
{
    if(!context || !parent || !parent->owner || !state ||
       (state->phase!=NTM_PARENT_EXIT_CANCEL_PENDING && state->phase!=NTM_PARENT_EXIT_ACCEPT_PENDING))return -1;
    if(ntm_display_reset(display)<0)return -1;
    return ntm_course_parent_resume(context,parent,state);
}
