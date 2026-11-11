/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_COURSE_PALETTE_H
#define NTM_COURSE_PALETTE_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    uint8_t kind,pending_index; /*1 groups,2 single color*/
    uint16_t offset,count,color,control_offset;
    const uint8_t *offsets,*colors;
    size_t offsets_size,colors_size;
} ntm_palette_job;
typedef struct {uint16_t offset,words;} ntm_palette_copy;
typedef struct {
    uint16_t slots[8],status,copy_bytes;
    ntm_palette_job jobs[8];
    ntm_palette_copy copies[8];
    uint8_t staging[512],palette[512],pending[2],background,window_control;
} ntm_course_palette;
/* Color-mode init jobs only. Borrowed data remains valid through tail.
 * Existing slots, staging and copy jobs are NOT reset by screen initialization.
 * Returns0 queued,1 full,-1 invalid. Caller initializes free slots and pending
 * sentinels when creating a fresh manager, not at every course screen. */
int ntm_course_palette_job_valid(const ntm_course_palette *state,const ntm_palette_job *job);
int ntm_course_palette_enqueue(ntm_course_palette *state,const ntm_palette_job *job);
/* Original tail sorts slots, resolves supported jobs into staging/copy queue;
 * next head uploads colors and controls. Maximum8 pending copy descriptors.
 * Malformed/unsupported jobs fail before any mutation. */
int ntm_course_palette_tail(ntm_course_palette *state);
int ntm_course_palette_head(ntm_course_palette *state);
#endif
