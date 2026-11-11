/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_COURSE_PARENT_H
#define NTM_COURSE_PARENT_H
#include "task_allocation.h"
#include "random_byte.h"
#include "viewport.h"
#include "game_services.h"
enum {NTM_PARENT_NONE=0,NTM_PARENT_SOUND_ZERO=1,NTM_PARENT_SOUND_ONE=2,
      NTM_PARENT_RENDER=3,NTM_PARENT_RANDOM=4,NTM_PARENT_SOUND_TWO=5,
      NTM_PARENT_FADE=6,NTM_PARENT_COURSE_SETUP=7,NTM_PARENT_SERVICE54=8,NTM_PARENT_EXIT=9};
enum {NTM_PARENT_WAIT=0,NTM_PARENT_CANCEL_SOUND,NTM_PARENT_CANCEL_RENDER,
      NTM_PARENT_ACCEPT_SOUND,NTM_PARENT_ACCEPT_RENDER,NTM_PARENT_RANDOM_PENDING,
      NTM_PARENT_DELAY,NTM_PARENT_RANDOM_RENDER,NTM_PARENT_RANDOM_SOUND,
      NTM_PARENT_RANDOM_DELAY,NTM_PARENT_FADE_DELAY,NTM_PARENT_FADE_PENDING,
      NTM_PARENT_SETUP_PENDING,NTM_PARENT_SERVICE54_PENDING,NTM_PARENT_ACCEPT_EXIT,
      NTM_PARENT_EXIT_CANCEL_PENDING,NTM_PARENT_EXIT_ACCEPT_PENDING,NTM_PARENT_DONE};
typedef struct {
    ntm_task_callback wait_callback,cancel_delay_callback,accept_delay_callback;
    ntm_task_callback random_delay_callback,random_cycle_callback,fade_callback,setup_callback;
    ntm_task_callback cancel_exit_callback,accept_exit_callback;
    uint16_t global_word;
    uint8_t global_flags,random_remaining,phase,mapped_course;
    uint8_t viewport[4],finish_outcome,course_index,course_order[3];
} ntm_course_parent_state;
/* Caller supplies distinct native renderer and continuation callback identities. */
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
 * explicit service boundaries; use the converted service helpers below. */
int ntm_course_parent_tick(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state);
/* Complete viewport-animation service, after applying its substate/viewport
 * changes. busy!=0 corresponds to original carry set. Exit service remains an
 * explicit request; only its successful completion allows owner/task removal. */
int ntm_course_parent_supply_fade(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state,int busy);
/* Execute the converted fade or course-setup service and continue its parent. */
int ntm_course_parent_fade(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state,const uint8_t window[256]);
int ntm_course_parent_setup(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state);
/* Execute converted queued-object initializer or display reset. A full queue
 * is not an error: original parent ignores carry and proceeds. Rendering and
 * audio realization of queued objects remain separate integration work. */
int ntm_course_parent_service54(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state,ntm_service54_state *queue,const uint8_t payload[9]);
int ntm_course_parent_exit(ntm_task_context *context,ntm_task *parent,
    ntm_course_parent_state *state,ntm_display_reset_state *display);
#endif
