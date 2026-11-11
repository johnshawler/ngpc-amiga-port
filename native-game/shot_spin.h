/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_SPIN_H
#define NTM_SHOT_SPIN_H
#include "shot_scene_objects.h"
#include "task_lifecycle.h"
typedef struct {const uint8_t *bounds;size_t bounds_size;} ntm_shot_spin_resources;
typedef struct {
    ntm_task_callback wait;
    uint8_t minimum,maximum,current,phase;
} ntm_shot_spin;
/* Original20b604/20db7c. Borrowed twelve two-byte bounds records, no ROM
 * addresses or data ownership in native state. Installs caller's20b62d adapter. */
int ntm_shot_spin_initialize(ntm_shot_spin *,ntm_task *,ntm_hole_prepare_state *,
    const ntm_shot_player_state *,const ntm_shot_spin_resources *);
/* Bounded original20b62d: lifecycle2 waits unconditionally. Other states
 * except3 wait only with input suppressed or no up/down edge, no4277 timer, and unchanged
 * spin. Returns NTM_TASK_BOUNDARY BEFORE unconverted input/timer/redraw.
 * Lifecycle3 calls the verified task finish helper. No swing transition is invented. */
int ntm_shot_spin_tick(const ntm_shot_spin *,const ntm_hole_prepare_state *,
    uint8_t lifecycle,uint8_t input_gate,uint8_t pressed,ntm_task_context *,ntm_task *);
typedef struct {ntm_screen_asset indicators[9],highlights[9];} ntm_shot_spin_draw_resources;
/* Complete20b62d control,20b64a adjustment and20b6b0 redraw/restore.
 * Out-of-range corrupt spin values requiring unresolved assets return boundary. */
int ntm_shot_spin_control(ntm_shot_spin *,ntm_shot_scene_object_context *,
    const ntm_shot_spin_draw_resources *,uint8_t lifecycle,uint8_t input_gate,
    uint8_t pressed,ntm_task_context *,ntm_task *);
#endif
