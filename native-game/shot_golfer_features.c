/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_golfer_features.h"
#include "shot_math.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static uint32_t lng(const uint8_t *p){return (uint32_t)word(p)|((uint32_t)word(p+2)<<16);}
static void putw(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static int greater(uint16_t a,uint16_t b){return (a^32768u)>(b^32768u);}
/* Middle32 bits of signed full64-bit20d1b2 product. Unlike20d1e9's tiny
 * operand fast path this always retains the true two's-complement floor. */
static uint32_t middle(uint32_t a,uint32_t b)
{
    uint32_t v=ntm_shot_multiply(a,b);
    if(a&0x80000000u)v-=b<<16;
    if(b&0x80000000u)v-=a<<16;
    return v;
}
int ntm_shot_golfer_features_select(ntm_shot_golfer_features *s,
    const ntm_hole_prepare_state *h,const ntm_shot_golfer_feature_resources *r)
{
    ntm_shot_golfer_features p;size_t at=0;unsigned count=0,visible=0,i,j,best=0;
    uint16_t x,y,delta,angle,min_x=4095,min_depth=4095;uint32_t dx,dy,depth;
    if(!s || !h || !r || !r->features || !r->features_size)return -1;
    p=*s;p.origin_x=(uint16_t)(word(h->hole+0x46)-100u);
    p.origin_y=(uint16_t)(word(h->hole+0x4a)-100u);p.extent=200;
    while(1) {
        if(at>=r->features_size)return -1;
        if(r->features[at]==255)break;
        if(r->features_size-at<5)return -1;
        x=word(r->features+at+1);y=word(r->features+at+3);
        delta=(uint16_t)(y-p.origin_y);
        if(!(delta&32768u) && delta<=p.extent) {
            delta=(uint16_t)(x-p.origin_x);
            if(!(delta&32768u) && delta<=p.extent) {
                for(j=0;j<5;j++)p.nearby[5*count+j]=r->features[at+j];
                if(++count==16)break;
            }
        }
        at+=5;
    }
    p.nearby[5*count]=255;p.view[0]=255;
    if(count) {
        dx=lng(h->hole+0xd4);dy=lng(h->hole+0xd8);
        p.lateral_bias=middle(lng(h->hole+0x44),dy)-middle(lng(h->hole+0x48),dx);
        p.depth_bias=middle(lng(h->hole+0x48),dy)+middle(lng(h->hole+0x44),dx);
        for(i=0;i<count;i++) {
            x=word(p.nearby+5*i+1);y=word(p.nearby+5*i+3);
            depth=p.depth_bias-dy*(uint32_t)y-dx*(uint32_t)x;
            if(!(depth&0x80000000u))continue;
            p.depth_scratch=0u-depth;
            p.lateral_scratch=p.lateral_bias+dx*(uint32_t)y-dy*(uint32_t)x;
            if(ntm_shot_aim_angle(0,0,(uint16_t)(p.lateral_scratch>>16),
                (uint16_t)(0u-(p.depth_scratch>>16)),r->aim,&angle)<0)return -1;
            if(((angle+512u)&4095u)>=1024)continue;
            p.view[5*visible]=p.nearby[5*i];putw(p.view+5*visible+1,(uint16_t)(p.lateral_scratch>>16));
            putw(p.view+5*visible+3,(uint16_t)(p.depth_scratch>>16));visible++;
        }
        p.view[5*visible]=255;
    }
    for(i=0;i<visible;i++) {
        x=word(p.view+5*i+1);if(x&32768u)x=(uint16_t)(0u-x);
        y=word(p.view+5*i+3);
        if(greater(x,min_x) || greater(y,min_depth))continue;
        best=5*i;min_x=x;min_depth=y;
    }
    for(j=0;j<5;j++)p.view[j]=p.view[best+j];
    p.view[5]=255;*s=p;return 0;
}
