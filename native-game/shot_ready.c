/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_ready.h"
int ntm_shot_ready_bind_players(uint8_t players[2][64])
{
    if(!players)return -1;
    players[0][0]=0;players[1][0]=1;
    players[0][0x31]=0xbf;players[0][0x32]=0x40;
    players[1][0x31]=0xcf;players[1][0x32]=0x40;return 0;
}
int ntm_shot_ready_tick(ntm_task *t,ntm_task_callback delay,
    ntm_task_callback ready,uint8_t camera_busy,uint8_t pressed)
{
    if(!t || !delay || !ready)return -1;
    if(t->callback==delay){
        t->delay=(uint16_t)(t->delay-1u);
        if(!(t->delay&32768u))return 0;
        t->callback=ready;
    }
    if(t->callback!=ready)return -1;
    return !camera_busy && (pressed&16u)?2:0;
}
int ntm_shot_ready_input(uint8_t held,uint8_t pressed,uint8_t overlay)
{
    if((held&48u)==48u && (pressed&64u))return 2;
    if(overlay&128u)return 0;
    if(overlay || (pressed&64u))return 2;
    return 0;
}
