/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_COURSE_INITIALIZATION_H
#define NTM_COURSE_INITIALIZATION_H
#include "course_palette.h"
#include "course_driver.h"
typedef struct {
    uint16_t tag,y,width,velocity,tile,link;
    uint8_t id,second,byte14,byte15,flags,byte30;
} ntm_course_cursor;
typedef struct {
    uint8_t guard,flags,plane_swap,sprite_x,sprite_y,selection,mapped_course;
    uint16_t screen_words[2];
    ntm_course_cursor cursor;
} ntm_course_initialization;
/* Bounded color-mode initializer20921e. Caller owns persistent queues and state;
 * initializes their valid free-slot sentinels at application boot, not here.
 * assets: background0,background1,label0,label1,label2. Descriptor5bytes and
 * palette jobs supplied by the asset loader, never embedded original assets.
 * Returns0 initialized,1 guard bypass (caller must execute the alternate continuation),-1 invalid contract.
 * Actual cursor sprite realization remains the object-update service's job. */
int ntm_course_initialize(ntm_course_initialization *state,ntm_course_screen *screen,
    ntm_display_reset_state *display,ntm_course_palette *palette,
    uint8_t sprites[256],uint8_t sprite_palettes[64],const ntm_screen_asset assets[5],
    const uint8_t cursor_descriptor[5],const ntm_palette_job jobs[2]);
#endif
