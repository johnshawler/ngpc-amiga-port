/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_hud.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static const uint8_t font[]={1,0,0,0,0,0,0,0};
static const uint8_t stamp[]={1,0,0,0,0,0,0,0,1,0,255};
static const uint8_t pixels[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
static uint32_t scale_reference(uint32_t n)
{
    uint32_t magnitude=n&0x80000000u?0u-n:n;
    uint64_t product=(uint64_t)magnitude*40960u;
    uint32_t out=(uint32_t)(product>>16);
    if((n&0x80000000u) && magnitude>=65536u && (product&65535u))out++;
    return n&0x80000000u?0u-out:out;
}
int main(void)
{
    ntm_shot_hud_resources r;ntm_shot_hud_context c;
    ntm_course_screen q;uint8_t maps[4096],tiles[8192],saved[4],before[4096];
    uint16_t text;uint32_t scratch;unsigned w,d,n,i,cases=0;
    memset(&r,0,sizeof r);r.font=(ntm_screen_asset){font,sizeof font,pixels,sizeof pixels};
    r.header=r.label=(ntm_screen_asset){stamp,sizeof stamp,pixels,sizeof pixels};r.label_id=4;
    for(i=0;i<17;i++)r.wind[i]=r.label;
    c=(ntm_shot_hud_context){&q,maps,tiles,&text,&scratch,saved};
    for(w=0;w<16;w++)for(d=0;d<16;d++)for(n=0;n<=32;n++){
        uint32_t distance=0x9e3779b9u*(cases+1),expected=scale_reference(distance)>>16;
        uint8_t digits[3];unsigned v=expected,j;
        memset(&q,0,sizeof q);q.stamp_bytes=(uint16_t)(8*n);memset(maps,0xa5,sizeof maps);memset(tiles,0x5a,sizeof tiles);
        text=0x1234;scratch=0xab765432;
        assert(!ntm_shot_hud_initialize(&c,&r,distance,(uint8_t)w,(uint8_t)d,4));
        assert(text==0 && q.stamp_bytes==8*(n>30?32:n+2));
        assert(!memcmp(tiles+0x100,pixels,16) && !memcmp(tiles+0x400,pixels,16));
        for(i=0;i<4;i++)assert(saved[i]==0xa5);
        for(i=3;i;i--){digits[i-1]=(uint8_t)(v%10);v/=10;}
        for(j=0;j<2 && !digits[j];j++)digits[j]=16;
        for(i=0;i<3;i++){assert(maps[12+2*i]==16+digits[i] && !maps[13+2*i]);assert(((scratch>>(8*i))&255)==digits[i]);}
        assert((scratch>>24)==0xab);
        if(n<32){assert(q.stamps[n].offset==0x448 && q.stamps[n].tile==0xd6);}
        if(n<31){assert(q.stamps[n+1].offset==0x16 && q.stamps[n+1].tile==0x22d2);}
        memcpy(before,maps,sizeof maps);maps[0x44]=0x34;maps[0x45]=0x12;assert(!ntm_shot_hud_clear(maps));
        for(i=0;i<7;i++)assert(maps[0x46+2*i]==0x34 && maps[0x47+2*i]==0x12);
        assert(!memcmp(before,maps,0x44) && !memcmp(before+0x54,maps+0x54,sizeof maps-0x54));cases++;
    }
    memcpy(before,maps,sizeof maps);assert(ntm_shot_hud_initialize(&c,&r,0,1,17,4)<0);assert(!memcmp(before,maps,sizeof maps));
    assert(ntm_shot_hud_initialize(&c,&r,0,0,0,5)<0);assert(ntm_shot_hud_clear(0)<0);
    printf("shot HUD: %u wind/direction/queue/arithmetic cases; invalid input rejection\n",cases);return 0;
}
