/* SPDX-License-Identifier: GPL-2.0-or-later
 * New color compositor. Behavioral reference: NeoPop/Mednafen gfx.c at the
 * pinned beetle-ngp submodule (NeoPop copyright 2001-2002 neopop_uk, GPL2+).
 */
#include "scene_renderer.h"
static unsigned le16(const uint8_t *p) { return p[0] | ((unsigned)p[1]<<8); }
static void tile_row(const nr_scene *s, uint8_t *out, unsigned desc,
                     int x, unsigned row, unsigned pal)
{
    unsigned i, word;
    if (desc & 0x4000) row=7-row;
    word=le16(s->tiles+(desc&511)*16+row*2);
    for (i=0;i<8;i++) {
        int px=x+(int)i;
        unsigned texel=(word>>((desc&0x8000)?2*i:14-2*i))&3;
        if (texel && px>=s->window_x && px<160 &&
            px<(int)s->window_x+s->window_w) out[px]=(uint8_t)(pal+texel);
    }
}
static void plane(const nr_scene *s, unsigned n, unsigned y, uint8_t *out)
{
    unsigned col, sy=(y+s->scroll_y[n])&255;
    for (col=0;col<32;col++) {
        unsigned desc=le16(s->maps+n*2048+((sy/8)*32+col)*2);
        int x=(int)((col*8-s->scroll_x[n])&255);
        if (x>248) x-=256;
        if (x<160) tile_row(s,out,desc,x,sy&7,64+n*64+((desc>>9)&15)*4);
    }
}
int nr_scene_row(const nr_scene *s, unsigned y, uint8_t out[160])
{
    int xs[64], ys[64];
    unsigned desc[64], i, depth;
    int lastx=0,lasty=0;
    if (!s || !out || !s->tiles || !s->maps || !s->sprites ||
        !s->sprite_pal || y>=152) return -1;
    for(i=0;i<160;i++) out[i]=(uint8_t)(248+(s->outside&7));
    if(y<s->window_y || y>=(unsigned)s->window_y+s->window_h) return 0;
    for(i=s->window_x;i<160 && i<(unsigned)s->window_x+s->window_w;i++)
        out[i]=(uint8_t)(240+(s->background&7));
    /* Resolve chains BEFORE priority filtering. Disabled sprites still anchor
     * following sprites. Preserve reference's pre-modulo vertical wrap test. */
    for(i=0;i<64;i++) {
        unsigned d=le16(s->sprites+i*4);
        int x=s->sprites[i*4+2], yy=s->sprites[i*4+3];
        if(d&0x400) x+=lastx;
        if(d&0x200) yy+=lasty;
        lastx=x; lasty=yy;
        x+=s->sprite_x; yy+=s->sprite_y;
        xs[i]=x&255; if(xs[i]>248) xs[i]-=256;
        ys[i]=(yy>248 && yy<256)?yy-256:yy&255;
        desc[i]=d;
    }
    /* Strict priority plus lower sprite index wins: painter order replaces
     * per-pixel depth comparisons. Plane priorities are 3 and 5. */
    for(depth=1;depth<=3;depth++) {
        for(i=64;i>0;i--) {
            unsigned j=i-1,d=desc[j];
            if(((d>>11)&3)==depth && (int)y>=ys[j] && (int)y<ys[j]+8)
                tile_row(s,out,d,xs[j],y-ys[j],(s->sprite_pal[j]&15)*4);
        }
        if(depth<3) plane(s,s->swap?(depth-1):(2-depth),y,out);
    }
    return 0;
}
int nr_store_scene_row(nr_bitmap *d, unsigned y, const uint8_t pens[160])
{
    size_t size; unsigned p,b,i;
    if(!d || !d->data || !pens || d->width!=160 || d->height!=152 ||
       d->stride<20 || d->stride>4096 || (d->stride&1) || y>=152) return -1;
    size=(size_t)d->stride*152;
    if(d->capacity<size*8) return -1;
    for(p=0;p<8;p++) for(b=0;b<20;b++) {
        unsigned v=0;
        for(i=0;i<8;i++) v=(v<<1)|((pens[b*8+i]>>p)&1);
        d->data[p*size+y*d->stride+b]=(uint8_t)v;
    }
    return 0;
}

