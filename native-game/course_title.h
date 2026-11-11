/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_COURSE_TITLE_H
#define NTM_COURSE_TITLE_H
#include "course_intro.h"
#include "course_initialization.h"
typedef struct {
    const uint8_t *descriptor;size_t descriptor_size; /* full 12-byte descriptor */
    const uint8_t *metadata;size_t metadata_size; /* five bytes at original21980e */
    ntm_screen_asset title,tournament[2]; /* group entries +12, +8 / +16 */
} ntm_course_title_resources;
typedef struct {
    ntm_course_intro_context *intro;
    ntm_course_cursor *cursor;
    uint16_t *cursor_x;
    ntm_task_context *tasks;
    uint8_t game_mode,input_mode,control,linked_controls[2];
} ntm_course_title_context;
enum {NTM_TITLE_NEW=0,NTM_TITLE_WAIT,NTM_TITLE_TOURNAMENT_DELAY,
      NTM_TITLE_TOURNAMENT_WAIT,NTM_TITLE_DONE};
enum {NTM_TITLE_NONE=0,NTM_TITLE_COMPLETE=1};
typedef struct {
    ntm_task_callback title_wait,tournament_delay,tournament_wait,cleanup;
    const uint8_t *descriptor_next;
    uint16_t asset_group;
    uint8_t phase,finish_outcome;
} ntm_course_title;
/* Original205318: normal pressed byte40c0; linked mode4148 uses OR of the
 * caller-resolved two linked control bytes. Neither reads nor consumes input. */
int ntm_course_title_skip(uint8_t control,uint8_t mode,const uint8_t linked[2]);
/* Continue ONLY the existing scenery TITLE_PENDING boundary. All spans are
 * caller-owned, kept alive until queued work drains. Registration appends to
 * the existing object list; the rendering adapter may impose a stricter bound. */
int ntm_course_title_initialize(ntm_course_title *state,ntm_task *task,
    ntm_course_title_context *context,const ntm_course_title_resources *resources,
    const ntm_course_intro *intro);
/* Stable COMPLETE after cleanup; completion does not execute the outer owner.
 * The scheduled owner subsequently requests unsupported round-record itself. */
int ntm_course_title_tick(ntm_course_title *state,ntm_task *task,
    ntm_course_title_context *context,const ntm_course_title_resources *resources);
#endif
