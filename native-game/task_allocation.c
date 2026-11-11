/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "task_allocation.h"
ntm_task *ntm_task_allocate(ntm_task_context *context,ntm_task *parent,
                          ntm_task_callback callback,ntm_task *fallback)
{
    ntm_task *task,*next;
    if(!context)return 0;
    task=context->free_head;
    if(!task)return fallback;
    if(!parent || !parent->next || !callback)return 0;
    context->free_head=task->next;
    task->callback=callback;task->owner=parent;
    task->priority=(uint8_t)(parent->priority+1u);
    next=parent->next;parent->next=task;
    task->previous=parent;task->next=next;next->previous=task;
    task->flags=0;
    return task;
}
int ntm_task_remove_descendants(ntm_task_context *context,ntm_task *parent)
{
    int removed=0;ntm_task *task,*next,*previous;
    if(!context || !parent || !parent->next)return -1;
    while(parent->priority<parent->next->priority) {
        task=parent->next;previous=task->previous;next=task->next;
        if(!previous || !next)return -1;
        previous->next=next;next->previous=previous;
        if(!(task->flags&1)){task->next=context->free_head;context->free_head=task;}
        context->current=parent;removed++;
    }
    return removed;
}
