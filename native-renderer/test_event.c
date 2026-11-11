/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "event_renderer.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
static uint8_t tiles[8192],maps[4096],sprites[256],pal[64];
static nr_event_cache cache;
static unsigned seed=123;
static unsigned random32(void){seed=seed*1664525u+1013904223u;return seed;}
static void check(nr_scene *s,unsigned y){uint8_t a[160],b[160];assert(!nr_scene_row(s,y,a));assert(!nr_scene_row_event(s,y,b,&cache));assert(!memcmp(a,b,160));}
int main(void)
{
    nr_scene s={tiles,maps,sprites,pal,{251,7},{253,19},249,250,3,2,154,147,1,2,5};
    unsigned i,y;
    for(i=0;i<sizeof tiles;i++)tiles[i]=(uint8_t)random32();
    for(i=0;i<sizeof maps;i++)maps[i]=(uint8_t)(random32()>>8);
    for(i=0;i<sizeof sprites;i++)sprites[i]=(uint8_t)(random32()>>16);
    for(i=0;i<sizeof pal;i++)pal[i]=(uint8_t)random32();
    nr_event_init(&cache);
    for(y=0;y<152;y++)check(&s,y);
    for(y=0;y<152;y++)check(&s,y);
    assert(cache.composed==152 && cache.reused==152);
    /* Every tile and map byte, changed between live scanlines. */
    for(i=0;i<8192;i++){tiles[i]^=255;nr_event_write(&cache,0,i);for(y=0;y<152;y++)check(&s,y);}
    for(i=0;i<4096;i++){maps[i]^=255;nr_event_write(&cache,1,i);for(y=0;y<152;y++)check(&s,y);}
    for(i=0;i<320;i++){
        if(i<256)sprites[i]^=255;else pal[i-256]^=15;
        nr_event_write(&cache,2,i);for(y=0;y<152;y++)check(&s,y);
    }
    for(i=0;i<5000;i++) {
        s.scroll_x[i&1]=(uint8_t)random32();s.scroll_y[(i>>1)&1]=(uint8_t)(random32()>>8);
        s.sprite_x=(uint8_t)(random32()>>16);s.window_x=(uint8_t)(random32()>>24);
        s.window_w=(uint8_t)random32();s.window_y=(uint8_t)(random32()>>8);
        s.window_h=(uint8_t)(random32()>>16);s.swap=i&1;s.background=i&7;
        check(&s,i%152);
    }
    nr_event_write(&cache,0,8192);for(y=0;y<152;y++)assert(!cache.valid[y]);
    puts("PASS event dependencies: all 12288 tile/map bytes, all 320 sprite bytes, live-register mutations");
    printf("event cache bytes=%zu\n",sizeof cache);return 0;
}
