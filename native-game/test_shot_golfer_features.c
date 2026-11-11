/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_golfer_features.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void putw(uint8_t *p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void putl(uint8_t *p,uint32_t v){unsigned i;for(i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static uint32_t middle_reference(uint32_t a,uint32_t b)
{
    int64_t x=a&0x80000000u?(int64_t)a-4294967296LL:a;
    int64_t y=b&0x80000000u?(int64_t)b-4294967296LL:b,p=x*y;
    return (uint32_t)(p>=0?p/65536:-((-p+65535)/65536));
}
static uint32_t random_word(uint32_t *s){*s=*s*1664525u+1013904223u;return *s;}
int main(void)
{
    static const uint16_t centers[6]={0,99,100,32767,32768,65535};
    static const int edges[7]={-101,-100,-99,0,99,100,101};
    static uint8_t angles[2050];ntm_shot_aim_resources aim={0};
    ntm_hole_prepare_state h={0};ntm_shot_golfer_feature_resources r={0};
    ntm_shot_golfer_features s,before;uint8_t records[101];unsigned i,j,k,cases=0;int dx;
    aim.angles=angles;aim.angles_size=sizeof(angles);r.aim=&aim;r.features=records;
    for(i=0;i<6;i++)for(dx=-101;dx<=101;dx++)for(j=0;j<7;j++) {
        unsigned visible=dx>=-100 && dx<=100 && edges[j]>=-100 && edges[j]<=100;
        memset(&s,165,sizeof(s));before=s;memset(&h,0,sizeof(h));
        putl(h.hole+0x44,(uint32_t)centers[i]<<16);putl(h.hole+0x48,(uint32_t)centers[5-i]<<16);
        records[0]=7;putw(records+1,(uint16_t)(centers[i]+dx));putw(records+3,(uint16_t)(centers[5-i]+edges[j]));records[5]=255;r.features_size=6;
        assert(!ntm_shot_golfer_features_select(&s,&h,&r));
        assert(s.origin_x==(uint16_t)(centers[i]-100u) && s.origin_y==(uint16_t)(centers[5-i]-100u) && s.extent==200);
        if(visible)assert(!memcmp(s.nearby,records,6));else assert(s.nearby[0]==255 && !memcmp(s.nearby+1,before.nearby+1,80));
        assert(s.view[0]==255 && s.view[5]==255 && !memcmp(s.view+1,before.view+1,4));
        assert(!memcmp(s.view+6,before.view+6,75));
        assert(s.lateral_bias==(visible?0:before.lateral_bias) && s.depth_bias==(visible?0:before.depth_bias));
        assert(s.lateral_scratch==before.lateral_scratch && s.depth_scratch==before.depth_scratch);cases++;
    }
    memset(&h,0,sizeof(h));putl(h.hole+0x44,100u<<16);putl(h.hole+0x48,100u<<16);putl(h.hole+0xd4,65536);
    for(i=0;i<4;i++){records[5*i]=(uint8_t)(i+1);putw(records+5*i+1,100u+(i==0?10u:i==1?20u:5u));putw(records+5*i+3,100);}
    records[20]=255;r.features_size=21;memset(&s,165,sizeof(s));
    assert(!ntm_shot_golfer_features_select(&s,&h,&r));
    assert(!memcmp(s.nearby,records,21));assert(s.view[0]==4 && word(s.view+1)==0 && word(s.view+3)==5 && s.view[5]==255);cases++;
    /* Original16-entry cap does not read a seventeenth feature. */
    for(i=0;i<20;i++){records[5*i]=(uint8_t)i;putw(records+5*i+1,105);putw(records+5*i+3,100);}
    records[100]=255;r.features_size=80;memset(&s,165,sizeof(s));
    assert(!ntm_shot_golfer_features_select(&s,&h,&r));assert(!memcmp(s.nearby,records,80) && s.nearby[80]==255 && s.view[0]==15);cases++;
    /* Truncated lists and missing angle input reject the whole plan. */
    for(k=0;k<5;k++) {
        memset(&s,165,sizeof(s));before=s;r.features_size=k;
        assert(ntm_shot_golfer_features_select(&s,&h,&r)==-1 && !memcmp(&s,&before,sizeof(s)));
    }
    r.features_size=80;r.aim=0;memset(&s,165,sizeof(s));before=s;
    assert(ntm_shot_golfer_features_select(&s,&h,&r)==-1 && !memcmp(&s,&before,sizeof(s)));r.aim=&aim;
    /* Independent64-bit test arithmetic is confined to this fixture, never
     * linked into the target module. Exercise true signed product middle bits. */
    {
        uint32_t seed=21081,x,y,dirx,diry;
        for(i=0;i<2048;i++) {
            x=random_word(&seed);y=random_word(&seed);dirx=random_word(&seed);diry=random_word(&seed);
            putl(h.hole+0x44,x);putl(h.hole+0x48,y);putl(h.hole+0xd4,dirx);putl(h.hole+0xd8,diry);
            records[0]=1;putw(records+1,(uint16_t)(x>>16));putw(records+3,(uint16_t)(y>>16));records[5]=255;r.features_size=6;
            memset(&s,165,sizeof(s));assert(!ntm_shot_golfer_features_select(&s,&h,&r));
            assert(s.lateral_bias==middle_reference(x,diry)-middle_reference(y,dirx));
            assert(s.depth_bias==middle_reference(y,diry)+middle_reference(x,dirx));cases++;
        }
    }
    printf("shot golfer features:%u authored collection/selection/arithmetic cases PASS\n",cases);return 0;
}