void nr_scene_cache_init(nr_scene_cache *c)
{
    if(c) {c->valid=0;c->rebuilds=0;c->hits=0;}
}
static void prepare_sprites(const nr_scene *s,nr_scene_cache *c)
{
    unsigned i,j,y,changed=!c->valid || c->sx!=s->sprite_x || c->sy!=s->sprite_y;
    int lastx=0,lasty=0;
    uint16_t next[456];
    if(!changed) {
        for(i=0;i<256;i++) if(c->raw[i]!=s->sprites[i]) {changed=1;break;}
        if(!changed) for(i=0;i<64;i++) if(c->raw[256+i]!=s->sprite_pal[i]) {changed=1;break;}
    }
    if(!changed) {c->hits++;return;}
    c->rebuilds++;
    for(i=0;i<256;i++)c->raw[i]=s->sprites[i];
    for(i=0;i<64;i++)c->raw[256+i]=s->sprite_pal[i];
    c->sx=s->sprite_x;c->sy=s->sprite_y;
    for(i=0;i<457;i++)c->offsets[i]=0;
    for(i=0;i<64;i++) {
        unsigned d=le16(s->sprites+i*4),priority=(d>>11)&3;
        int x=s->sprites[i*4+2],yy=s->sprites[i*4+3],first,last;
        if(d&0x400)x+=lastx;
        if(d&0x200)yy+=lasty;
        lastx=x;lasty=yy;x+=s->sprite_x;yy+=s->sprite_y;
        x&=255;if(x>248)x-=256;
        yy=(yy>248 && yy<256)?yy-256:yy&255;
        c->x[i]=(int16_t)x;c->y[i]=(int16_t)yy;c->desc[i]=(uint16_t)d;
        first=yy<0?0:yy;last=yy+8>152?152:yy+8;
        if(priority && x<160) for(j=(unsigned)first;j<(unsigned)last;j++)
            c->offsets[j*3+priority]++;
    }
    for(i=1;i<457;i++)c->offsets[i]+=c->offsets[i-1];
    for(i=0;i<456;i++)next[i]=c->offsets[i];
    /* Descending index means earlier sprites overwrite later equal-priority
     * pixels; each list is already in painter order. */
    for(i=64;i>0;i--) {
        unsigned n=i-1,priority=(c->desc[n]>>11)&3;
        int first=c->y[n]<0?0:c->y[n],last=c->y[n]+8>152?152:c->y[n]+8;
        if(priority && c->x[n]<160) for(y=(unsigned)first;y<(unsigned)last;y++)
            c->entries[next[y*3+priority-1]++]=(uint8_t)n;
    }
    c->valid=1;
}
int nr_scene_row_cached(const nr_scene *s,unsigned y,uint8_t out[160],nr_scene_cache *c)
{
    unsigned i,depth;
    if(!s || !out || !c || !s->tiles || !s->maps || !s->sprites || !s->sprite_pal || y>=152)return -1;
    for(i=0;i<160;i++)out[i]=(uint8_t)(248+(s->outside&7));
    if(y<s->window_y || y>=(unsigned)s->window_y+s->window_h)return 0;
    for(i=s->window_x;i<160 && i<(unsigned)s->window_x+s->window_w;i++)out[i]=(uint8_t)(240+(s->background&7));
    prepare_sprites(s,c);
    for(depth=1;depth<=3;depth++) {
        unsigned bucket=y*3+depth-1;
        for(i=c->offsets[bucket];i<c->offsets[bucket+1];i++) {
            unsigned n=c->entries[i];
            tile_row(s,out,c->desc[n],c->x[n],y-c->y[n],(s->sprite_pal[n]&15)*4);
        }
        if(depth<3)plane(s,s->swap?(depth-1):(2-depth),y,out);
    }
    return 0;
}
