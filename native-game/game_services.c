/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "game_services.h"
int ntm_display_reset(ntm_display_reset_state *state)
{
    unsigned i;if(!state)return -1;
    for(i=0;i<16;i++)state->zero_flags[i]=0;
    state->zero_words[0]=state->zero_words[1]=0;state->capacity=64;state->limit=80;
    for(i=0;i<32;i++){state->object_ids[i]=(uint8_t)i;state->object_tags[i]=65535;state->list[i]=0;}
    for(i=0;i<256;i++)state->pools[0][i]=state->pools[1][i]=0;
    state->reset_words[0]=0x974c54f9u;state->reset_words[1]=0x4b4c5bf9u;return 0;
}
static uint16_t service_word(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
int ntm_service54(ntm_service54_state *state,const uint8_t payload[9])
{
    unsigned i,index;ntm_service54_record *record;
    if(!state || !payload)return -1;
    for(i=0;i<8;i++)if(state->slots[i]&32768)break;
    if(i==8)return 1;
    index=state->slots[i]&255;if(index>28 || (index&3))return -1;
    record=&state->records[index>>2];state->slots[i]=(uint16_t)(0x2000|index);record->command=84;
    if(state->mode>=16) {
        record->code=32;record->value8=(uint16_t)(480+payload[0]);
        record->value12=service_word(payload+1);record->value14=service_word(payload+3);record->value26=payload[5];
    }else {
        record->code=84;record->value8=service_word(payload+6);
        record->value12=(uint16_t)((record->value12&0xff00)|payload[8]);
    }
    state->status=65535;return 0;
}
