/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "fine_event_renderer.h"

static unsigned fine_word(const uint8_t *p)
{return p[0]|((unsigned)p[1]<<8);}

void nr_fine_event_invalidate_all(nr_fine_event_cache *c)
{
    unsigned y;
    if(!c)return;
    for(y=0;y<152;y++)c->valid[y]=0;
    nr_scene_cache_init(&c->sprites);
}

void nr_fine_event_init(nr_fine_event_cache *c)
{
    if(!c)return;
    nr_fine_event_invalidate_all(c);
    c->composed=c->reused=c->notifications=0;
}

void nr_fine_event_write(nr_fine_event_cache *c,unsigned kind,unsigned offset)
{
    unsigned y,n,index;
    uint32_t mask;
    if(!c)return;
    c->notifications++;
    if(kind>3 || (kind==0 && offset>=8192) ||
       (kind==1 && offset>=4096) || (kind==2 && offset>=256) ||
       (kind==3 && offset>=64) || kind==2) {
        nr_fine_event_invalidate_all(c);
        return;
    }
    if(kind==1) {
        unsigned plane=offset>>11,row=(offset>>6)&31;
        mask=(uint32_t)1u<<((offset>>1)&31);
        for(y=0;y<152;y++)
            if(c->valid[y] && c->map_rows[y][plane]==row &&
               (c->map_columns[y][plane]&mask))c->valid[y]=0;
        return;
    }
    n=kind==0?offset>>1:offset;
    index=n>>5;mask=(uint32_t)1u<<(n&31);
    if(kind==0) {
        for(y=0;y<152;y++)
            if(c->valid[y] && (c->tile_words[y][index]&mask))c->valid[y]=0;
    } else {
        for(y=0;y<152;y++)
            if(c->valid[y] && (c->sprite_palettes[y][index]&mask))c->valid[y]=0;
    }
}

static unsigned fine_selected_word(unsigned desc,unsigned row)
{
    if(desc&0x4000)row=7-row;
    return (desc&511)*8+row;
}

static void fine_depend_word(uint32_t *bits,unsigned n)
{bits[n>>5]|=(uint32_t)1u<<(n&31);}

/* Geometry is already clipped to a nonempty window intersection. Only palette
 * dependencies inspect texels: transparent words must still depend on writes. */
static unsigned fine_visible_opaque(const nr_scene *s,unsigned desc,unsigned n,int x,
                               unsigned right)
{
    unsigned first=s->window_x>x?(unsigned)((int)s->window_x-x):0;
    unsigned last=(int)right<x+8?(unsigned)((int)right-x):8;
    unsigned mask;
    if(desc&0x8000) {unsigned t=first;first=8-last;last=8-t;}
    mask=(65535u>>(first*2))&(65535u<<((8-last)*2));
    return fine_word(s->tiles+n*2)&mask;
}

static void fine_dependencies(const nr_scene *s,unsigned y,nr_fine_event_cache *c)
{
    unsigned i,n,col,depth,right=(unsigned)s->window_x+s->window_w;
    uint32_t *words=c->tile_words[y];
    for(i=0;i<128;i++)words[i]=0;
    for(i=0;i<2;i++) {
        c->map_columns[y][i]=0;c->map_rows[y][i]=0;
        c->sprite_palettes[y][i]=0;
    }
    if(right>160)right=160;
    if(y<s->window_y || y>=(unsigned)s->window_y+s->window_h ||
       s->window_x>=right)return;
    for(n=0;n<2;n++) {
        unsigned sy=(y+s->scroll_y[n])&255;
        uint32_t columns=0;
        c->map_rows[y][n]=(uint8_t)(sy>>3);
        for(col=0;col<32;col++) {
            int x=(int)((col*8-s->scroll_x[n])&255);
            unsigned index,desc;
            if(x>248)x-=256;
            if(x>=(int)right || x+8<=s->window_x)continue;
            index=n*1024+(sy>>3)*32+col;
            desc=fine_word(s->maps+index*2);
            columns|=(uint32_t)1u<<col;
            fine_depend_word(words,fine_selected_word(desc,sy&7));
        }
        c->map_columns[y][n]=columns;
    }
    /* nr_scene_row_cached prepared this exact scene's sprite buckets first. */
    for(depth=0;depth<3;depth++) {
        unsigned bucket=y*3+depth;
        for(i=c->sprites.offsets[bucket];i<c->sprites.offsets[bucket+1];i++) {
            unsigned j=c->sprites.entries[i],desc=c->sprites.desc[j],selected;
            int x=c->sprites.x[j];
            if(x>=(int)right || x+8<=s->window_x)continue;
            selected=fine_selected_word(desc,(unsigned)((int)y-c->sprites.y[j]));
            fine_depend_word(words,selected);
            if(fine_visible_opaque(s,desc,selected,x,right))
                c->sprite_palettes[y][j>>5]|=(uint32_t)1u<<(j&31);
        }
    }
}

int nr_scene_row_fine_event(const nr_scene *s,unsigned y,uint8_t out[160],
                            nr_fine_event_cache *c)
{
    uint8_t regs[13];
    unsigned i,changed;
    if(!s || !c || !out || !s->tiles || !s->maps || !s->sprites ||
       !s->sprite_pal || y>=152)return -1;
    regs[0]=s->window_x;regs[1]=s->window_y;regs[2]=s->window_w;regs[3]=s->window_h;
    regs[4]=s->scroll_x[0];regs[5]=s->scroll_y[0];regs[6]=s->scroll_x[1];regs[7]=s->scroll_y[1];
    regs[8]=s->sprite_x;regs[9]=s->sprite_y;regs[10]=s->swap;regs[11]=s->background;regs[12]=s->outside;
    changed=!c->valid[y];
    if(!changed)for(i=0;i<13;i++)if(regs[i]!=c->regs[y][i]){changed=1;break;}
    if(changed) {
        nr_scene_row_cached(s,y,c->pixels[y],&c->sprites);
        fine_dependencies(s,y,c);
        for(i=0;i<13;i++)c->regs[y][i]=regs[i];
        c->valid[y]=1;c->composed++;
    }else c->reused++;
    for(i=0;i<160;i++)out[i]=c->pixels[y][i];
    return 0;
}
