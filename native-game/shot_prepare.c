/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_prepare.h"
#include "shot_math.h"
#include "random_byte.h"
static uint32_t lng(const uint8_t *p)
{return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void putl(uint8_t *p,uint32_t v)
{unsigned i;for(i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
int ntm_shot_prepare_geometry(ntm_shot_player_state *s,ntm_hole_prepare_state *h,const ntm_shot_prepare_resources *r)
{
    unsigned i,bank,index[2];uint32_t x,y;
    if(!s || !h || !r || s->selection>8 || (s->selection&3) || !r->traits)return -1;
    for(bank=0;bank<2;bank++){index[bank]=(uint8_t)((uint8_t)(h->players[bank][1]+h->players[bank][0x33])^128u);if(index[bank]>=r->traits_size)return -1;}
    s->active_player=0;s->other_player=1;
    for(bank=0;bank<2;bank++)h->players[bank][0x23]=r->traits[index[bank]];
    for(i=0;i<12;i++)h->hole[0x44+i]=h->players[0][4+i];
    for(i=0;i<8;i++)h->hole[0x50+i]=h->players[0][4+i];
    for(i=0;i<16;i++)h->hole[0x58+i]=0;
    x=lng(h->hole+0x44);y=lng(h->hole+0x48);
    putl(h->hole+0x68,ntm_shot_distance(x,y,lng(h->hole+0x18),lng(h->hole+0x1c)));
    h->hole[0x6c]=h->hole[0x6d]=0;
    return 0;
}
int ntm_shot_advance_stroke(ntm_shot_player_state *s,ntm_hole_prepare_state *h)
{
    uint8_t *p;
    if(!s || !h || s->active_player>1)return -1;
    p=h->players[s->active_player];p[2]=(uint8_t)(p[2]+1);s->stroke_snapshot=p[2];
    if(p[0x2a] || h->hole[0x73]==3)p[0x2a]=(uint8_t)(p[0x2a]+1);
    return 0;
}
int ntm_shot_update_wind(ntm_hole_prepare_state *s,const ntm_shot_prepare_resources *r,
    uint8_t *cursor,const uint8_t window[256])
{
    unsigned average,draw;uint8_t value;const uint8_t *table;
    if(!s || !r || !cursor || !window || !r->wind_low || !r->wind_step || !r->wind_limit ||
       r->wind_low_size<16 || r->wind_step_size<16 || r->wind_limit_size<18 || s->hole[6]>=18)return -1;
    average=(uint8_t)(s->difficulty+s->selected_difficulty);average>>=1;
    table=average<3?r->wind_low:r->wind_step;
    draw=(unsigned)ntm_random_next(cursor,window)&15u;
    value=(uint8_t)(s->hole[0x6e]+table[draw]);
    if(value&128)value=0;
    if(!s->alternate && s->secondary_mode!=2) {
        if(value>r->wind_limit[s->hole[6]])value=r->wind_limit[s->hole[6]];
        /*20506a replaces XIX with the limit table. The later direction
         * lookup uses that changed pointer; XIY is not used by20505c. */
        table=r->wind_limit;
    }
    if(value>15)value=15;
    s->hole[0x6e]=value;
    draw=(unsigned)ntm_random_next(cursor,window)&15u;
    s->hole[0x6f]=(uint8_t)((s->hole[0x6f]+table[draw])&15);
    return 0;
}
