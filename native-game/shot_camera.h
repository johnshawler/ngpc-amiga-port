/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_CAMERA_H
#define NTM_SHOT_CAMERA_H
#include "shot_markers.h"
#include "aim_world.h"
typedef struct {uint16_t saved_x,saved_y;} ntm_shot_camera_state;
/* Original20c8a5 states0/2/4. pressed is active-player edge input; held is
 * context->held. Owns4417/4419, uses aim origin/reset_4416 and shared viewport.
 * Nonzero reset_d1 genuinely removes descendants and completes this task.
 * Returns0 on success,2 for unsupported state, -1 invalid. No frame advances,
 * tile writes, object registration or queue requests are fabricated. */
int ntm_shot_camera_active(ntm_shot_camera_state *,ntm_task *,ntm_task_context *,
    ntm_shot_marker_context *,const ntm_shot_marker_resources *,uint8_t pressed);
/* Original201608, called by the main loop BEFORE task dispatch when400d bit3
 * is set. Writes the two raw layer2 scroll bytes and only newly exposed blocks.
 * Uses the world loader's existing resources; no tile/palette reload. State
 * identity fields must match resources. Bounded validation precedes mutation.
 * Returns0 success,-1 malformed resources/state. */
int ntm_shot_camera_scroll(ntm_aim_world_state *,const ntm_aim_world_resources *,
    uint8_t maps[4096],uint8_t scroll[2]);
#endif
