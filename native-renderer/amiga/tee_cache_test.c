/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "tee_planar_cache.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t packed[25000],output[SELECTOR_PLANAR_BYTES],saved[SELECTOR_PLANAR_BYTES];
static tee_planar_entry entry;
static unsigned polls;
static void pump(void){polls++;}
static void failure(unsigned size)
{
    entry.packed_size=size;memset(output,0xa5,sizeof output);memcpy(saved,output,sizeof output);
    assert(tee_planar_unpack(&entry,output,pump)==-1);assert(!memcmp(saved,output,sizeof output));
}
int main(void)
{
    unsigned i,j,at=0,n=0;entry.packed=packed;
    while(n<sizeof output){unsigned count=sizeof output-n;if(count>128)count=128;packed[at++]=(uint8_t)(257-count);packed[at++]=(uint8_t)n;n+=count;}
    entry.packed_size=at;assert(!tee_planar_unpack(&entry,output,pump));assert(polls==94);
    for(i=0;i<sizeof output;i++)assert(output[i]==(uint8_t)(i/128*128));
    for(i=0;i<at;i++)failure(i);
    packed[at++]=0;packed[at++]=1;failure(at);packed[0]=128;failure(at);
    at=n=0;while(n<sizeof output){unsigned count=sizeof output-n;if(count>128)count=128;packed[at++]=(uint8_t)(count-1);for(j=0;j<count;j++)packed[at++]=(uint8_t)(n+j);n+=count;}
    entry.packed_size=at;polls=0;assert(!tee_planar_unpack(&entry,output,pump));assert(polls==94);
    for(i=0;i<sizeof output;i++)assert(output[i]==(uint8_t)i);
    for(i=at-128;i<at;i++)failure(i);
    assert(tee_planar_unpack(0,output,0)==-1);assert(tee_planar_unpack(&entry,0,0)==-1);
    entry.packed=0;assert(tee_planar_unpack(&entry,output,0)==-1);
    puts("tee cache decoder: literal/run boundaries, truncated/overflow/noop rejection, atomic failure and polling pass");return 0;
}
