/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "scene_renderer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t scalar[24*152*8],packed[24*152*8],dirty[24*152*8];
static nr_planar_cache cache;
int main(void)
{
    nr_bitmap a={scalar,sizeof scalar,160,152,24},b={packed,sizeof packed,160,152,24},c={dirty,sizeof dirty,160,152,24};
    uint8_t row[160];unsigned n,i,y;uint32_t random=1234;
    memset(scalar,0xa5,sizeof scalar);memset(packed,0xa5,sizeof packed);memset(dirty,0xa5,sizeof dirty);nr_planar_cache_init(&cache);
    for(n=0;n<65536;n++) {
        y=n%152;
        for(i=0;i<160;i++) {random=random*1664525u+1013904223u;row[i]=(uint8_t)(random>>24);}
        row[0]=(uint8_t)n;row[1]=(uint8_t)(n>>8);
        assert(nr_store_scene_row(&a,y,row)==0);assert(nr_store_scene_row_packed(&b,y,row)==0);
        assert(nr_store_scene_row_dirty(&c,y,row,&cache)==0);
        assert(nr_store_scene_row_dirty(&c,y,row,&cache)==0);
        if(n%257==0) {assert(memcmp(scalar,packed,sizeof scalar)==0);assert(memcmp(scalar,dirty,sizeof scalar)==0);}
    }
    assert(memcmp(scalar,packed,sizeof scalar)==0);assert(memcmp(scalar,dirty,sizeof scalar)==0);
    assert(cache.skipped_blocks>=65536u*20);
    /* Explicit invalidate, destination ownership and stride changes. */
    nr_planar_cache_init(&cache);memset(dirty,0xa5,sizeof dirty);
    assert(nr_store_scene_row_dirty(&c,y,row,&cache)==0);assert(cache.converted_blocks==20);
    c.data=packed;assert(nr_store_scene_row_dirty(&c,y,row,&cache)==0);assert(cache.converted_blocks==40);
    c.stride=22;assert(nr_store_scene_row_dirty(&c,y,row,&cache)==0);assert(cache.converted_blocks==60);
    memset(packed,0xa5,sizeof packed);b.capacity=0;
    assert(nr_store_scene_row_packed(&b,0,row)==-1);assert(nr_store_scene_row_dirty(&b,0,row,&cache)==-1);
    for(i=0;i<sizeof packed;i++)assert(packed[i]==0xa5);
    puts("PASS: 65536 packed/dirty rows, repeat skips and invalidation guards");return 0;
}
