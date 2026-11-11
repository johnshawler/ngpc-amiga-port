/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_hud.h"
#include "tee_graphics.h"
static uint32_t scale(uint32_t n)
{
    uint32_t m=(n&0x80000000u)?0u-n:n;
    uint32_t v=(m>>16)*40960u+(((m&65535u)*40960u)>>16);
    if((n&0x80000000u) && m>=65536u && (m&7u))v++;
    return (n&0x80000000u)?0u-v:v;
}
int ntm_shot_hud_initialize(ntm_shot_hud_context *c,const ntm_shot_hud_resources *r,
    uint32_t distance,uint8_t wind,uint8_t direction,uint8_t label)
{
    ntm_screen_stamp stamp;unsigned i,index=wind?direction:16;
    if(!c || !r || !c->screen || !c->maps || !c->tiles || !c->text_base ||
       !c->scratch || !c->saved || index>=17 || label!=r->label_id ||
       c->screen->stamp_bytes>256 || (c->screen->stamp_bytes&7))return -1;
    *c->text_base=0;
    if(ntm_tee_copy_tiles(&r->font,0x100,c->tiles)<0)return -1;
    stamp.asset=r->header;stamp.offset=0;stamp.tile=0x40;
    if(ntm_course_screen_stamp(&stamp,c->tiles,c->maps)<0)return -1;
    if(ntm_course_screen_enqueue_stamp(c->screen,&r->label,0x448,0xd6)<0)return -1;
    if(ntm_tee_draw_number(wind,2,0,0,0x1a,c->scratch,c->maps)<0)return -1;
    if(ntm_course_screen_enqueue_stamp(c->screen,r->wind+index,0x16,0x22d2)<0)return -1;
    if(ntm_tee_draw_number((uint16_t)(scale(distance)>>16),3,0,0,0xc,c->scratch,c->maps)<0)return -1;
    for(i=0;i<4;i++)c->saved[i]=c->maps[0x40+i];
    return 0;
}
int ntm_shot_hud_clear(uint8_t maps[4096])
{
    unsigned i;if(!maps)return -1;
    for(i=0;i<7;i++){maps[0x46+2*i]=maps[0x44];maps[0x47+2*i]=maps[0x45];}
    return 0;
}
