/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "fine_event_renderer.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

static uint8_t tiles[8192],maps[4096],sprites[256],pal[64];
static nr_fine_event_cache cache;
static unsigned seed=123;
static unsigned random32(void){seed=seed*1664525u+1013904223u;return seed;}
static void put16(uint8_t *p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void check(nr_scene *s,unsigned y)
{
    uint8_t a[160],b[160];
    assert(!nr_scene_row(s,y,a));
    assert(!nr_scene_row_fine_event(s,y,b,&cache));
    assert(!memcmp(a,b,160));
}
static void all_rows(nr_scene *s){unsigned y;for(y=0;y<152;y++)check(s,y);}
static unsigned invalid_rows(void)
{unsigned y,n=0;for(y=0;y<152;y++)n+=!cache.valid[y];return n;}
static nr_scene blank(void)
{
    nr_scene s={tiles,maps,sprites,pal,{0,0},{0,0},0,0,0,0,160,152,0,0,0};
    unsigned i;
    memset(tiles,0,sizeof tiles);memset(maps,0,sizeof maps);
    memset(sprites,0,sizeof sprites);memset(pal,0,sizeof pal);
    /* Scroll1 selects tile zero, scroll2 tile one, sprites default disabled. */
    for(i=1024;i<2048;i++)put16(maps+i*2,1);
    nr_fine_event_init(&cache);
    return s;
}

static void selected_words(void)
{
    nr_scene s=blank();unsigned y,flip,byte;
    for(flip=0;flip<4;flip++) {
        unsigned desc=(flip&1?0x4000:0)|(flip&2?0x8000:0);
        for(y=0;y<1024;y++)put16(maps+y*2,desc);
        nr_fine_event_invalidate_all(&cache);all_rows(&s);
        for(byte=0;byte<2;byte++) {
            uint32_t before=cache.composed;
            tiles[6+byte]^=255;nr_fine_event_write(&cache,0,6+byte);
            assert(invalid_rows()==19);
            for(y=0;y<152;y++)assert(!cache.valid[y]==((y&7)==(flip&1?4:3)));
            all_rows(&s);assert(cache.composed-before==19);
        }
        /* An unused tile, including the last tile word, invalidates nothing. */
        tiles[8191]^=255;nr_fine_event_write(&cache,0,8191);
        assert(!invalid_rows());all_rows(&s);
    }
    /* Zero texels still depend on both bytes; writes can create visibility. */
    assert(cache.notifications==12);
}

static void selected_maps(void)
{
    nr_scene s=blank();unsigned i;
    all_rows(&s);
    maps[0]^=1;nr_fine_event_write(&cache,1,0);
    assert(invalid_rows()==8);all_rows(&s);
    maps[1]^=128;nr_fine_event_write(&cache,1,1);
    assert(invalid_rows()==8);all_rows(&s);
    maps[50]^=1;nr_fine_event_write(&cache,1,50); /* Offscreen column 25. */
    assert(!invalid_rows());all_rows(&s);
    maps[4095]^=64;nr_fine_event_write(&cache,1,4095); /* Offscreen row/column. */
    assert(!invalid_rows());all_rows(&s);
    s.scroll_x[1]=255;s.scroll_y[1]=255;
    nr_fine_event_invalidate_all(&cache);all_rows(&s);
    /* Row zero wraps vertically to map row 31; column 31 stays offscreen. */
    maps[2048+31*64]^=1;nr_fine_event_write(&cache,1,2048+31*64);
    assert(invalid_rows()==1 && !cache.valid[0]);all_rows(&s);
    for(i=0;i<152;i++)assert(cache.valid[i]);
    s.window_w=0;all_rows(&s);
    nr_fine_event_write(&cache,1,0);nr_fine_event_write(&cache,0,0);
    assert(!invalid_rows());all_rows(&s);
}

static void palette_rows(void)
{
    nr_scene s=blank();unsigned y,flip,wx,ww;
    /* Tile two, highest-priority sprite at (4,20). Only local row three is
     * opaque. Palette writes must leave all other screen rows cached. */
    put16(sprites,0x1802);sprites[2]=4;sprites[3]=20;
    put16(tiles+2*16+3*2,0xc000);all_rows(&s);
    pal[0]=1;nr_fine_event_write(&cache,3,0);
    assert(invalid_rows()==1 && !cache.valid[23]);all_rows(&s);
    pal[63]=5;nr_fine_event_write(&cache,3,63);
    assert(!invalid_rows());all_rows(&s);
    /* Making a transparent sprite row opaque first invalidates through tile
     * words, then adds its palette dependency during recomposition. */
    put16(tiles+2*16+4*2,0x0003);
    nr_fine_event_write(&cache,0,2*16+4*2);
    nr_fine_event_write(&cache,0,2*16+4*2+1);
    assert(invalid_rows()==1 && !cache.valid[24]);all_rows(&s);
    pal[0]=2;nr_fine_event_write(&cache,3,0);
    assert(invalid_rows()==2 && !cache.valid[23] && !cache.valid[24]);all_rows(&s);
    /* Exhaustively clip a one-texel row against small windows for each flip.
     * H flip changes palette membership but never selected-word identity. */
    for(flip=0;flip<4;flip++)for(wx=0;wx<14;wx++)for(ww=0;ww<14;ww++) {
        unsigned desc=0x1802|(flip&1?0x4000:0)|(flip&2?0x8000:0);
        unsigned py=20+(flip&1?4:3),px=4+(flip&2?7:0);
        unsigned visible=ww && px>=wx && px<wx+ww;
        memset(tiles+2*16,0,16);put16(tiles+2*16+3*2,0xc000);
        put16(sprites,desc);s.window_x=(uint8_t)wx;s.window_w=(uint8_t)ww;
        nr_fine_event_invalidate_all(&cache);all_rows(&s);
        pal[0]^=1;nr_fine_event_write(&cache,3,0);
        assert(invalid_rows()==visible);
        for(y=0;y<152;y++)assert(!cache.valid[y]==(visible && y==py));
        all_rows(&s);
    }
    /* Negative wrapped x and y, and a disabled relative-chain anchor. */
    s=blank();put16(sprites+4,0x1e02);sprites[2]=255;sprites[3]=255;
    memset(tiles+2*16,255,16);nr_fine_event_write(&cache,2,0);all_rows(&s);
    pal[1]=4;nr_fine_event_write(&cache,3,1);
    assert(invalid_rows()==7);all_rows(&s);
    sprites[2]=7;nr_fine_event_write(&cache,2,2);
    assert(invalid_rows()==152);all_rows(&s);
    s.window_y=7;s.window_h=1;all_rows(&s);
    nr_fine_event_write(&cache,3,1);assert(!invalid_rows());all_rows(&s);
}

static void invalid_arguments(void)
{
    nr_scene s=blank(),bad;uint8_t out[160],saved[160];unsigned kind;
    memset(out,0xa5,sizeof out);memcpy(saved,out,sizeof out);
    assert(nr_scene_row_fine_event(0,0,out,&cache)==-1);
    assert(nr_scene_row_fine_event(&s,0,out,0)==-1);
    assert(nr_scene_row_fine_event(&s,0,0,&cache)==-1);
    assert(nr_scene_row_fine_event(&s,152,out,&cache)==-1);
    assert(nr_scene_row_fine_event(&s,UINT_MAX,out,&cache)==-1);
    bad=s;bad.tiles=0;assert(nr_scene_row_fine_event(&bad,0,out,&cache)==-1);
    bad=s;bad.maps=0;assert(nr_scene_row_fine_event(&bad,0,out,&cache)==-1);
    bad=s;bad.sprites=0;assert(nr_scene_row_fine_event(&bad,0,out,&cache)==-1);
    bad=s;bad.sprite_pal=0;assert(nr_scene_row_fine_event(&bad,0,out,&cache)==-1);
    assert(!memcmp(out,saved,sizeof out));
    assert(!cache.composed && !cache.reused && !cache.notifications);
    nr_fine_event_init(0);nr_fine_event_invalidate_all(0);nr_fine_event_write(0,0,0);
    for(kind=0;kind<5;kind++) {
        unsigned limits[5]={8192,4096,256,64,0};
        all_rows(&s);nr_fine_event_write(&cache,kind,limits[kind]);
        assert(invalid_rows()==152);
    }
    all_rows(&s);nr_fine_event_write(&cache,UINT_MAX,UINT_MAX);
    assert(invalid_rows()==152 && cache.notifications==6);
    all_rows(&s);nr_fine_event_invalidate_all(&cache);
    assert(invalid_rows()==152 && cache.notifications==6);
    nr_fine_event_init(&cache);
    assert(!cache.composed && !cache.reused && !cache.notifications);
}

static void mutate_register(nr_scene *s,unsigned n,unsigned value)
{
    switch(n) {
    case 0:s->window_x=(uint8_t)value;break;
    case 1:s->window_y=(uint8_t)value;break;
    case 2:s->window_w=(uint8_t)value;break;
    case 3:s->window_h=(uint8_t)value;break;
    case 4:s->scroll_x[0]=(uint8_t)value;break;
    case 5:s->scroll_y[0]=(uint8_t)value;break;
    case 6:s->scroll_x[1]=(uint8_t)value;break;
    case 7:s->scroll_y[1]=(uint8_t)value;break;
    case 8:s->sprite_x=(uint8_t)value;break;
    case 9:s->sprite_y=(uint8_t)value;break;
    case 10:s->swap=(uint8_t)value;break;
    case 11:s->background=(uint8_t)value;break;
    default:s->outside=(uint8_t)value;break;
    }
}

static void all_bytes_and_raster(void)
{
    nr_scene s={tiles,maps,sprites,pal,{251,7},{253,19},249,250,3,2,154,147,1,2,5};
    unsigned i,j;
    for(i=0;i<sizeof tiles;i++)tiles[i]=(uint8_t)random32();
    for(i=0;i<sizeof maps;i++)maps[i]=(uint8_t)(random32()>>8);
    for(i=0;i<sizeof sprites;i++)sprites[i]=(uint8_t)(random32()>>16);
    for(i=0;i<sizeof pal;i++)pal[i]=(uint8_t)random32();
    nr_fine_event_init(&cache);all_rows(&s);all_rows(&s);
    assert(cache.composed==152 && cache.reused==152);
    for(i=0;i<8192;i++) {tiles[i]^=255;nr_fine_event_write(&cache,0,i);all_rows(&s);}
    for(i=0;i<4096;i++) {maps[i]^=255;nr_fine_event_write(&cache,1,i);all_rows(&s);}
    for(i=0;i<256;i++) {sprites[i]^=255;nr_fine_event_write(&cache,2,i);all_rows(&s);}
    for(i=0;i<64;i++) {pal[i]^=15;nr_fine_event_write(&cache,3,i);all_rows(&s);}
    assert(cache.notifications==12608);
    /* Every latched register changes independently, including global sprite Y,
     * background/outside pen masks, empty/overflowing windows and both planes. */
    for(i=0;i<5000;i++) {
        mutate_register(&s,i%13,random32()>>24);check(&s,i%152);
        /* Interleave notified writes while other rows retain different old
         * register snapshots, then revisit a spread of those cached rows. */
        j=random32()%8192;tiles[j]^=0xa5;nr_fine_event_write(&cache,0,j);
        j=random32()%4096;maps[j]^=0x5a;nr_fine_event_write(&cache,1,j);
        j=random32()%64;pal[j]^=15;nr_fine_event_write(&cache,3,j);
        for(j=0;j<4;j++)check(&s,random32()%152);
    }
    all_rows(&s);
    /* Bulk replacement must explicitly invalidate, even at identical pointers. */
    memset(tiles,255,sizeof tiles);memset(sprites,0,sizeof sprites);
    nr_fine_event_invalidate_all(&cache);all_rows(&s);
}

int main(void)
{
    selected_words();selected_maps();palette_rows();invalid_arguments();
    all_bytes_and_raster();
    puts("PASS fine event: all 12608 graphics bytes, selected words/flips/maps, clipped sprite palettes, relative chains, raster writes, invalid inputs");
    printf("fine event cache bytes=%zu\n",sizeof cache);
    return 0;
}
