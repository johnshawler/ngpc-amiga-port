/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "task_lifecycle.h"
int ntm_task_finish(ntm_task_context *context,ntm_task *task)
{
    ntm_task *previous,*next;
    if(!context || !task || !task->next || !task->previous)return -1;
    next=task->next;
    if(task->priority<next->priority) {
        task->callback=ntm_task_finish;
        return NTM_TASK_DEFERRED;
    }
    previous=task->previous;
    previous->next=next;next->previous=previous;
    if(!(task->flags&1)) {
        task->next=context->free_head;context->free_head=task;
        context->current=previous;return NTM_TASK_RECYCLED;
    }
    context->current=previous;return NTM_TASK_UNLINKED;
}
int ntm_course_task(ntm_task_context *context,ntm_task *task)
{
    ntm_course_choice choice;int effect;
    if(!context || !context->controls || !task || !task->owner)return -1;
    choice.selection=context->selection;choice.result=task->owner->result;
    effect=ntm_course_choice_update(&choice,context->controls[0],context->controls[1],context->gate);
    context->selection=choice.selection;task->owner->result=choice.result;
    if(effect==NTM_COURSE_FINISH)return ntm_task_finish(context,task);
    if(effect==NTM_COURSE_MOVE_SOUND && context->sound_request)context->sound_request(context->sound_context,2);
    return NTM_TASK_IDLE;
}
int ntm_task_dispatch_one(ntm_task_context *context)
{
    int outcome;
    if(!context || !context->current || !context->current->callback)return -1;
    context->published=context->current;
    outcome=context->current->callback(context,context->current);
    if(outcome<0 || !context->current || !context->current->next)return -1;
    context->current=context->current->next;
    return outcome;
}
