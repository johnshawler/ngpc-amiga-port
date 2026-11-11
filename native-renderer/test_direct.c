/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "scene_renderer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t tiles[8192],maps[4096],sprites[256],pal[64],a[24320],b[24320],c[24320];
int main(void)
{
    nr_scene s={tiles,maps,sprites,pal,{0,0},{0,0},0,0,0,0,8,1,0,2,5};
    nr_bitmap ba={a,sizeof a,160,152,20},bb={b,sizeof b,160,152,20},bc={c,sizeof c,160,152,20};
    nr_scene_cache cache;uint8_t row[160];unsigned n,p,i;
    nr_scene_cache_init(&cache);
    for(i=0;i<2048;i++){maps[i*2]=1;maps[i*2+1]=0;}
    memset(a,0xa5,sizeof a);memset(b,0xa5,sizeof b);memset(c,0xa5,sizeof c);
    for(n=0;n<131072;n++) {
        unsigned d=(((n>>8)&15)<<9)|((n&65536)?0x8000:0)|((n&1)?0x4000:0);
        maps[0]=(uint8_t)d;maps[1]=(uint8_t)(d>>8);
        tiles[0]=(uint8_t)n;tiles[1]=(uint8_t)(n>>8);tiles[14]=(uint8_t)~n;tiles[15]=(uint8_t)(~n>>8);
        assert(nr_scene_row(&s,0,row)==0);assert(nr_store_scene_row(&ba,0,row)==0);
        assert(nr_scene_row_planar(&s,0,&bb,&cache)==0);assert(nr_scene_row_planar_words(&s,0,&bc,&cache)==0);
        for(p=0;p<8;p++){assert(memcmp(a+p*3040,b+p*3040,20)==0);assert(memcmp(a+p*3040,c+p*3040,20)==0);}
    }
    assert(memcmp(a,b,sizeof a)==0);assert(memcmp(a,c,sizeof a)==0);
    memset(b,0xa5,sizeof b);bb.capacity=0;
    assert(nr_scene_row_planar(&s,0,&bb,&cache)==-1);assert(nr_scene_row_planar_words(&s,0,&bb,&cache)==-1);
    for(i=0;i<sizeof b;i++)assert(b[i]==0xa5);
    puts("PASS: 131072 direct planar tile rows and output guards");return 0;
}
