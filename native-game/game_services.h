/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_GAME_SERVICES_H
#define NTM_GAME_SERVICES_H
#include <stdint.h>
typedef struct {
    uint8_t zero_flags[16],capacity,limit,object_ids[32],pools[2][256];
    uint16_t zero_words[2],object_tags[32],list[32];
    uint32_t reset_words[2];
} ntm_display_reset_state;
/* Reset only the owned fields represented here; other object state is retained
 * separately by the application. No allocation, hardware or ROM access. */
int ntm_display_reset(ntm_display_reset_state *state);
typedef struct {uint16_t code,command,value8,value12,value14;uint8_t value26;} ntm_service54_record;
typedef struct {
    uint16_t slots[8],status;
    uint8_t mode;
    ntm_service54_record records[8];
} ntm_service54_state;
/* First-negative-slot allocation. Legal slot lowbytes are 0,4,...28, selecting
 * native records. Caller supplies private 9-byte descriptor payload. Returns
 * 0 allocated, 1 full (no mutation), -1 invalid bounded input (no mutation). */
int ntm_service54(ntm_service54_state *state,const uint8_t payload[9]);
#endif
