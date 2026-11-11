/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_projection.h"
#include "shot_markers.h"
#include "shot_math.h"
static uint32_t sar(uint32_t n,unsigned bits)
{return (n>>bits)|((n&0x80000000u)?(0xffffffffu<<(32-bits)):0);}
static uint32_t divide(uint32_t a,uint32_t b)
{
    unsigned negative=(a^b)>>31;uint32_t q;
    if(a&0x80000000u)a=0u-a;
    if(b&0x80000000u)b=0u-b;
    q=ntm_shot_divide(a,b);return negative?0u-q:q;
}
int ntm_shot_project(uint32_t x,uint32_t y,uint32_t ox,uint32_t oy,
    uint32_t dx,uint32_t dy,ntm_shot_projection *out)
{
    uint32_t lateral,depth,denominator,perspective,projected;
    if(!out)return -1;
    x-=ox;y-=oy;
    lateral=ntm_shot_marker_multiply(dx,y)-ntm_shot_marker_multiply(x,dy);
    depth=sar(ntm_shot_marker_multiply(dy,y)+ntm_shot_marker_multiply(dx,x),2);
    denominator=depth+4u*65536u;
    projected=divide(ntm_shot_marker_multiply(depth,55u*65536u),denominator);
    out->depth=sar(divide(projected,43u*65536u),2);
    out->y=(uint16_t)(120u-(projected>>16));
    perspective=divide(0x00f2db6du,denominator);
    out->x=(uint16_t)(ntm_shot_marker_multiply(sar(lateral,2),perspective)>>16);
    out->lateral_scratch=lateral;out->depth_scratch=denominator;return 0;
}
