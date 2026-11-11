/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_GOLFER_ANIMATION_H
#define NTM_SHOT_GOLFER_ANIMATION_H
#include "shot_golfer.h"
/* Resolved original four-frame scenery sequences. Ground kinds0/4/8/12
 * use rows0..3; foreground kind0 uses row4. No embedded ROM pointers. */
typedef struct {ntm_screen_asset frames[5][4];} ntm_shot_golfer_animation_resources;
/* Output owned by the separate, not-yet-converted initial green drawing stage.
 * These are original4535/4537/439f; no guessed geometry is generated here. */
typedef struct {uint16_t center_x,half_width;uint8_t status;} ntm_shot_golfer_green;
/* Complete20a5c0 callback:20ac01 then20ac50, including20ac7e.
 * Call only at its genuine scheduling boundary after initial drawing20a5aa.
 * Does not install a callback, advance the scheduler or consume drawing jobs.
 * Static kind252 retains all four bytes. Unsupported kinds or odd expiring
 * frame indices return NTM_TASK_BOUNDARY before any mutation. Invalid contracts
 * return-1. Resources remain borrowed until the actual drawing head consumes.
 * Timer0 wraps to255; timer1 reloads and advances even when the queue is full. */
int ntm_shot_golfer_animation_tick(ntm_shot_golfer *,ntm_course_screen *,
    const ntm_shot_golfer_animation_resources *);
/* Complete20a94a: adjust real slot5 attributes and terrain6/7 animation.
 * Green data must come from the real drawing stage, not fabricated defaults. */
int ntm_shot_golfer_pin_adjust(const ntm_shot_golfer *,ntm_shot_scene_object_context *,
    const ntm_shot_golfer_green *);
#endif
