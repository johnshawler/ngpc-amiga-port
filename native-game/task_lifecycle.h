/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_TASK_LIFECYCLE_H
#define NTM_TASK_LIFECYCLE_H
#include <stdint.h>
#include "course_choice.h"
typedef struct ntm_task ntm_task;
typedef struct ntm_task_context ntm_task_context;
typedef int (*ntm_task_callback)(ntm_task_context *,ntm_task *);
struct ntm_task {
    ntm_task_callback callback;
    ntm_task *previous,*next,*owner;
    uint8_t priority,result,flags,substate;
    uint16_t delay;
};
struct ntm_task_context {
    ntm_task *free_head,*current,*published;
    uint8_t *controls;
    uint8_t selection,gate;
    /* The application supplies this service. It is not a native audio engine. */
    void (*sound_request)(void *,uint8_t);
    void *sound_context;
};
enum {NTM_TASK_IDLE=0,NTM_TASK_DEFERRED=1,NTM_TASK_UNLINKED=2,NTM_TASK_RECYCLED=3};
/* State-level task completion. Nodes/links must be valid, caller-owned objects,
 * context->current==task, with coherent links and no overlapping node storage.
 * Adapters map the original 0xffffffff free sentinel to a native sentinel/null.
 * Original register/stack/cycle ABI and allocation are outside this contract. */
int ntm_task_finish(ntm_task_context *context,ntm_task *task);
/* Native callback adapting the verified course handler to task-owned result. */
int ntm_course_task(ntm_task_context *context,ntm_task *task);
/* One original dispatch iteration: publish current, call, advance from the
 * callback-returned cursor. No frame pacing, allocator or unconverted callbacks. */
int ntm_task_dispatch_one(ntm_task_context *context);
#endif
