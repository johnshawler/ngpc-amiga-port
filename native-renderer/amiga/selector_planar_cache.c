/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "selector_planar_cache.h"
#include "selector_memory.h"
static unsigned word(const uint8_t *p){return p[0]+256u*p[1];}
static void controls(const nr_scene *s,uint8_t out[13])
{
    out[0]=s->scroll_x[0];out[1]=s->scroll_x[1];out[2]=s->scroll_y[0];out[3]=s->scroll_y[1];
    out[4]=s->sprite_x;out[5]=s->sprite_y;out[6]=s->window_x;out[7]=s->window_y;
    out[8]=s->window_w;out[9]=s->window_h;out[10]=s->swap;out[11]=s->background;out[12]=s->outside;
}
void selector_planar_key(const nr_scene *s,selector_planar_entry *e)
{
    unsigned i;
    controls(s,e->controls);e->controls[13]=e->controls[14]=e->controls[15]=0;
    for(i=0;i<64;i++)e->referenced[i]=0;
    for(i=0;i<4096;i++)e->maps[i]=s->maps[i];
    for(i=0;i<256;i++)e->sprites[i]=s->sprites[i];
    for(i=0;i<64;i++)e->palettes[i]=s->sprite_pal[i];
    for(i=0;i<2048;i++){unsigned tile=word(s->maps+2*i)&511;e->referenced[tile/8]|=(uint8_t)(1u<<(tile&7));}
    for(i=0;i<64;i++){unsigned tile=word(s->sprites+4*i)&511;e->referenced[tile/8]|=(uint8_t)(1u<<(tile&7));}
    for(i=0;i<8192;i++)e->tiles[i]=(e->referenced[i/128]&(1u<<((i/16)&7)))?s->tiles[i]:0;
}
static int equal(const uint8_t *a,const uint8_t *b,unsigned n,void (*pump)(void))
{
    unsigned i=0;
    while(i<n){unsigned part=n-i;if(part>1024)part=1024;if(!selector_span_equal(a+i,b+i,part))return 0;i+=part;if(pump)pump();}return 1;
}
int selector_planar_find(const nr_scene *s,const selector_planar_entry *entries,
    unsigned count,unsigned hint,selector_planar_stats *stats,void (*pump)(void))
{
    uint8_t state[13];unsigned pass,i;
    if(!s || !entries || !count || count>15 || !stats || !s->maps || !s->tiles || !s->sprites || !s->sprite_pal)return -1;
    stats->searches++;controls(s,state);if(hint>=count)hint=0;
    for(pass=0;pass<count;pass++){
        unsigned index=pass+hint;const selector_planar_entry *e;
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
