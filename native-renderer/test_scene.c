/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "scene_renderer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t seed=0x12345678;
static uint8_t random8(void) { seed=seed*1664525u+1013904223u; return seed>>24; }
static unsigned word(const uint8_t *p) { return p[0]+256u*p[1]; }
/* Independent destination-pixel oracle: direct map lookup and ascending
 * sprite traversal with strict depth comparison, no painter-order routine. */
static unsigned texel(const nr_scene *s,unsigned d,unsigned x,unsigned y)
{
    if(d&0x8000) x=7-x;
    if(d&0x4000) y=7-y;
    return (word(s->tiles+(d&511)*16+2*y)>>(14-2*x))&3;
}
static void oracle(const nr_scene *s,unsigned y,uint8_t *out)
{
    unsigned px;
    for(px=0;px<160;px++) {
        unsigned pen=248+(s->outside&7),z=0,n;
        int lx=0,ly=0;
        if(px<s->window_x || px>=(unsigned)s->window_x+s->window_w ||
           y<s->window_y || y>=(unsigned)s->window_y+s->window_h) {out[px]=pen;continue;}
        pen=240+(s->background&7);
        for(n=0;n<2;n++) {
            unsigned sx=(px+s->scroll_x[n])&255,sy=(y+s->scroll_y[n])&255;
            unsigned d=word(s->maps+n*2048+2*((sy/8)*32+sx/8));
            unsigned t=texel(s,d,sx&7,sy&7),depth=(n==(s->swap?1u:0u))?5:3;
            if(t && depth>z) {z=depth;pen=64+n*64+((d>>9)&15)*4+t;}
        }
        for(n=0;n<64;n++) {
            unsigned d=word(s->sprites+4*n),depth=((d>>11)&3)*2;
            int x=s->sprites[4*n+2],yy=s->sprites[4*n+3],rx,ry;
            if(d&0x400) x+=lx;
            if(d&0x200) yy+=ly;
            lx=x;ly=yy;x+=s->sprite_x;yy+=s->sprite_y;
            rx=x&255;if(rx>248)rx-=256;
            ry=(yy>248&&yy<256)?yy-256:yy&255;
            if(depth>z && (int)px>=rx && (int)px<rx+8 && (int)y>=ry && (int)y<ry+8) {
                unsigned t=texel(s,d,px-rx,y-ry);
                if(t) {z=depth;pen=(s->sprite_pal[n]&15)*4+t;}
            }
        }
        out[px]=pen;
    }
}
int main(void)
{
    uint8_t tiles[8192],maps[4096],sprites[256],pal[64],actual[160],expected[160];
    static uint8_t bitmap[24*152*8];
    nr_bitmap dst={bitmap,sizeof bitmap,160,152,24};
    nr_scene s; unsigned iteration,i,y,p,x;
    memset(&s,0,sizeof s);s.tiles=tiles;s.maps=maps;s.sprites=sprites;s.sprite_pal=pal;
    for(i=0;i<sizeof tiles;i++) tiles[i]=random8();
    for(iteration=0;iteration<4000;iteration++) {
        for(i=0;i<sizeof maps;i++) maps[i]=random8();
        for(i=0;i<sizeof sprites;i++) sprites[i]=random8();
        for(i=0;i<sizeof pal;i++) pal[i]=random8();
        for(i=0;i<2;i++) {s.scroll_x[i]=random8();s.scroll_y[i]=random8();}
        s.sprite_x=random8();s.sprite_y=random8();s.swap=iteration&1;
        s.window_x=(iteration%3)?random8():0;s.window_y=(iteration%3)?random8():0;
        s.window_w=(iteration%3)?random8():160;s.window_h=(iteration%3)?random8():152;
        s.background=random8();s.outside=random8();y=iteration%152;
        oracle(&s,y,expected);assert(nr_scene_row(&s,y,actual)==0);
        assert(memcmp(actual,expected,160)==0);
        memset(bitmap,0xa5,sizeof bitmap);assert(nr_store_scene_row(&dst,y,actual)==0);
        for(p=0;p<8;p++) for(i=0;i<152;i++) for(x=0;x<24;x++) {
            unsigned value=0xa5;
            if(i==y && x<20) {unsigned b;value=0;for(b=0;b<8;b++) value=(value<<1)|((expected[x*8+b]>>p)&1);}
            assert(bitmap[p*24*152+i*24+x]==value);
        }
    }
    /* Directed coincident sprites, hidden chain anchors and wrap boundaries. */
    memset(tiles,0x55,sizeof tiles);memset(maps,0,sizeof maps);
    for(i=0;i<64;i++)pal[i]=i&15;
    s.window_x=0;s.window_y=0;s.window_w=160;s.window_h=152;
    for(iteration=0;iteration<512;iteration++) {
        memset(sprites,0,sizeof sprites);
        for(i=0;i<64;i++) {
            unsigned d=0x800*((i+iteration)%4);
            /* Some sprites chain through priority-zero entries. */
            if(iteration&4)d|=0x600;
            sprites[4*i]=d;sprites[4*i+1]=d>>8;
            sprites[4*i+2]=(iteration&4)?1:8;
            sprites[4*i+3]=(iteration&4)?4:0;
        }
        s.sprite_x=iteration&255;s.sprite_y=(iteration/2)&255;
        s.scroll_x[0]=s.scroll_x[1]=iteration&255;
        s.scroll_y[0]=s.scroll_y[1]=iteration&255;s.swap=iteration&1;
        for(y=0;y<16;y++) {
            oracle(&s,y,expected);assert(nr_scene_row(&s,y,actual)==0);
            assert(memcmp(actual,expected,160)==0);
        }
    }
    memset(actual,0xa5,sizeof actual);assert(nr_scene_row(&s,152,actual)==-1);
    assert(nr_scene_row(NULL,0,actual)==-1);s.tiles=NULL;
    assert(nr_scene_row(&s,0,actual)==-1);for(i=0;i<160;i++)assert(actual[i]==0xa5);
    memset(bitmap,0xa5,sizeof bitmap);dst.capacity--;
    assert(nr_store_scene_row(&dst,0,actual)==-1);
    for(i=0;i<sizeof bitmap;i++)assert(bitmap[i]==0xa5);
    puts("PASS: 4000 scene rows + 8192 directed rows, priority oracle, planar output and guards");return 0;
}
