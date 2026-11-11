/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "adaptive_renderer.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
static uint8_t tiles[8192],maps[4096],sprites[256],pal[64];
static nr_adaptive_cache cache;
static unsigned seed=123;
static unsigned random32(void){seed=seed*1664525u+1013904223u;return seed;}
static void check(nr_scene *s,unsigned y){uint8_t a[160],b[160];assert(!nr_scene_row(s,y,a));assert(nr_scene_row_adaptive(s,y,b,&cache)>=0);assert(!memcmp(a,b,160));}
int main(void)
{
    nr_scene s={tiles,maps,sprites,pal,{251,7},{253,19},249,250,3,2,154,147,1,2,5};
    unsigned i,y;
    for(i=0;i<sizeof tiles;i++)tiles[i]=(uint8_t)random32();
    for(i=0;i<sizeof maps;i++)maps[i]=(uint8_t)(random32()>>8);
    for(i=0;i<sizeof sprites;i++)sprites[i]=(uint8_t)(random32()>>16);
    for(i=0;i<sizeof pal;i++)pal[i]=(uint8_t)random32();
    nr_adaptive_init(&cache);
    for(y=0;y<152;y++)check(&s,y);
    for(y=0;y<152;y++)check(&s,y);
    assert(cache.fine.composed==152 && cache.fine.reused==0 && cache.fallback_rows==152);
    /* Every tile and map byte, changed between live scanlines. */
    for(i=0;i<8192;i++){tiles[i]^=255;nr_adaptive_write(&cache,0,i);for(y=0;y<152;y++)check(&s,y);}
    for(i=0;i<4096;i++){maps[i]^=255;nr_adaptive_write(&cache,1,i);for(y=0;y<152;y++)check(&s,y);}
    for(i=0;i<320;i++){
        if(i<256)sprites[i]^=255;else pal[i-256]^=15;
        nr_adaptive_write(&cache,i<256?2:3,i<256?i:i-256);for(y=0;y<152;y++)check(&s,y);
    }
    for(i=0;i<5000;i++) {
        s.scroll_x[i&1]=(uint8_t)random32();s.scroll_y[(i>>1)&1]=(uint8_t)(random32()>>8);
        s.sprite_x=(uint8_t)(random32()>>16);s.window_x=(uint8_t)(random32()>>24);
        s.window_w=(uint8_t)random32();s.window_y=(uint8_t)(random32()>>8);
        s.window_h=(uint8_t)(random32()>>16);s.swap=i&1;s.background=i&7;
        check(&s,i%152);
    }
    /* Packed fallback must invalidate dirty shadow before later cache reentry. */
    {
        static uint8_t actual[24320],expected[24320];
        static nr_planar_cache dirty;
        uint8_t a[160],b[160];unsigned frame;
        nr_bitmap dst={actual,sizeof actual,160,152,20};
        nr_bitmap ref={expected,sizeof expected,160,152,20};
        nr_adaptive_init(&cache);nr_planar_cache_init(&dirty);
        for(frame=0;frame<12;frame++) {
            s.scroll_x[0]=(uint8_t)(frame<5?frame:5);
            for(y=0;y<152;y++) {
                int fallback=nr_scene_row_adaptive(&s,y,a,&cache);
                assert(fallback>=0);nr_scene_row(&s,y,b);assert(!memcmp(a,b,160));
                if(fallback){dirty.valid[y]=0;nr_store_scene_row_packed(&dst,y,a);}
                else nr_store_scene_row_dirty(&dst,y,a,&dirty);
                nr_store_scene_row(&ref,y,b);
            }
            assert(!memcmp(actual,expected,sizeof actual));
        }
        assert(cache.fallback_rows && cache.fine.reused);
    }
    nr_adaptive_write(&cache,0,8192);for(y=0;y<152;y++)assert(!cache.fine.valid[y]);
    puts("PASS adaptive dependencies: all 12288 tile/map bytes, all 320 sprite bytes, live-register mutations");
    printf("adaptive cache bytes=%zu\n",sizeof cache);return 0;
}
