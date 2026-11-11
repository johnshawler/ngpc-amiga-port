/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "event_renderer.h"
static unsigned word(const uint8_t *p){return p[0]|((unsigned)p[1]<<8);}
static void bit(uint8_t *p,unsigned n){p[n>>3]|=(uint8_t)(1u<<(n&7));}
void nr_event_invalidate_all(nr_event_cache *c)
{
    unsigned y;if(!c)return;for(y=0;y<152;y++)c->valid[y]=0;
    nr_scene_cache_init(&c->sprites);
}
void nr_event_init(nr_event_cache *c)
{
    if(!c)return;
    nr_event_invalidate_all(c);c->composed=c->reused=c->notifications=0;
}
void nr_event_write(nr_event_cache *c,unsigned kind,unsigned offset)
{
    unsigned y,n;if(!c)return;c->notifications++;
    if(kind>1 || (kind==0 && offset>=8192) || (kind==1 && offset>=4096)) {
        nr_event_invalidate_all(c);return;
    }
    n=kind?offset/2:offset/16;
    for(y=0;y<152;y++)if(c->valid[y]) {
        const uint8_t *p=kind?c->maps[y]:c->tiles[y];
        if(p[n>>3]&(1u<<(n&7)))c->valid[y]=0;
    }
}
int nr_scene_row_event(const nr_scene *s,unsigned y,uint8_t out[160],nr_event_cache *c)
{
    uint8_t regs[13];unsigned i,n,col,depth,changed;
    if(!s || !c || !out || !s->tiles || !s->maps || !s->sprites || !s->sprite_pal || y>=152)return -1;
    regs[0]=s->window_x;regs[1]=s->window_y;regs[2]=s->window_w;regs[3]=s->window_h;
    regs[4]=s->scroll_x[0];regs[5]=s->scroll_y[0];regs[6]=s->scroll_x[1];regs[7]=s->scroll_y[1];
    regs[8]=s->sprite_x;regs[9]=s->sprite_y;regs[10]=s->swap;regs[11]=s->background;regs[12]=s->outside;
    changed=!c->valid[y];
    if(!changed)for(i=0;i<13;i++)if(regs[i]!=c->regs[y][i]){changed=1;break;}
    if(changed) {
        nr_scene_row_cached(s,y,c->pixels[y],&c->sprites);
        for(i=0;i<64;i++)c->tiles[y][i]=0;
        for(i=0;i<256;i++)c->maps[y][i]=0;
        if(y>=s->window_y && y<(unsigned)s->window_y+s->window_h) {
            for(n=0;n<2;n++) {
                unsigned sy=(y+s->scroll_y[n])&255;
                for(col=0;col<32;col++) {
                    int x=(int)((col*8-s->scroll_x[n])&255);unsigned index,d;
                    if(x>248)x-=256;
                    if(x>=160 || x>=(int)s->window_x+s->window_w || x+8<=s->window_x)continue;
                    index=n*1024+(sy/8)*32+col;d=word(s->maps+index*2);
                    bit(c->maps[y],index);bit(c->tiles[y],d&511);
                }
            }
            for(depth=0;depth<3;depth++) {
                unsigned bucket=y*3+depth;
                for(i=c->sprites.offsets[bucket];i<c->sprites.offsets[bucket+1];i++)
                    bit(c->tiles[y],c->sprites.desc[c->sprites.entries[i]]&511);
            }
        }
        for(i=0;i<13;i++)c->regs[y][i]=regs[i];
        c->valid[y]=1;c->composed++;
    }else c->reused++;
    for(i=0;i<160;i++)out[i]=c->pixels[y][i];
    return 0;
}
