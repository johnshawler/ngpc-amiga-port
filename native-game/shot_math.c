/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_math.h"
uint32_t ntm_shot_divide(uint32_t n,uint32_t d)
{
    uint32_t q=n<<16,r=0;uint16_t high=(uint16_t)(n>>16);unsigned i;
    if(!n)return 0;
    if(!d)d=1;
    /* The original unrolled48 rounds discard carry out of the remainder.
     * Preserve that behavior even outside ordinary positive game geometry. */
    for(i=0;i<48;i++) {
        uint32_t bit=(uint32_t)(high>>15);
        high=(uint16_t)((high<<1)|(q>>31));q<<=1;r=(r<<1)|bit;
        if(r>=d){q++;r-=d;}
    }
    return q;
}
uint32_t ntm_shot_multiply(uint32_t a,uint32_t b)
{
    uint32_t low=(a&65535u)*(b&65535u);
    return (low>>16)+(a>>16)*(b&65535u)+(b>>16)*(a&65535u)+((a>>16)*(b>>16)<<16);
}
static uint32_t square_ratio(uint32_t x)
{return x==65536u?x:((x&65535u)*(x&65535u))>>16;}
uint32_t ntm_shot_distance(uint32_t x,uint32_t y,uint32_t tx,uint32_t ty)
{
    uint32_t a=x-tx,b=y-ty,t,d,q;
    if(a&0x80000000u)a=0u-a;
    if(b&0x80000000u)b=0u-b;
    if((a^0x80000000u)>(b^0x80000000u)){t=a;a=b;b=t;}
    if(!b)return a;
    q=square_ratio(ntm_shot_divide(a,b));t=ntm_shot_divide(q,q+262144u);
    d=b+ntm_shot_multiply(t+t,b);
    q=square_ratio(ntm_shot_divide(ntm_shot_multiply(a,t),d));
    t=ntm_shot_divide(q,q+262144u);
    return d+ntm_shot_multiply(t+t,d);
}
