/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_screen.h"
#ifdef NTM_SCREEN_PRIVATE_ASSETS
/* Optional target backend. Caller guarantees the borrowed asset spans never
 * alias tile/map destinations. This does not change validation or queue order. */
extern void ntm_course_screen_copy_private(void *dst,const void *src,unsigned bytes);
#endif
static int screen_producer_valid(const ntm_course_screen *s,uint16_t selection,const ntm_screen_asset assets[3])
{
    return s && assets && selection<3 && s->stamp_bytes<=256 && !(s->stamp_bytes&7);
}
int ntm_course_screen_enqueue_fill(ntm_course_screen *s,uint16_t offset,
    uint8_t width,uint8_t height,uint16_t fill)
{
    unsigned i;
    if(!s || !width || !height || (offset&1) || (unsigned)offset+64u*(height-1u)+2u*width>4096)return -1;
    for(i=8;i;i--)if(s->fill_status[i-1]==255) {
        ntm_screen_fill *f=s->fills+i-1;
        f->offset=offset;f->width=width;f->height=height;f->fill=fill;s->fill_status[i-1]=0;return 0;
    }
    return 1;
}
int ntm_course_screen_enqueue_stamp(ntm_course_screen *s,const ntm_screen_asset *asset,
    uint16_t offset,uint16_t tile)
{
    ntm_screen_stamp *stamp;
    if(!s || !asset || s->stamp_bytes>256 || (s->stamp_bytes&7))return -1;
    if(s->stamp_bytes==256)return 1;
    stamp=s->stamps+s->stamp_bytes/8;
    stamp->asset.data=asset->data;stamp->asset.size=asset->size;
    stamp->asset.tiles=asset->tiles;stamp->asset.tiles_size=asset->tiles_size;
    stamp->offset=offset;stamp->tile=tile;s->stamp_bytes=(uint16_t)(s->stamp_bytes+8);return 0;
}
int ntm_course_screen_draw(ntm_course_screen *s,uint16_t selection,const ntm_screen_asset assets[3])
{
    if(!screen_producer_valid(s,selection,assets))return -1;
    s->displayed_selection=(uint8_t)selection;s->cursor_x=(uint16_t)(8+48*selection);
    (void)ntm_course_screen_enqueue_fill(s,0x300,20,6,0);
    (void)ntm_course_screen_enqueue_stamp(s,assets+selection,(uint16_t)(0x302+12*selection),301);
    return 0;
}
int ntm_course_screen_tick(ntm_course_screen *s,uint16_t selection,const ntm_screen_asset assets[3])
{
    if(!screen_producer_valid(s,selection,assets))return -1;
    if(s->cached_selection==selection)return 0;
    s->cached_selection=selection;
    return ntm_course_screen_draw(s,selection,assets)==0?1:-1;
}

