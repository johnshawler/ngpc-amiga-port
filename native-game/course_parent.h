/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_COURSE_PARENT_H
#define NTM_COURSE_PARENT_H
#include "task_allocation.h"
#include "random_byte.h"
enum {NTM_PARENT_NONE=0,NTM_PARENT_SOUND_ZERO=1,NTM_PARENT_SOUND_ONE=2,
      NTM_PARENT_RENDER=3,NTM_PARENT_RANDOM=4,NTM_PARENT_SOUND_TWO=5,
      NTM_PARENT_FADE=6,NTM_PARENT_COURSE_SETUP=7};
enum {NTM_PARENT_WAIT=0,NTM_PARENT_CANCEL_SOUND,NTM_PARENT_CANCEL_RENDER,
      NTM_PARENT_ACCEPT_SOUND,NTM_PARENT_ACCEPT_RENDER,NTM_PARENT_RANDOM_PENDING,
      NTM_PARENT_DELAY,NTM_PARENT_RANDOM_RENDER,NTM_PARENT_RANDOM_SOUND,
      NTM_PARENT_RANDOM_DELAY,NTM_PARENT_FADE_DELAY,NTM_PARENT_FADE_PENDING,
      NTM_PARENT_SETUP_PENDING};
typedef struct {
    ntm_task_callback wait_callback,cancel_delay_callback,accept_delay_callback;
    ntm_task_callback random_delay_callback,random_cycle_callback,fade_callback,setup_callback;
    uint16_t global_word;
    uint8_t global_flags,random_remaining,phase,mapped_course;
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
 * RANDOM requires supply_random(), not a blind resume. */
int ntm_course_parent_resume(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state);
/* Complete a pending random-byte request. Provider implementation is separate. */
int ntm_course_parent_supply_random(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state,uint8_t value);
/* Native completion of the pending table/cursor provider request. */
int ntm_course_parent_generate_random(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state,uint8_t *cursor,const uint8_t window[256]);
/* One later parent callback, only when no service is pending. Timers decrement
 * modulo 65536 and continue on result sign bit, including 0 -> 65535. Stops at
 * unconverted fade/course-setup services with explicit pending phases. */
int ntm_course_parent_tick(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state);
#endif
