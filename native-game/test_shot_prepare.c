/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_math.h"
#include "shot_prepare.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t traits[256],low[16],step[16],limit[18],randoms[256];
static uint32_t random_state=47;
static uint32_t next(void){random_state=random_state*1664525u+1013904223u;return random_state;}
static void putl(uint8_t *p,uint32_t v){unsigned i;for(i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static uint32_t divide_reference(uint32_t a,uint32_t b)
{
    uint64_t dividend=(uint64_t)a<<16,quotient=0,remainder=0;int bit;
    if(!b)b=1;
    for(bit=47;bit>=0;bit--){remainder=(remainder*2+((dividend>>bit)&1))&0xffffffffu;quotient*=2;if(remainder>=b){remainder-=b;quotient++;}}
    return (uint32_t)quotient;
}
static uint32_t multiply_reference(uint32_t a,uint32_t b){return (uint32_t)(((uint64_t)a*b)>>16);}
static uint32_t distance_reference(uint32_t a,uint32_t b)
{
    uint32_t q,t,d;if(a>b){t=a;a=b;b=t;}if(!b)return a;
    q=(uint32_t)(((uint64_t)a<<16)/b);q=(uint32_t)(((uint64_t)q*q)>>16);
    t=(uint32_t)(((uint64_t)q<<16)/(q+262144u));d=b+multiply_reference(2*t,b);
    q=(uint32_t)(((uint64_t)multiply_reference(a,t)<<16)/d);q=(uint32_t)(((uint64_t)q*q)>>16);
    t=(uint32_t)(((uint64_t)q<<16)/(q+262144u));return d+multiply_reference(2*t,d);
}
int main(void)
{
    ntm_shot_prepare_resources r={0};ntm_hole_prepare_state h,e,before;ntm_shot_player_state p,pe;
    unsigned i,j,selection,n=0,stroke=0,wind=0;uint8_t cursor;
    static const uint32_t edge[]={0,1,65535,65536,65537,0x7fffffffu,0x80000000u,0x80000001u,0xffff0000u,0xffffffffu};
    for(i=0;i<100010;i++){
        uint32_t a=i<10?edge[i]:next(),b=i<10?edge[9-i]:next();
        assert(ntm_shot_divide(a,b)==divide_reference(a,b));assert(ntm_shot_multiply(a,b)==multiply_reference(a,b));
        a&=0xffffffu;b&=0xffffffu;
        assert(ntm_shot_distance(a,b,0,0)==distance_reference(a,b));
        assert(ntm_shot_distance(0,0,a,b)==distance_reference(a,b));
    }
    for(i=0;i<256;i++){traits[i]=(uint8_t)(i*7);randoms[i]=(uint8_t)(i*73+19);}
    for(i=0;i<16;i++){low[i]=(uint8_t)(i-7);step[i]=(uint8_t)(2*i-16);}
    for(i=0;i<18;i++)limit[i]=(uint8_t)(i%16);
    r.traits=traits;r.traits_size=256;r.wind_low=low;r.wind_step=step;r.wind_limit=limit;r.wind_low_size=r.wind_step_size=16;r.wind_limit_size=18;
    for(selection=0;selection<=8;selection+=4)for(i=0;i<256;i++) {
        uint32_t x=next()&0xffffffu,y=next()&0xffffffu;
        memset(&h,0xa5,sizeof h);memset(&p,0,sizeof p);p.selection=(uint16_t)selection;
        h.players[0][1]=(uint8_t)i;h.players[0][0x33]=23;h.players[1][1]=19;h.players[1][0x33]=(uint8_t)i;
        putl(h.players[0]+4,x);putl(h.players[0]+8,y);putl(h.hole+0x18,0);putl(h.hole+0x1c,0);e=h;pe=p;pe.other_player=1;
        for(j=0;j<2;j++)e.players[j][0x23]=traits[(uint8_t)((uint8_t)(h.players[j][1]+h.players[j][0x33])^128u)];
        memcpy(e.hole+0x44,h.players[0]+4,12);memcpy(e.hole+0x50,h.players[0]+4,8);memset(e.hole+0x58,0,16);
        putl(e.hole+0x68,distance_reference(x,y));e.hole[0x6c]=e.hole[0x6d]=0;
        assert(!ntm_shot_prepare_geometry(&p,&h,&r));assert(!memcmp(&h,&e,sizeof h)&&!memcmp(&p,&pe,sizeof p));n++;
    }
    for(selection=0;selection<2;selection++)for(i=0;i<256;i++)for(j=0;j<256;j++) {
        memset(&h,0xa5,sizeof h);memset(&p,0,sizeof p);p.active_player=(uint8_t)selection;
        h.players[selection][2]=(uint8_t)i;h.players[selection][42]=(uint8_t)j;h.hole[0x73]=(uint8_t)((i&1)?3:4);e=h;pe=p;
        e.players[selection][2]=(uint8_t)(i+1);pe.stroke_snapshot=(uint8_t)(i+1);
        if(j || h.hole[0x73]==3)e.players[selection][42]=(uint8_t)(j+1);
        assert(!ntm_shot_advance_stroke(&p,&h));assert(!memcmp(&h,&e,sizeof h)&&!memcmp(&p,&pe,sizeof p));stroke++;
    }
    for(selection=0;selection<4;selection++)for(i=0;i<64;i++)for(j=0;j<256;j++) {
        unsigned index=(j+i)%18,draw1=randoms[j^128]&15,draw2=randoms[((j+1)&255)^128]&15;uint8_t value;const uint8_t *table;
        memset(&h,0xa5,sizeof h);h.alternate=(uint8_t)(selection&1);h.secondary_mode=(uint8_t)((selection&2)?2:0);
        h.difficulty=(uint8_t)(i/8);h.selected_difficulty=(uint8_t)(i%8);h.hole[6]=(uint8_t)index;
        h.hole[0x6e]=(uint8_t)j;h.hole[0x6f]=(uint8_t)(j*17);e=h;cursor=(uint8_t)j;
        table=((i/8+i%8)/2)<3?low:step;value=(uint8_t)(j+table[draw1]);if(value&128)value=0;
        if(!h.alternate && h.secondary_mode!=2){if(value>limit[index])value=limit[index];table=limit;}
        if(value>15)value=15;
        e.hole[0x6e]=value;e.hole[0x6f]=(uint8_t)((h.hole[0x6f]+table[draw2])&15);
        assert(!ntm_shot_update_wind(&h,&r,&cursor,randoms));assert(!memcmp(&h,&e,sizeof h));assert(cursor==(uint8_t)(j+2));wind++;
    }
    before=h;cursor=25;h.hole[6]=18;e=h;assert(ntm_shot_update_wind(&h,&r,&cursor,randoms)==-1);assert(!memcmp(&h,&e,sizeof h)&&cursor==25);
    h=before;p.selection=12;e=h;pe=p;assert(ntm_shot_prepare_geometry(&p,&h,&r)==-1);assert(!memcmp(&h,&e,sizeof h)&&!memcmp(&p,&pe,sizeof p));
    r.traits_size=0;p.selection=0;pe=p;assert(ntm_shot_prepare_geometry(&p,&h,&r)==-1);assert(!memcmp(&h,&e,sizeof h)&&!memcmp(&p,&pe,sizeof p));
    printf("shot preparation:100010 arithmetic vectors,%u geometry,%u stroke,%u wind exact states\n",n,stroke,wind);return 0;
}