static unsigned screen_word(const uint8_t *p){return (unsigned)p[0]|((unsigned)p[1]<<8);}
static int screen_stamp_valid(const ntm_screen_stamp *s)
{
    size_t p;unsigned n,i,y,rows,width,first_width=0,wide;
    int left,right;
    if(!s || !s->asset.data || !s->asset.tiles || s->asset.size<7)return 0;
    n=screen_word(s->asset.data);p=6u+2u*n;
    if(!n || p>=s->asset.size || n>512u-(s->tile&511u))return 0;
    for(i=0;i<n;i++)if((size_t)screen_word(s->asset.data+6+2*i)+16>s->asset.tiles_size)return 0;
    rows=s->asset.data[p++];wide=(rows&128)!=0;rows&=127;if(!rows)return 0;
    for(y=0;y<rows;y++) {
        width=0;
        for(;;) {
            unsigned v;if(p+(wide?2u:1u)>s->asset.size)return 0;v=wide?screen_word(s->asset.data+p):s->asset.data[p];p+=wide?2u:1u;
            if(v==(wide?65535u:255u)){if(!width)return 0;break;}
            if(++width>2048)return 0;
        }
        if(!y)first_width=width;
        left=(int)s->offset+64*(int)y;
        if(s->tile&0x8000) {
            right=left+2*((int)first_width-1);
            left+=2*((int)first_width-(int)width);
        } else right=left+2*((int)width-1);
        if(left<0 || right>4094 || (left&1))return 0;
    }
    return 1;
}
int ntm_course_screen_stamp_valid(const ntm_screen_stamp *s)
{
    return screen_stamp_valid(s)?0:-1;
}
int ntm_course_screen_stamp(const ntm_screen_stamp *s,uint8_t tiles[8192],uint8_t maps[4096])
{
    size_t p,q;unsigned n,i,y,rows,v,wide,first_width=0;
    int offset,step;
#ifndef NTM_SCREEN_PRIVATE_ASSETS
    unsigned j;
#endif
    if(!tiles || !maps || !screen_stamp_valid(s))return -1;
    n=screen_word(s->asset.data);
#ifdef NTM_SCREEN_PRIVATE_ASSETS
    for(i=0;i<n;i++)ntm_course_screen_copy_private(tiles+16*((s->tile&511u)+i),s->asset.tiles+screen_word(s->asset.data+6+2*i),16);
#else
    for(i=0;i<n;i++)for(j=0;j<16;j++)tiles[16*((s->tile&511u)+i)+j]=s->asset.tiles[screen_word(s->asset.data+6+2*i)+j];
#endif
    p=6+2*n;rows=s->asset.data[p++];wide=(rows&128)!=0;rows&=127;
    /* Original2005b5 scans only row zero for the reflection anchor. Later
     * rows may be shorter or longer; they retain that same right edge. */
    if(s->tile&0x8000) {
        q=p;
        while((wide?screen_word(s->asset.data+q):s->asset.data[q])!=(wide?65535u:255u)) {
            first_width++;q+=wide?2u:1u;
        }
    }
    step=(s->tile&0x8000)?-2:2;
    for(y=0;y<rows;y++) {
        offset=(int)s->offset+64*(int)y;
        if(s->tile&0x8000)offset+=2*((int)first_width-1);
        while((v=wide?screen_word(s->asset.data+p):s->asset.data[p])!=(wide?65535u:255u)) {
            v=((v+(s->tile&0x3fff))&65535)^(s->tile&0x8000);maps[offset]=(uint8_t)v;maps[offset+1]=(uint8_t)(v>>8);offset+=step;p+=wide?2u:1u;
        }
        p+=wide?2u:1u;
    }
    return 0;
}

static int screen_fill_valid(const ntm_screen_fill *f)
{
    return f->width && f->height && !(f->offset&1) &&
        (unsigned)f->offset+64u*(f->height-1u)+2u*f->width<=4096;
}
int ntm_course_screen_fill_queue(ntm_course_screen *s,uint8_t maps[4096])
{
    unsigned i,x,y;
    if(!s || !maps)return -1;
    for(i=0;i<8;i++)if(s->fill_status[i]!=255 && !screen_fill_valid(s->fills+i))return -1;
    for(i=8;i;i--)if(s->fill_status[i-1]!=255) {
        const ntm_screen_fill *f=s->fills+i-1;s->fill_status[i-1]=255;
        for(y=0;y<f->height;y++)for(x=0;x<f->width;x++) {
            unsigned offset=f->offset+64*y+2*x;
            maps[offset]=(uint8_t)f->fill;maps[offset+1]=(uint8_t)(f->fill>>8);
        }
    }
    return 0;
}
int ntm_course_screen_stamp_queue(ntm_course_screen *s,uint8_t tiles[8192],uint8_t maps[4096])
{
    unsigned i;
    if(!s || !tiles || !maps || s->stamp_bytes>256 || (s->stamp_bytes&7))return -1;
    for(i=0;i<s->stamp_bytes/8;i++)if(!screen_stamp_valid(s->stamps+i))return -1;
    for(i=0;i<s->stamp_bytes/8;i++)(void)ntm_course_screen_stamp(s->stamps+i,tiles,maps);
    s->stamp_bytes=0;return 0;
}
