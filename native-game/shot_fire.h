/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_FIRE_H
#define NTM_SHOT_FIRE_H
#include "shot_markers.h"
typedef struct {
    ntm_task_callback spin_entry,trajectory_entry,golfer_entry;
    ntm_task_callback closed_delay,open_delay,ready;
    uint8_t phase,boundary;
} ntm_shot_fire;
typedef struct {
    ntm_tee_setup_context *shared;
    ntm_shot_aim *aim;
    uint8_t *overlay; /* Original413b. */
} ntm_shot_fire_context;
typedef struct {ntm_palette_job palette;} ntm_shot_fire_resources;
enum {NTM_SHOT_FIRE_NONE=0,NTM_SHOT_FIRE_POWER=1,NTM_SHOT_FIRE_CANCEL=2};
/* Exact209919 parent transition: real object removals, HUD clearing, palette
 * request, three allocations, and same-call first decrement. The callback
 * arguments must identify the real child entries, not success placeholders.
 * This module does NOT implement those children. A caller must stop before
 * their dispatch unless it supplies genuine implementations. Palette0x10 is
 * privately resolved and borrowed until its queued consumer has drained. */
int ntm_shot_fire_initialize(ntm_shot_fire *,ntm_task *,ntm_shot_fire_context *,
    const ntm_shot_fire_resources *);
/* Exact2099a5/2099b6/2099e1/2099f2 parent waits and second-button gate. This
 * must not be used to skip the children. Return2 before209a0a (power) or
 *209b71 (cancel), recording boundary without executing either transition. */
int ntm_shot_fire_tick(ntm_shot_fire *,ntm_task *,ntm_shot_fire_context *,uint8_t pressed);
/* Genuine20af8a transition and20afa5 waiting/20b1d6 miniature refresh. Unlike
 * normal aiming, waiting never moves the removed main marker or changes the
 * angle. Lifecycle2 performs real task completion. Other lifecycle values
 * preserve the original waiting callback and dirty-gated miniature behavior.
 * The original marker initializer and pre-fire bounded API remain unchanged. */
int ntm_shot_fire_markers_tick(ntm_shot_markers *,ntm_task *,ntm_task_context *,
    ntm_shot_marker_context *,const ntm_shot_marker_resources *);
#endif
