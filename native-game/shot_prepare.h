/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_PREPARE_H
#define NTM_SHOT_PREPARE_H
#include "hole_prepare.h"
typedef struct {
    uint16_t selection; /* Original41ea table-byte offset. Normal entries0/4/8. */
    uint8_t active_player,other_player,stroke_snapshot;
} ntm_shot_player_state;
typedef struct {
    const uint8_t *traits;size_t traits_size; /* 256-byte window centred128 bytes before21ee81; signed byte indexing. */
    const uint8_t *wind_low,*wind_step,*wind_limit;
    size_t wind_low_size,wind_step_size,wind_limit_size;
} ntm_shot_prepare_resources;
/* Normal active-player selection20d774, both player-trait updates20d9fc,
 * coordinates20d87b, and2096cb..20970a including actual204ecd distance.
 * No terrain lookup, child allocation or shot progression is implied. */
int ntm_shot_prepare_geometry(ntm_shot_player_state *,ntm_hole_prepare_state *,const ntm_shot_prepare_resources *);
/* Original20d8b4: wrapped player counters, after terrain has set hole[0x73]. */
int ntm_shot_advance_stroke(ntm_shot_player_state *,ntm_hole_prepare_state *);
/* Original20500e and20506a: exactly two draws from the shared cursor. */
int ntm_shot_update_wind(ntm_hole_prepare_state *,const ntm_shot_prepare_resources *,uint8_t *random_cursor,const uint8_t random_window[256]);
#endif
