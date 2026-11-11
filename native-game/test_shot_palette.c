/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_palette.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static ntm_course_palette p,saved;static ntm_shot_palette s,saved_s;
static uint8_t table[512],source[8],target[8];
static void start(void){unsigned i;memset(&p,0,sizeof p);memset(&s,0,sizeof s);for(i=0;i<8;i++)p.slots[i]=(uint16_t)(0xff00+4*i);p.pending[0]=p.pending[1]=255;}
static unsigned interpolation(unsigned from,unsigned to,unsigned level)
{
    unsigned result=0,shift;for(shift=0;shift<12;shift+=4){int a=(from>>shift)&15,b=(to>>shift)&15;result|=(unsigned)(a+(b-a)*(int)level/15)<<shift;}return result;
}
int main(void)
{
    unsigned level,color,tick,speed,cases=0;int delta;
    ntm_shot_palette_job job={40,16,0,486,0,0,0,0x118,0,0,table,0,0,512};
    for(level=0;level<16;level++)for(delta=-16;delta<16;delta++)table[16+32*level+delta]=(uint8_t)(delta*(int)level/15);
    for(color=0;color<4096;color++){
        start();job.source_color=(uint16_t)color;job.target_color=(uint16_t)(4095-color);job.speed=16;p.background=0xa8;
        assert(!ntm_shot_palette_enqueue(&p,&s,&job));
        for(level=0;level<16;level++){
            unsigned expected=interpolation(color,4095-color,level);assert(!ntm_shot_palette_tail(&p,&s));
            assert(p.copy_bytes==4 && p.copies[0].offset==486 && p.copies[0].words==1);
            assert(p.staging[486]==(expected&255) && p.staging[487]==(expected>>8));
            assert(s.level[0]==level && !s.fraction[0]);assert(p.pending[0]==0xab);
            assert(!ntm_course_palette_head(&p));assert(p.background==0xab && p.pending[0]==255);cases++;
        }assert((p.slots[0]&0xff00)==0xff00);
    }
    for(speed=0;speed<256;speed++){
        unsigned fraction=0,progress=255,active=1;start();job.speed=(uint8_t)speed;
        assert(!ntm_shot_palette_enqueue(&p,&s,&job));
        for(tick=0;tick<40;tick++){
            unsigned emitted=0;
            if(active){fraction=(fraction+speed)&255;if(fraction>=16){progress=(progress+(fraction>>4))&255;if(progress>=15)progress=15;fraction&=15;emitted=1;if(progress==15)active=0;}}
            assert(!ntm_shot_palette_tail(&p,&s));assert(s.fraction[0]==fraction && s.level[0]==progress);assert(p.copy_bytes==4*emitted);assert(!ntm_course_palette_head(&p));cases++;
        }
    }
    start();job.kind=24;job.speed=16;job.offset=0;job.count=1;job.source=source;job.source_size=8;job.target=target;job.target_size=8;
    memset(source,0,8);memset(target,15,8);memset(p.staging,0x5a,512);
    assert(!ntm_shot_palette_enqueue(&p,&s,&job));assert(!ntm_shot_palette_tail(&p,&s));assert(p.staging[0]==0x5a && p.staging[1]==0x5a);assert(p.copies[0].words==4);
    start();saved=p;saved_s=s;job.blend_size=511;assert(ntm_shot_palette_enqueue(&p,&s,&job)==-1);assert(!memcmp(&p,&saved,sizeof p)&&!memcmp(&s,&saved_s,sizeof s));job.blend_size=512;
    for(tick=0;tick<8;tick++)assert(!ntm_shot_palette_enqueue(&p,&s,&job));
    assert(ntm_shot_palette_enqueue(&p,&s,&job)==1);
    p.copy_bytes=4;saved=p;saved_s=s;assert(ntm_shot_palette_tail(&p,&s)==-1);assert(!memcmp(&p,&saved,sizeof p)&&!memcmp(&s,&saved_s,sizeof s));
    p.copy_bytes=0;s.jobs[7].source_size=7;saved=p;saved_s=s;assert(ntm_shot_palette_tail(&p,&s)==-1);assert(!memcmp(&p,&saved,sizeof p)&&!memcmp(&s,&saved_s,sizeof s));
    printf("PASS animated palettes: %u color/step states, transparent preservation, full queue and atomic invalid resources\n",cases);return 0;
}
