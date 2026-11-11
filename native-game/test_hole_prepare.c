/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "hole_prepare.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static ntm_hole_geometry geometry[54];
static uint8_t pins[64],quarter[4100],variation[22],threshold[8],wind[16],limit[18],random_table[256];
static const uint8_t hazards[]={5,1,11,0,22,0,33,0,44,0,255,255};
static unsigned word(const uint8_t *p){return p[0]+256u*p[1];}
static uint32_t lng(const uint8_t *p){return word(p)|((uint32_t)word(p+2)<<16);}
static void putw(uint8_t *p,unsigned n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static void putl(uint8_t *p,uint32_t n){putw(p,n);putw(p+2,n>>16);}
static uint32_t scale(uint32_t n){return (uint32_t)(((uint64_t)n*5u)/8u);}
static uint32_t dir(unsigned n){unsigned a=(n&15u)*256u,sign=0;if(a>2048)a=(0u-a)&2047u;if(a>1024){sign=1;a=(0u-a)&1023u;}return sign?1024u-a:0u-(1024u-a);}
int main(void)
{
    ntm_hole_prepare_resources r;ntm_hole_prepare_state s,before,e;unsigned i,j,course,hole,start,diff,old,index,draws[6],n=0;uint8_t cursor;
    memset(&r,0,sizeof r);
    for(i=0;i<54;i++) {
        ntm_hole_geometry *g=geometry+i;g->pins=pins;g->pins_size=64;
        g->descriptor[0]=(uint8_t)i;g->descriptor[1]=(uint8_t)(3+i%3);putw(g->descriptor+2,300+i);
        putw(g->descriptor+4,100+i);putw(g->descriptor+6,900-i);g->descriptor[8]=240;g->descriptor[9]=15;
        putw(g->descriptor+10,200+i);putw(g->descriptor+12,400+i);g->dimensions[0]=12;g->dimensions[1]=20;
        g->map_index=(uint16_t)(i*18);g->aux_index=(uint16_t)(i*18+1000);
        g->descriptor_identity=0x210000+i*18;g->path_identity=0x220000+i*64;g->aux_identity=0x230000+i*8;
    }
    for(i=0;i<16;i++){putw(pins+4*i,i);putw(pins+4*i+2,2*i);wind[i]=(uint8_t)(i-8);}
    for(i=0;i<1025;i++)putl(quarter+4*i,i);
    for(i=0;i<22;i++)variation[i]=(uint8_t)(2*i);
    for(i=0;i<8;i++)threshold[i]=(uint8_t)(i+1);
    for(i=0;i<18;i++)limit[i]=(uint8_t)(3+i%13);
    for(i=0;i<256;i++)random_table[i]=(uint8_t)(i*73+19);
    r.holes=geometry;r.hole_count=54;r.hazards=hazards;r.hazards_size=sizeof hazards;r.quarter=quarter;r.quarter_size=sizeof quarter;
    r.variation=variation;r.variation_size=22;r.threshold=threshold;r.threshold_size=8;r.wind_low=r.wind_high=wind;r.wind_low_size=r.wind_high_size=16;r.wind_limit=limit;r.wind_limit_size=18;
    for(course=0;course<3;course++)for(hole=0;hole<18;hole++)for(start=0;start<256;start++)for(diff=0;diff<8;diff++) {
        const ntm_hole_geometry *g=geometry+18*course+hole;unsigned x,y,v,angle;
        old=(start+hole)&7u;index=hole;memset(&s,0x5a,sizeof s);s.difficulty=(uint8_t)diff;s.selected_difficulty=(uint8_t)old;s.alternate=s.secondary_mode=0;s.hole[6]=(uint8_t)index;
        s.order[2*index]=(uint8_t)course;s.order[2*index+1]=(uint8_t)hole;cursor=(uint8_t)start;e=s;
        for(j=0;j<6;j++)draws[j]=random_table[((start+j)&255u)^128u];
        i=18*course+hole;e.hole[4]=(uint8_t)course;e.hole[5]=(uint8_t)hole;
        putl(e.hole+0x10,g->descriptor_identity);e.hole[0x14]=(uint8_t)i;e.hole[0x15]=(uint8_t)(3+i%3);putw(e.hole+0x16,300+i);
        for(j=0;j<2;j++){putl(e.players[j]+12,0);putl(e.players[j]+4,(100+i)<<16);putl(e.players[j]+8,(900-i)<<16);e.players[j][2]=e.players[j][3]=e.players[j][34]=e.players[j][37]=e.players[j][42]=0;}
        putw(e.hole+0x2e,100+i);putw(e.hole+0x30,900-i);putw(e.hole+0x32,240);putw(e.hole+0x34,15);
        putw(e.hole+0x3c,200+i);putw(e.hole+0x3e,400+i);x=200+i+(draws[0]&15);y=400+i+2*(draws[0]&15);
        putw(e.hole+0x1a,x);putw(e.hole+0x1e,y);putl(e.hole+0x20,scale(lng(e.hole+0x18)));putl(e.hole+0x24,scale(lng(e.hole+0x1c)));
        putl(e.hole+0x36,g->path_identity);putl(e.hole+0x40,g->aux_identity);putw(e.hole+0x28,g->map_index);putw(e.hole+0x3a,g->aux_index);putw(e.hole+0x2a,272);putw(e.hole+0x2c,488);e.ready_flag=255;
        e.auxiliary[15]=(uint8_t)(index>=diff+1?2*((draws[1]&7)+(draws[2]&7)+old):0);angle=draws[3]&15;e.auxiliary[16]=(uint8_t)angle;putl(e.hole+0xec,dir(angle+4));putl(e.hole+0xf0,dir(angle));
        putw(e.auxiliary+7,65535);putw(e.auxiliary+9,65535);e.auxiliary[11]=0;putw(e.auxiliary+12,32767);
        if(course==1 && hole==5){putw(e.auxiliary+7,11);putw(e.auxiliary+9,22);putw(e.auxiliary+11,33);putw(e.auxiliary+12,44);}
        e.selected_difficulty=(uint8_t)diff;e.hole[0x6f]=(uint8_t)(draws[4]&15);v=(uint8_t)(diff+wind[draws[5]&15]);if(v&128)v=0;if(v>limit[index])v=limit[index];if(v>15)v=15;e.hole[0x6e]=(uint8_t)v;
        assert(ntm_hole_prepare(&s,&r,&cursor,random_table)==0);assert(!memcmp(&s,&e,sizeof s));assert(cursor==(uint8_t)(start+6));n++;
    }
    /* Preserve the original short/full signed-multiply rounding split. */
    memset(pins,0,sizeof pins);putw(geometry[53].descriptor+10,65535);putw(geometry[53].descriptor+12,65534);
    putw(s.hole+0x18,32768);putw(s.hole+0x1c,32769);cursor=0;
    assert(ntm_hole_prepare(&s,&r,&cursor,random_table)==0);
    assert(lng(s.hole+0x20)==0xffffb000u && lng(s.hole+0x24)==0xffff1000u);
    before=s;cursor=77;
    for(i=0;i<8;i++){
        ntm_hole_prepare_resources bad=r;s=before;
        if(i==0)s.alternate=1;
        if(i==1)s.secondary_mode=1;
        if(i==2)s.difficulty=8;
        if(i==3)s.hole[6]=18;
        if(i==4)bad.hole_count=0;
        if(i==5)bad.quarter_size=4099;
        if(i==6)bad.hazards_size=10;
        if(i==7)bad.wind_limit=NULL;
        e=s;assert(ntm_hole_prepare(&s,&bad,&cursor,random_table)==-1);assert(!memcmp(&s,&e,sizeof s)&&cursor==77);
    }
    assert(ntm_hole_prepare(NULL,&r,&cursor,random_table)==-1);
    printf("hole preparation: %u exact-state cases and invalid-span rejection\n",n);return 0;
}
