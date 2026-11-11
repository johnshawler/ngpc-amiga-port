/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_screen.h"
static int screen_producer_valid(const ntm_course_screen *s,uint16_t selection,const ntm_screen_asset assets[3])
{
    return s && assets && selection<3 && s->stamp_bytes<=256 && !(s->stamp_bytes&7);
}
int ntm_course_screen_draw(ntm_course_screen *s,uint16_t selection,const ntm_screen_asset assets[3])
{
    unsigned i;
    if(!screen_producer_valid(s,selection,assets))return -1;
    s->displayed_selection=(uint8_t)selection;s->cursor_x=(uint16_t)(8+48*selection);
    for(i=8;i;i--)if(s->fill_status[i-1]==255) {
        ntm_screen_fill *f=s->fills+i-1;
        f->offset=0x300;f->width=20;f->height=6;f->fill=0;s->fill_status[i-1]=0;break;
    }
    if(s->stamp_bytes<256) {
        ntm_screen_stamp *stamp=s->stamps+s->stamp_bytes/8;
        stamp->asset.data=assets[selection].data;stamp->asset.size=assets[selection].size;
        stamp->asset.tiles=assets[selection].tiles;stamp->asset.tiles_size=assets[selection].tiles_size;
        stamp->offset=(uint16_t)(0x302+12*selection);stamp->tile=301;
        s->stamp_bytes=(uint16_t)(s->stamp_bytes+8);
    }
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
    size_t p;unsigned n,i,y,rows,offset;
    if(!s || !s->asset.data || !s->asset.tiles || s->asset.size<7 || (s->tile&0xc000))return 0;
    n=screen_word(s->asset.data);p=6u+2u*n;
    if(!n || p>=s->asset.size || n>512u-(s->tile&511u))return 0;
    for(i=0;i<n;i++)if((size_t)screen_word(s->asset.data+6+2*i)+16>s->asset.tiles_size)return 0;
    rows=s->asset.data[p++];if(!(rows&128))return 0;rows&=127;if(!rows)return 0;
    for(y=0;y<rows;y++) {
        offset=s->offset+64*y;
        for(;;) {
            unsigned v;if(p+2>s->asset.size)return 0;v=screen_word(s->asset.data+p);p+=2;
            if(v==65535){if(offset==s->offset+64*y)return 0;break;}
            if(offset>4094 || (offset&1))return 0;
            offset+=2;
        }
    }
    return 1;
}
int ntm_course_screen_stamp(const ntm_screen_stamp *s,uint8_t tiles[8192],uint8_t maps[4096])
{
    size_t p;unsigned n,i,j,y,rows,offset,v;
    if(!tiles || !maps || !screen_stamp_valid(s))return -1;
    n=screen_word(s->asset.data);
    for(i=0;i<n;i++)for(j=0;j<16;j++)tiles[16*((s->tile&511u)+i)+j]=s->asset.tiles[screen_word(s->asset.data+6+2*i)+j];
    p=6+2*n;rows=s->asset.data[p++]&127;
    for(y=0;y<rows;y++) {
        offset=s->offset+64*y;
        while((v=screen_word(s->asset.data+p))!=65535) {
            v=(v+s->tile)&65535;maps[offset]=(uint8_t)v;maps[offset+1]=(uint8_t)(v>>8);offset+=2;p+=2;
        }
        p+=2;
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
