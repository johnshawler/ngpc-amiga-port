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

/* 8x8 bit transpose using two 32-bit words, never 64-bit arithmetic or tables.
 * First output byte is plane 7; input pixel zero becomes its most significant bit. */
static void pack8(const uint8_t *p,uint8_t *dst,size_t plane_size)
{
    uint32_t hi=((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
    uint32_t lo=((uint32_t)p[4]<<24)|((uint32_t)p[5]<<16)|((uint32_t)p[6]<<8)|p[7];
    uint32_t a,b;
    a=(lo^((lo>>7)|(hi<<25)))&0x00aa00aaU;b=(hi^(hi>>7))&0x00aa00aaU;
    lo^=a^(a<<7);hi^=b^(b<<7)^(a>>25);
    a=(lo^((lo>>14)|(hi<<18)))&0x0000ccccU;b=(hi^(hi>>14))&0x0000ccccU;
    lo^=a^(a<<14);hi^=b^(b<<14)^(a>>18);
    a=(lo^((lo>>28)|(hi<<4)))&0xf0f0f0f0U;
    lo^=a^(a<<28);hi^=a>>4;
    dst[0]=(uint8_t)lo;dst[plane_size]=(uint8_t)(lo>>8);
    dst[plane_size*2]=(uint8_t)(lo>>16);dst[plane_size*3]=(uint8_t)(lo>>24);
    dst[plane_size*4]=(uint8_t)hi;dst[plane_size*5]=(uint8_t)(hi>>8);
    dst[plane_size*6]=(uint8_t)(hi>>16);dst[plane_size*7]=(uint8_t)(hi>>24);
}
static int valid_scene_bitmap(const nr_bitmap *d,unsigned y,const uint8_t *pens)
{
    return d && d->data && pens && d->width==160 && d->height==152 &&
        d->stride>=20 && d->stride<=4096 && !(d->stride&1) && y<152 &&
        d->capacity>=(size_t)d->stride*152*8;
}
int nr_store_scene_row_packed(nr_bitmap *d,unsigned y,const uint8_t pens[160])
{
    unsigned b;
    if(!valid_scene_bitmap(d,y,pens))return -1;
    for(b=0;b<20;b++)pack8(pens+b*8,d->data+y*d->stride+b,(size_t)d->stride*152);
    return 0;
}
void nr_planar_cache_init(nr_planar_cache *c)
{
    unsigned y;if(!c)return;
    for(y=0;y<152;y++)c->valid[y]=0;
    c->owner=0;c->stride=0;c->converted_blocks=0;c->skipped_blocks=0;
}
int nr_store_scene_row_dirty(nr_bitmap *d,unsigned y,const uint8_t pens[160],nr_planar_cache *c)
{
    unsigned b,i;
    if(!c || !valid_scene_bitmap(d,y,pens))return -1;
    if(c->owner!=d->data || c->stride!=d->stride) {
        for(i=0;i<152;i++)c->valid[i]=0;
        c->owner=d->data;c->stride=d->stride;
    }
    for(b=0;b<20;b++) {
        uint8_t *previous=c->pens+y*160+b*8;
        unsigned changed=!(c->valid[y]&(1u<<b));
        if(!changed)for(i=0;i<8;i++)if(previous[i]!=pens[b*8+i]) {changed=1;break;}
        if(!changed) {c->skipped_blocks++;continue;}
        pack8(pens+b*8,d->data+y*d->stride+b,(size_t)d->stride*152);
        for(i=0;i<8;i++)previous[i]=pens[b*8+i];
        c->valid[y]|=1u<<b;c->converted_blocks++;
    }
    return 0;
}

void nr_composition_cache_init(nr_composition_cache *c)
{
    unsigned y;if(!c)return;
    nr_scene_cache_init(&c->sprites);
    for(y=0;y<152;y++)c->lengths[y]=0;
    c->composed_rows=0;c->reused_rows=0;
}
static void key16(uint8_t **p,unsigned value)
{*(*p)++=(uint8_t)value;*(*p)++=(uint8_t)(value>>8);}
static unsigned scene_key(const nr_scene *s,unsigned y,nr_scene_cache *c,uint8_t *key)
{
    uint8_t *p=key;unsigned n,col,depth;
    *p++=s->window_x;*p++=s->window_y;*p++=s->window_w;*p++=s->window_h;
    *p++=s->scroll_x[0];*p++=s->scroll_y[0];*p++=s->scroll_x[1];*p++=s->scroll_y[1];
    *p++=s->swap;*p++=s->background;*p++=s->outside;
    if(y<s->window_y || y>=(unsigned)s->window_y+s->window_h)return (unsigned)(p-key);
    for(n=0;n<2;n++) {
        unsigned sy=(y+s->scroll_y[n])&255;
        for(col=0;col<32;col++) {
            int x=(int)((col*8-s->scroll_x[n])&255);
            unsigned d,row;
            if(x>248)x-=256;
            if(x>=160 || x>=(int)s->window_x+s->window_w || x+8<=s->window_x)continue;
            d=le16(s->maps+n*2048+((sy/8)*32+col)*2);row=sy&7;
            if(d&0x4000)row=7-row;
            key16(&p,d);key16(&p,le16(s->tiles+(d&511)*16+row*2));
        }
    }
    prepare_sprites(s,c);
    for(depth=1;depth<=3;depth++) {
        unsigned bucket=y*3+depth-1,i;
        *p++=(uint8_t)(c->offsets[bucket+1]-c->offsets[bucket]);
        for(i=c->offsets[bucket];i<c->offsets[bucket+1];i++) {
            unsigned j=c->entries[i],d=c->desc[j],row=y-c->y[j];
            if(d&0x4000)row=7-row;
            key16(&p,d);key16(&p,(unsigned)c->x[j]);key16(&p,(unsigned)c->y[j]);
            *p++=s->sprite_pal[j];key16(&p,le16(s->tiles+(d&511)*16+row*2));
        }
    }
    /* At most 42 visible plane tiles x4, 64 sprites x9, 11 regs +3 counts:
     * 758 bytes. Exact byte keys avoid collision-based stale rendering. */
    return (unsigned)(p-key);
}
int nr_scene_row_reused(const nr_scene *s,unsigned y,uint8_t out[160],nr_composition_cache *c)
{
    uint8_t key[768];unsigned length,i,changed;
    if(!s || !out || !c || !s->tiles || !s->maps || !s->sprites || !s->sprite_pal || y>=152)return -1;
    length=scene_key(s,y,&c->sprites,key);
    changed=length!=c->lengths[y];
    if(!changed)for(i=0;i<length;i++)if(key[i]!=c->keys[y][i]){changed=1;break;}
    if(changed) {
        nr_scene_row_cached(s,y,c->pixels[y],&c->sprites);
        for(i=0;i<length;i++)c->keys[y][i]=key[i];
        c->lengths[y]=(uint16_t)length;c->composed_rows++;
    } else c->reused_rows++;
    for(i=0;i<160;i++)out[i]=c->pixels[y][i];
    return 0;
}

static __attribute__((always_inline)) inline unsigned compact_even(unsigned v)
{
    v&=0x5555;v=(v|(v>>1))&0x3333;v=(v|(v>>2))&0x0f0f;return (v|(v>>4))&255;
}
static __attribute__((always_inline)) inline unsigned reverse8(unsigned v)
{
    v=((v&0x55)<<1)|((v>>1)&0x55);v=((v&0x33)<<2)|((v>>2)&0x33);return ((v<<4)|(v>>4))&255;
}
static void planar_tile(const nr_scene *s,nr_bitmap *d,unsigned y,unsigned desc,int x,unsigned row,unsigned pal)
{
    int first=(int)s->window_x-x,last=(int)s->window_x+s->window_w;
    unsigned word,b0,b1,opaque,p,offset,mask,lo,hi;
    size_t size=(size_t)d->stride*152;
    if(last>160)last=160;
    last-=x;if(first<0)first=0;if(last>8)last=8;
    if(first>=last)return;
    if(desc&0x4000)row=7-row;
    word=le16(s->tiles+(desc&511)*16+row*2);
    b0=compact_even(word);b1=compact_even(word>>1);
    if(desc&0x8000){b0=reverse8(b0);b1=reverse8(b1);}
    opaque=(b0|b1)&(255u>>first)&(255u<<(8-last));
    if(!opaque)return;
    b0&=opaque;b1&=opaque;
    if(x<0) {mask=(opaque<<(-x+8))&65535;lo=(b0<<(-x+8))&65535;hi=(b1<<(-x+8))&65535;offset=0;}
    else {unsigned shift=x&7;mask=(opaque<<8)>>shift;lo=(b0<<8)>>shift;hi=(b1<<8)>>shift;offset=(unsigned)x/8;}
    offset+=y*d->stride;
    for(p=0;p<8;p++) {
        unsigned bits=p==0?lo:p==1?hi:(pal&(1u<<p))?mask:0;
        uint8_t *dst=d->data+p*size+offset;
        if(mask&0xff00)dst[0]=(uint8_t)((dst[0]&~(mask>>8))|(bits>>8));
        if(mask&255)dst[1]=(uint8_t)((dst[1]&~mask)|bits);
    }
}
static void planar_plane(const nr_scene *s,unsigned n,unsigned y,nr_bitmap *d)
{
    unsigned col,sy=(y+s->scroll_y[n])&255;
    for(col=0;col<32;col++) {
        int x=(int)((col*8-s->scroll_x[n])&255);
        unsigned desc;
        if(x>248)x-=256;
        if(x>=160 || x>=(int)s->window_x+s->window_w || x+8<=s->window_x)continue;
        desc=le16(s->maps+n*2048+((sy/8)*32+col)*2);
        planar_tile(s,d,y,desc,x,sy&7,64+n*64+((desc>>9)&15)*4);
    }
}
int nr_scene_row_planar(const nr_scene *s,unsigned y,nr_bitmap *d,nr_scene_cache *c)
{
    unsigned byte,p,depth,out,bg,active,left,right;
    size_t size;
    if(!s || !c || !s->tiles || !s->maps || !s->sprites || !s->sprite_pal || !valid_scene_bitmap(d,y,s->tiles))return -1;
    size=(size_t)d->stride*152;out=248+(s->outside&7);bg=240+(s->background&7);
    active=y>=s->window_y && y<(unsigned)s->window_y+s->window_h;
    left=s->window_x;right=left+s->window_w;if(right>160)right=160;
    for(byte=0;byte<20;byte++) {
        unsigned mask=0,i;
        if(active)for(i=0;i<8;i++)if(byte*8+i>=left && byte*8+i<right)mask|=128u>>i;
        for(p=0;p<8;p++)d->data[p*size+y*d->stride+byte]=(uint8_t)(((out&(1u<<p))?~mask:0)|((bg&(1u<<p))?mask:0));
    }
    if(!active)return 0;
    prepare_sprites(s,c);
    for(depth=1;depth<=3;depth++) {
        unsigned bucket=y*3+depth-1,i;
        for(i=c->offsets[bucket];i<c->offsets[bucket+1];i++) {
            unsigned j=c->entries[i];
            planar_tile(s,d,y,c->desc[j],c->x[j],y-c->y[j],(s->sprite_pal[j]&15)*4);
        }
        if(depth<3)planar_plane(s,s->swap?(depth-1):(2-depth),y,d);
    }
    return 0;
}

static const uint32_t plane_colors[16]={0x00000000U,0x000000ffU,0x0000ff00U,0x0000ffffU,0x00ff0000U,0x00ff00ffU,0x00ffff00U,0x00ffffffU,0xff000000U,0xff0000ffU,0xff00ff00U,0xff00ffffU,0xffff0000U,0xffff00ffU,0xffffff00U,0xffffffffU};
static __attribute__((always_inline)) inline void merge_words(uint32_t *dst,unsigned mask,unsigned b0,unsigned b1,unsigned pal)
{
    uint32_t alpha=mask|(mask<<8);alpha|=alpha<<16;
    dst[0]=(dst[0]&~alpha)|b0|(b1<<8)|(plane_colors[pal&12]&alpha);
    dst[1]=(dst[1]&~alpha)|(plane_colors[pal>>4]&alpha);
}
static __attribute__((always_inline)) inline void words_tile(const nr_scene *s,uint32_t *out,unsigned desc,int x,unsigned row,unsigned pal)
{
    int first=(int)s->window_x-x,last=(int)s->window_x+s->window_w;
    unsigned word,b0,b1,opaque,offset,mask,lo,hi;
    if(last>160)last=160;
    last-=x;if(first<0)first=0;if(last>8)last=8;
    if(first>=last)return;
    if(desc&0x4000)row=7-row;
    word=le16(s->tiles+(desc&511)*16+row*2);
    b0=compact_even(word);b1=compact_even(word>>1);
    if(desc&0x8000){b0=reverse8(b0);b1=reverse8(b1);}
    opaque=(b0|b1)&(255u>>first)&(255u<<(8-last));
    if(!opaque)return;
    b0&=opaque;b1&=opaque;
    if(x<0) {mask=(opaque<<(-x+8))&65535;lo=(b0<<(-x+8))&65535;hi=(b1<<(-x+8))&65535;offset=0;}
    else {unsigned shift=x&7;mask=(opaque<<8)>>shift;lo=(b0<<8)>>shift;hi=(b1<<8)>>shift;offset=((unsigned)x/8)*2;}
    if(mask&0xff00)merge_words(out+offset,mask>>8,lo>>8,hi>>8,pal);
    if(mask&255)merge_words(out+offset+2,mask&255,lo&255,hi&255,pal);
}
static __attribute__((always_inline)) inline void words_plane(const nr_scene *s,unsigned n,unsigned y,uint32_t *out)
{
    unsigned col,sy=(y+s->scroll_y[n])&255;
    for(col=0;col<32;col++) {
        int x=(int)((col*8-s->scroll_x[n])&255);unsigned desc;
        if(x>248)x-=256;
        if(x>=160 || x>=(int)s->window_x+s->window_w || x+8<=s->window_x)continue;
        desc=le16(s->maps+n*2048+((sy/8)*32+col)*2);
        words_tile(s,out,desc,x,sy&7,64+n*64+((desc>>9)&15)*4);
    }
}
int nr_scene_row_planar_words(const nr_scene *s,unsigned y,nr_bitmap *d,nr_scene_cache *c)
{
    uint32_t words[40];unsigned b,depth,out,bg,active;
    size_t size;
    if(!s || !c || !s->tiles || !s->maps || !s->sprites || !s->sprite_pal || !valid_scene_bitmap(d,y,s->tiles))return -1;
    size=(size_t)d->stride*152;out=248+(s->outside&7);bg=240+(s->background&7);
    active=y>=s->window_y && y<(unsigned)s->window_y+s->window_h;
    for(b=0;b<20;b++) {
        uint32_t alpha=0;int first=(int)s->window_x-(int)b*8,last=(int)s->window_x+s->window_w-(int)b*8;
        if(first<0)first=0;
        if(last>8)last=8;
        if(active && first<last) {alpha=(255u>>first)&(255u<<(8-last));alpha|=alpha<<8;alpha|=alpha<<16;}
        words[b*2]=(plane_colors[out&15]&~alpha)|(plane_colors[bg&15]&alpha);
        words[b*2+1]=(plane_colors[out>>4]&~alpha)|(plane_colors[bg>>4]&alpha);
    }
    if(active) {
        prepare_sprites(s,c);
        for(depth=1;depth<=3;depth++) {
            unsigned bucket=y*3+depth-1,i;
            for(i=c->offsets[bucket];i<c->offsets[bucket+1];i++) {
                unsigned j=c->entries[i];words_tile(s,words,c->desc[j],c->x[j],y-c->y[j],(s->sprite_pal[j]&15)*4);
            }
            if(depth<3)words_plane(s,s->swap?(depth-1):(2-depth),y,words);
        }
    }
    for(b=0;b<20;b++) {
        uint8_t *dst=d->data+y*d->stride+b;uint32_t lo=words[b*2],hi=words[b*2+1];
        dst[0]=(uint8_t)lo;dst[size]=(uint8_t)(lo>>8);dst[size*2]=(uint8_t)(lo>>16);dst[size*3]=(uint8_t)(lo>>24);
        dst[size*4]=(uint8_t)hi;dst[size*5]=(uint8_t)(hi>>8);dst[size*6]=(uint8_t)(hi>>16);dst[size*7]=(uint8_t)(hi>>24);
    }
    return 0;
}
