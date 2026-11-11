/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "tile_renderer.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static uint32_t rng = 0x12006802;
static unsigned random_byte(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng & 255; }
/* Independent byte/pair extraction, not a mirrored 16-bit row or planar decode. */
static unsigned texel(const uint8_t *tile, unsigned x, unsigned y)
{
    unsigned b = tile[y*2 + (x < 4 ? 1 : 0)];
    return (b / (1u << (6 - (x%4)*2))) % 4;
}
static void oracle(nr_bitmap *b, const uint8_t *tiles, unsigned desc,
                   const uint8_t *pens, int x, int y)
{
    unsigned i,j,p;
    if (x <= -8 || y <= -8 || x >= (int)b->width || y >= (int)b->height) return;
    for (j=0;j<8;j++) for (i=0;i<8;i++) {
        int dx=x+(int)i, dy=y+(int)j;
        unsigned t,pen;
        if (dx<0 || dy<0 || dx>=(int)b->width || dy>=(int)b->height) continue;
        t=texel(tiles+(desc%512)*16, desc&32768 ? 7-i:i, desc&16384 ? 7-j:j);
        if (!t) continue;
        pen=pens[((desc/512)%16)*4+t];
        for (p=0;p<8;p++) {
            size_t a=(size_t)p*b->stride*b->height+(unsigned)dy*b->stride+(unsigned)dx/8;
            unsigned m=128u >> ((unsigned)dx%8);
            b->data[a]=(uint8_t)((b->data[a]&~m) | ((pen&(1u<<p)) ? m:0));
        }
    }
}
int main(void)
{
    unsigned w,f,i,p,n;
    uint8_t pens[64], tile[8192], row[2], planes[8], mask;
    static uint8_t actual[8*42*256+32], expected[sizeof actual];
    nr_bitmap a={actual+16, sizeof(actual)-32, 320,256,42}, b=a;
    b.data=expected+16;
    /* Explicit little-endian / leftmost orientation and transparency vector. */
    row[0]=0x1b;row[1]=0xe4;
    pens[0]=255;pens[1]=1;pens[2]=2;pens[3]=3;
    nr_decode_row(row,0,pens,planes,&mask);
    assert(planes[0]==0xa5 && planes[1]==0xc3 && mask==0xe7);
    for(w=0;w<65536;w++) {
        row[0]=(uint8_t)w;row[1]=(uint8_t)(w>>8);
        for(i=0;i<4;i++) pens[i]=(uint8_t)random_byte();
        for(f=0;f<2;f++) {
            nr_decode_row(row,(int)f,pens,planes,&mask);
            for(i=0;i<8;i++) {
                unsigned t=texel(row,f?7-i:i,0), pen=0;
                assert(!!(mask&(128u>>i)) == !!t);
                for(p=0;p<8;p++) if(planes[p]&(128u>>i)) pen |= 1u<<p;
                assert(pen==(t?pens[t]:0));
            }
        }
    }
    for(w=0;w<65536;w++)
        assert(nr_amiga_rgb12((uint16_t)w)==((w%16)*256+((w/16)%16)*16+(w/256)%16));
    for(i=0;i<sizeof actual;i++) actual[i]=expected[i]=(uint8_t)random_byte();
    for(i=0;i<sizeof tile;i++) tile[i]=(uint8_t)random_byte();
    for(i=0;i<sizeof pens;i++) pens[i]=(uint8_t)random_byte();
    /* Every descriptor covers all tiles, palettes, flips and ignored bit 13.
     * Continuous overdraw tests transparent retention and opaque zero pens. */
    for(n=0;n<65536;n++) {
        int x=(int)((n*37)%336)-8, y=(int)((n*11)%272)-8;
        if(n%1000==0) pens[(n/1000)%64]=0;
        assert(nr_draw_tile(&a,tile,sizeof tile,(uint16_t)n,pens,x,y)==0);
        oracle(&b,tile,n,pens,x,y);
        assert(memcmp(actual,expected,sizeof actual)==0);
    }
    /* Actual NGPC viewport; deliberately non-byte-aligned alternative width. */
    for(w=159;w<=160;w++) {
        a.width=b.width=w;a.height=b.height=152;a.stride=b.stride=20;
        for(n=0;n<2000;n++) {
            int x=(int)random_byte()-10,y=(int)random_byte()-10;
            unsigned d=random_byte()*256+random_byte();
            assert(nr_draw_tile(&a,tile,sizeof tile,(uint16_t)d,pens,x,y)==0);
            oracle(&b,tile,d,pens,x,y);
            assert(memcmp(actual,expected,sizeof actual)==0);
        }
    }
    assert(nr_draw_tile(&a,tile,sizeof tile,0,pens,INT_MIN,INT_MAX)==0);
    assert(nr_draw_tile(&a,tile,sizeof tile,0,pens,INT_MAX,INT_MIN)==0);
    assert(nr_draw_tile(NULL,tile,sizeof tile,0,pens,0,0)==-1);
    assert(nr_draw_tile(&a,NULL,sizeof tile,0,pens,0,0)==-1);
    assert(nr_draw_tile(&a,tile,8191,0,pens,0,0)==-1);
    assert(nr_draw_tile(&a,tile,sizeof tile,0,NULL,0,0)==-1);
    a.capacity=1;
    assert(nr_draw_tile(&a,tile,sizeof tile,0,pens,0,0)==-1);
    a.capacity=sizeof actual-32;a.stride=21;
    assert(nr_draw_tile(&a,tile,sizeof tile,0,pens,0,0)==-1);
    a.stride=2;
    assert(nr_draw_tile(&a,tile,sizeof tile,0,pens,0,0)==-1);
    a.stride=4098;
    assert(nr_draw_tile(&a,tile,sizeof tile,0,pens,0,0)==-1);
    a.stride=20;a.width=321;
    assert(nr_draw_tile(&a,tile,sizeof tile,0,pens,0,0)==-1);
    a.width=160;a.height=257;
    assert(nr_draw_tile(&a,tile,sizeof tile,0,pens,0,0)==-1);
    a.height=0;
    assert(nr_draw_tile(&a,tile,sizeof tile,0,pens,0,0)==-1);
    assert(memcmp(actual,expected,sizeof actual)==0);
    puts("PASS: 131072 row decodes, 65536 descriptors, 4000 viewport draws, 65536 colors, guards and invalid inputs");
    return 0;
}
