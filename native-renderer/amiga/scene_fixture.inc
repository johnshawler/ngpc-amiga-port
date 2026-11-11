/* SPDX-License-Identifier: GPL-2.0-or-later. Synthetic native scene fixture. */
#include "../scene_renderer.c"
static void fill_scene(uint8_t *bitmap,uint8_t *tiles)
{
    uint8_t *maps=tiles+8192,*sprites=maps+4096,*pal=sprites+256;
    uint8_t row[160];unsigned i,y,x,p;
    nr_scene s={tiles,maps,sprites,pal,{251,7},{253,19},249,250,3,2,154,147,1,2,5};
    for(i=0;i<8192;i++)tiles[i]=(uint8_t)((i*37u)^(i>>3));
    for(i=0;i<2048;i++) {
        unsigned d=(i&511)|(((i/7)&15)<<9)|((i&1)?0x8000:0)|((i&2)?0x4000:0);
        maps[i*2]=(uint8_t)d;maps[i*2+1]=(uint8_t)(d>>8);
    }
    for(i=0;i<64;i++) {
        unsigned d=(i*3)|((i%4)<<11)|((i&1)?0x400:0)|((i&2)?0x200:0);
        sprites[i*4]=(uint8_t)d;sprites[i*4+1]=(uint8_t)(d>>8);
        sprites[i*4+2]=(uint8_t)(i*11);sprites[i*4+3]=(uint8_t)(i*7);
        pal[i]=(uint8_t)(i&15);
    }
    for(y=0;y<152;y++) {
        nr_scene_row(&s,y,row);
        for(x=0;x<160;x++) for(p=0;p<8;p++)
            if(row[x]&(1u<<p))bitmap[p*10240+(y+52)*40+(x+80)/8]|=(uint8_t)(128u>>((x+80)&7));
    }
}
void *memcpy(void *dst,const void *src,size_t size)
{unsigned char *d=dst;const unsigned char *s=src;while(size--)*d++=*s++;return dst;}
