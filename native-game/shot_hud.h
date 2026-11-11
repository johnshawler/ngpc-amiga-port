/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_HUD_H
#define NTM_SHOT_HUD_H
#include "course_screen.h"
typedef struct {
    ntm_screen_asset font,header,label,wind[17];
    uint8_t label_id;
} ntm_shot_hud_resources;
typedef struct {
    ntm_course_screen *screen;
    uint8_t *maps,*tiles;
    uint16_t *text_base;
    uint32_t *scratch;
    uint8_t *saved; /* Original451f, four bytes saved from map offset64. */
} ntm_shot_hud_context;
/* Original20a38a: immediate font/header, deferred label/wind icon, numerical
 * wind/distance, then saving the HUD prefix. Caller provides matching label.
 * Queue saturation drops requests exactly; valid input is the normal first
 * tee. Resource errors fail, never substitute art. Borrowed spans stay alive. */
int ntm_shot_hud_initialize(ntm_shot_hud_context *,const ntm_shot_hud_resources *,
    uint32_t distance,uint8_t wind,uint8_t direction,uint8_t label);
/* Original20a473 replicates map word0x44 into seven following words. */
int ntm_shot_hud_clear(uint8_t maps[4096]);
#endif
