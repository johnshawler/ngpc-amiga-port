/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_PALETTE_H
#define NTM_SHOT_PALETTE_H
#include "course_palette.h"
typedef struct {
    uint8_t kind,speed,pending_index; /* Original kinds24 or40. */
    uint16_t offset,count,source_color,target_color,control_offset;
    const uint8_t *source,*target,*blend;
    size_t source_size,target_size,blend_size;
} ntm_shot_palette_job;
typedef struct {
    ntm_shot_palette_job jobs[8];
    uint8_t fraction[8],level[8];
} ntm_shot_palette;
/* Group arrays are resolved contiguous four-color groups, including the unused
 * transparent color. blend is the512-byte window starting16 bytes before the
 * original interpolation table. Borrowed spans remain alive through completion.
 * Return0 queued,1 full,-1 invalid; invalid input does not mutate either state. */
int ntm_shot_palette_enqueue(ntm_course_palette *,ntm_shot_palette *,const ntm_shot_palette_job *);
/* Combined sorted tail for existing init jobs plus original kinds24/40.
 * Existing ntm_course_palette_head remains the upload operation. */
int ntm_shot_palette_tail(ntm_course_palette *,ntm_shot_palette *);
#endif
