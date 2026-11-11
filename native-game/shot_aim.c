/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_aim.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static uint32_t lng(const uint8_t *p){return (uint32_t)word(p)|((uint32_t)word(p+2)<<16);}
static int gt(uint16_t a,uint16_t b){return (unsigned)(a^32768u)>(unsigned)(b^32768u);}
int ntm_shot_aim_angle(uint16_t x,uint16_t y,uint16_t tx,uint16_t ty,
    const ntm_shot_aim_resources *r,uint16_t *result)
{
    uint16_t a=(uint16_t)(x-tx),b=(uint16_t)(y-ty),v,t;unsigned flags=0,index;
    if(!r || !result || !r->angles || r->angles_size<2050)return -1;
    if(!gt(x,tx)){a=(uint16_t)(0u-a);flags|=1;}
    if(!gt(y,ty)){b=(uint16_t)(0u-b);flags|=2;}
    if(a>b){t=a;a=b;b=t;flags|=4;}
    if(!b){*result=0;return 0;}
    index=((uint32_t)a<<10)/b;v=word(r->angles+2u*index);
    if(flags&4)v=(uint16_t)(1024u-v);
    if(flags&2)v=(uint16_t)(2048u-v);
    if(!(flags&1))v=(uint16_t)(4096u-v);
    *result=(uint16_t)(v&4095u);return 0;
}
uint8_t ntm_shot_aim_at_tee(const uint8_t h[256])
{
    uint16_t x,y;if(!h)return 0;
    x=(uint16_t)(word(h+0x46)-word(h+0x2e));y=(uint16_t)(word(h+0x4a)-word(h+0x30));
    if(x&32768u)x=(uint16_t)(0u-x);
    if(x>10)return 0;
    if(y&32768u)y=(uint16_t)(0u-y);
    return (uint8_t)(y<=7);
}
int ntm_shot_aim_club(ntm_shot_aim *s,const ntm_hole_prepare_state *h,
    const ntm_shot_player_state *p,const ntm_shot_aim_resources *r)
{
    unsigned golfer,power,index,i;uint16_t distance,mid;uint8_t club,minimum;const uint8_t *table,*player;
    if(!s || !h || !p || !r || p->active_player>1)return -1;
    player=h->players[p->active_player];golfer=player[1];power=(uint8_t)(golfer+player[0x33]);
    index=(uint8_t)((uint8_t)(h->hole[0x70]+player[3])^128u);
    if(golfer>=6 || power>=12 || !r->powers[power] || r->power_sizes[power]<52 ||
       !r->minimums[golfer] || index>=r->minimum_sizes[golfer])return -1;
    table=r->powers[power];distance=word(h->hole+0x6a);
    for(i=0;i<13 && distance<=word(table+4*i);i++){}
    if(i==13)club=12;
    else if(!i)club=0;
    else {club=(uint8_t)(i-1);mid=(uint16_t)(word(table+4*i-4)+word(table+4*i));mid>>=1;if(distance<=mid)club++;}
    minimum=r->minimums[golfer][index];if(club<minimum)club=minimum;
    if(!club && !ntm_shot_aim_at_tee(h->hole))club=1;
    s->club=club;return 0;
}
int ntm_shot_aim_camera(ntm_shot_aim *s,const uint8_t h[256])
{
    uint16_t x,y,offset=112;if(!s || !h)return -1;
    if((uint16_t)(s->angle-1024u)<=2048u)offset=80;
    x=(uint16_t)(word(h+0x46)-56u);y=(uint16_t)(word(h+0x4a)-offset);
    if(x&32768u)x=0;else if(gt(x,word(h+0x2a)))x=word(h+0x2a);
    if(y&32768u)y=0;else if(gt(y,word(h+0x2c)))y=word(h+0x2c);
    s->origin_x=(uint16_t)(x-16u);s->origin_y=y;return 0;
}
int ntm_shot_aim_prepare(ntm_shot_aim *s,ntm_hole_prepare_state *h,
    const ntm_shot_player_state *p,const ntm_shot_aim_resources *r,uint8_t *status,uint16_t screen[2])
{
    uint32_t x,y,tx,ty;uint16_t angle;ntm_shot_aim next;
    if(!s || !h || !status || !screen)return -1;
    /* Validate resource dependencies before mutating any owned state. */
    next=*s;
    if(h->hole[0x70]!=3) {
        x=lng(h->hole+0x44);y=lng(h->hole+0x48);tx=lng(h->hole+0x18);ty=lng(h->hole+0x1c);
        if(!gt(word(h->hole+0x6a),96)){x<<=4;y<<=4;tx<<=4;ty<<=4;}
        if(ntm_shot_aim_angle((uint16_t)(x>>16),(uint16_t)(y>>16),(uint16_t)(tx>>16),(uint16_t)(ty>>16),r,&angle)<0 ||
           ntm_shot_aim_club(&next,h,p,r)<0)return -1;
        next.angle=next.initial_angle=angle;next.angle_delta=0;
        if(ntm_shot_aim_camera(&next,h->hole)<0)return -1;
    }
    next.reset_d1=next.reset_d2=next.reset_d3=next.reset_e2=next.reset_e4=next.reset_e6=next.reset_4416=0;
    h->hole[0x78]=19;*s=next;
    if(h->hole[0x70]==3)return 2;
    *status=0;screen[0]=screen[1]=0;return 0;
}
