/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_COURSE_PARENT_H
#define NTM_COURSE_PARENT_H
#include "task_allocation.h"
enum {NTM_PARENT_NONE=0,NTM_PARENT_SOUND_ZERO=1,NTM_PARENT_SOUND_ONE=2,
      NTM_PARENT_RENDER=3,NTM_PARENT_RANDOM=4};
enum {NTM_PARENT_WAIT=0,NTM_PARENT_CANCEL_SOUND,NTM_PARENT_CANCEL_RENDER,
      NTM_PARENT_ACCEPT_SOUND,NTM_PARENT_ACCEPT_RENDER,NTM_PARENT_RANDOM_PENDING,
      NTM_PARENT_DELAY};
typedef struct {
    ntm_task_callback wait_callback,cancel_delay_callback,accept_delay_callback;
    uint16_t global_word;
    uint8_t global_flags,random_remaining,phase;
} ntm_course_parent_state;
/* Caller supplies native renderer and continuation callback identities. */
int ntm_course_parent_initialize(ntm_task_context *context,ntm_task *parent,
    ntm_task_callback renderer,ntm_task *fallback,ntm_course_parent_state *state);
/* Return a service REQUEST and suspend before it executes. Caller must not
 * advance dispatcher while suspended. No service implementation is implied. */
int ntm_course_parent_begin(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state);
/* Invoke ONLY after the pending sound/render service completed successfully.
 * Explicit phase prevents repeated entry from duplicating cleanup or requests.
 * RANDOM remains an unconverted continuation; resume returns -1 unchanged. */
int ntm_course_parent_resume(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state);
#endif
