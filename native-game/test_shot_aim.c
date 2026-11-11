/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_aim.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static uint8_t angles[2050],powers[12][52],minimums[6][256];
static ntm_shot_aim_resources resources;
static void putw(uint8_t *p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static uint32_t random_state=0x209751;
static uint32_t next(void){random_state=random_state*1664525u+1013904223u;return random_state;}
static int signed_word(uint16_t v){return v<32768u?(int)v:(int)v-65536;}
static uint16_t reference_angle(uint16_t x,uint16_t y,uint16_t tx,uint16_t ty)
{
    unsigned a=(uint16_t)(x-tx),b=(uint16_t)(y-ty),swap=0,value,t;
    int xs=signed_word(x)<=signed_word(tx),ys=signed_word(y)<=signed_word(ty);
    if(xs)a=(uint16_t)(0u-a);
    if(ys)b=(uint16_t)(0u-b);
    if(a>b){t=a;a=b;b=t;swap=1;}
    if(!b)return 0;
    value=(1024u*a/b)/2u; /* Authored synthetic angle table, not original bytes. */
    if(swap)value=(uint16_t)(1024u-value);
    if(ys)value=(uint16_t)(2048u-value);
    if(!xs)value=(uint16_t)(4096u-value);
    return (uint16_t)(value&4095);
}
static void init(void)
{
    unsigned i,j;resources.angles=angles;resources.angles_size=sizeof angles;
    for(i=0;i<1025;i++)putw(angles+2*i,i/2);
    for(i=0;i<12;i++){
        resources.powers[i]=powers[i];resources.power_sizes[i]=sizeof powers[i];
        for(j=0;j<13;j++)putw(powers[i]+4*j,300-20*j+i);
    }
    for(i=0;i<6;i++){resources.minimums[i]=minimums[i];resources.minimum_sizes[i]=sizeof minimums[i];}
}
int main(void)
{
    unsigned i,j;uint16_t a,screen[2]={41,42};uint8_t status=255;
    ntm_shot_aim s,prior;ntm_hole_prepare_state h;ntm_shot_player_state p;
    init();memset(&s,0,sizeof s);memset(&h,0,sizeof h);memset(&p,0,sizeof p);
    for(i=0;i<100000;i++) {
        uint16_t x=(uint16_t)next(),y=(uint16_t)(next()>>16),tx=(uint16_t)(next()>>8),ty=(uint16_t)next();
        assert(!ntm_shot_aim_angle(x,y,tx,ty,&resources,&a));assert(a==reference_angle(x,y,tx,ty));
    }
    for(i=0;i<65536;i++) {
        putw(h.hole+0x46,i);putw(h.hole+0x2e,0);putw(h.hole+0x4a,0);putw(h.hole+0x30,0);
        assert(ntm_shot_aim_at_tee(h.hole)==(i<=10 || i>=65526));
        putw(h.hole+0x46,0);putw(h.hole+0x4a,i);
        assert(ntm_shot_aim_at_tee(h.hole)==(i<=7 || i>=65529));
    }
    memset(&h,0,sizeof h);
    for(i=0;i<12;i++)for(j=0;j<512;j++) {
        unsigned club,k=0;h.players[0][1]=(uint8_t)(i%6);h.players[0][0x33]=(uint8_t)(i>=6?6:0);
        putw(h.hole+0x6a,j);
        while(k<13 && j<=300-20*k+i)k++;
        club=k==13?12:k? k-1+(j<=310-20*k+i):0;
        assert(!ntm_shot_aim_club(&s,&h,&p,&resources));assert(s.club==club);
    }
    memset(&h,0,sizeof h);memset(&s,255,sizeof s);h.hole[0x70]=4;
    assert(!ntm_shot_aim_prepare(&s,&h,&p,&resources,&status,screen));
    assert(!status && !screen[0] && !screen[1] && !s.angle && !s.initial_angle && !s.angle_delta && h.hole[0x78]==19);
    assert(!s.reset_d1 && !s.reset_d2 && !s.reset_d3 && !s.reset_e2 && !s.reset_e4 && !s.reset_e6 && !s.reset_4416);
    assert(s.origin_x==65520 && s.origin_y==0);
    h.players[0][1]=6;prior=s;h.hole[0x78]=12;status=17;screen[0]=18;
    assert(ntm_shot_aim_prepare(&s,&h,&p,&resources,&status,screen)==-1);
    assert(!memcmp(&s,&prior,sizeof s) && h.hole[0x78]==12 && status==17 && screen[0]==18);
    h.hole[0x70]=3;s.reset_e2=255;
    assert(ntm_shot_aim_prepare(&s,&h,&p,&resources,&status,screen)==2);
    assert(!s.reset_e2 && h.hole[0x78]==19 && status==17 && screen[0]==18);
    puts("shot aim:100000 angles,131072 tee-boundary checks,6144 club choices,exact setup and unsupported branch");return 0;
}
