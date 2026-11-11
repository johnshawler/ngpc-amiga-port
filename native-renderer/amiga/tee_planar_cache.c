/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "tee_planar_cache.h"
#include "selector_memory.h"
static void controls(const nr_scene *s,uint8_t out[13])
{
    out[0]=s->scroll_x[0];out[1]=s->scroll_x[1];out[2]=s->scroll_y[0];out[3]=s->scroll_y[1];
    out[4]=s->sprite_x;out[5]=s->sprite_y;out[6]=s->window_x;out[7]=s->window_y;
    out[8]=s->window_w;out[9]=s->window_h;out[10]=s->swap;out[11]=s->background;out[12]=s->outside;
}
static int equal(const uint8_t *a,const uint8_t *b,unsigned n,void (*pump)(void))
{
    unsigned i=0;
    while(i<n){unsigned part=n-i;if(part>1024)part=1024;if(!selector_span_equal(a+i,b+i,part))return 0;i+=part;if(pump)pump();}return 1;
}
int tee_planar_find(const nr_scene *s,const tee_planar_entry *entries,
    unsigned count,unsigned hint,selector_planar_stats *stats,void (*pump)(void))
{
    uint8_t state[13];unsigned pass,i;
    if(!s || !entries || !count || count>15 || !stats || !s->maps || !s->tiles || !s->sprites || !s->sprite_pal)return -1;
    stats->searches++;controls(s,state);if(hint>=count)hint=0;
    for(pass=0;pass<count;pass++){
        unsigned index=pass+hint;const tee_planar_entry *e;
        if(index>=count)index-=count;
        e=entries+index;stats->candidates++;
        if(pump)pump();
        /* Sprite bytes select likely animation/position before larger keys. */
        if(!equal(state,e->controls,13,0) || !equal(s->sprites,e->sprites,256,0) || !equal(s->sprite_pal,e->palettes,64,0))continue;
        /* High-numbered cursor tiles usually reject a different animation
         * before comparing background tiles/maps. This is only search order. */
        for(i=512;i>0;i--)if(e->referenced[(i-1)/8]&(1u<<((i-1)&7))){
            if(pump && !(i&31))pump();
            stats->tile_bytes+=16;
            if(!selector_span_equal(s->tiles+(i-1)*16,e->tiles+(i-1)*16,16))break;
        }
        if(i==0 && equal(s->maps,e->maps,4096,pump)){stats->hits++;return (int)index;}
    }
    stats->misses++;return -1;
}

/* PackBits: prevalidate the complete bounded stream before changing output. */
int tee_planar_unpack(const tee_planar_entry *e,uint8_t out[SELECTOR_PLANAR_BYTES],void (*pump)(void))
{
    unsigned pass;
    if(!e || !e->packed || !out)return -1;
    for(pass=0;pass<2;pass++){
        uint32_t at=0,dest=0,polled=0;
        while(at<e->packed_size){
            unsigned code=e->packed[at++],count;
            if(code==128)return -1;
            count=code<128?code+1:257-code;
            if(count>SELECTOR_PLANAR_BYTES-dest || (code<128?count:1)>e->packed_size-at)return -1;
            if(pass){
                if(code<128)selector_span_copy(out+dest,e->packed+at,count);
                else {unsigned i;for(i=0;i<count;i++)out[dest+i]=e->packed[at];}
            }
            at+=code<128?count:1;dest+=count;
            if(pump && dest-polled>=512){pump();polled=dest;}
        }
        if(dest!=SELECTOR_PLANAR_BYTES)return -1;
    }
    return 0;
}
