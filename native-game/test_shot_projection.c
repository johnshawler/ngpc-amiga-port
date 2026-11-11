/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_projection.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    unsigned n,y,cases=0;ntm_shot_projection p;
    assert(ntm_shot_project(0,0,0,0,0,0,0)==-1);
    for(n=0;n<512;n++)for(y=0;y<16;y++){
        uint32_t projected=(55u*n*65536u)/(n+4u),ratio=0x00f2db6du/(n+4u);
        assert(!ntm_shot_project(4u*n*65536u,y*65536u,0,0,65536,0,&p));
        assert(p.x==(uint16_t)((y*ratio)/(4u*65536u)));
        assert(p.y==120u-(projected>>16));assert(p.depth==(projected/43u)/4u);
        assert(p.lateral_scratch==y*65536u && p.depth_scratch==(n+4u)*65536u);
        /* Translation of both coordinates/origin must leave the exact wrapped
         * difference and every projected output unchanged. */
        {ntm_shot_projection q;assert(!ntm_shot_project(4u*n*65536u+0xfedcba98u,y*65536u+0x12345678u,0xfedcba98u,0x12345678u,65536,0,&q));
         assert(q.x==p.x && q.y==p.y && q.depth==p.depth && q.lateral_scratch==p.lateral_scratch && q.depth_scratch==p.depth_scratch);}
        cases++;
    }
    printf("shot projection:%u rational/cardinal and%u wrapped-translation cases PASS\n",cases,cases);return 0;
}
