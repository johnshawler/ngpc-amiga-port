/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_READY_H
#define NTM_SHOT_READY_H
#include "task_lifecycle.h"
/* Normal pre-selector20d68c..20d6c2 player/input binding. The two source
 * input identities are diagnostics; native input pointers remain owned by
 * the application. Normal local mode only, no linked-player substitution. */
int ntm_shot_ready_bind_players(uint8_t players[2][64]);
/* Original2098ee/2098ff. Return2 immediately before209919 fire transition,
 * without fabricating its effects. Other returns:0 completed tick,-1 invalid.
 * Input is the active player's existing rising-edge byte, not a host sample. */
int ntm_shot_ready_tick(ntm_task *,ntm_task_callback delay,
    ntm_task_callback ready,uint8_t camera_busy,uint8_t pressed);
/* Bounded200472 main-loop admission. Normal/disabled information-overlay
 * states may continue. Return2 before the still-unconverted information
 * overlay or reset dispatch. No controls/state are consumed or changed. */
int ntm_shot_ready_input(uint8_t held,uint8_t pressed,uint8_t overlay);
#endif
