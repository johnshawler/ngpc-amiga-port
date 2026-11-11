/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "tee_graphics.h"
static unsigned word(const uint8_t *p){return p[0]+256u*p[1];}
int ntm_tee_copy_tiles(const ntm_screen_asset *a,uint16_t offset,uint8_t tiles[8192])
{
    unsigned n,i,j;
    if(!a || !a->data || !a->tiles || !tiles || a->size<6 || (offset&1))return -1;
    n=word(a->data);
    if(!n || n>512 || a->size<6u+2u*n || (unsigned)offset+16u*n>8192)return -1;
    for(i=0;i<n;i++)if((size_t)word(a->data+6+2*i)+16>a->tiles_size)return -1;
    for(i=0;i<n;i++)for(j=0;j<16;j++)tiles[offset+16*i+j]=a->tiles[word(a->data+6+2*i)+j];
    return 0;
}
int ntm_tee_draw_number(uint16_t value,uint8_t digits,uint8_t color,
    uint16_t text_base,uint16_t offset,uint32_t *scratch,uint8_t maps[4096])
{
    uint8_t v[4];unsigned i;uint32_t out;
    if(!scratch || !maps || !digits || digits>4 || (offset&1) || (unsigned)offset+2u*digits>4096)return -1;
    out=*scratch;
    for(i=digits;i;i--){v[i-1]=(uint8_t)(value%10u);value=(uint16_t)(value/10u);}
    for(i=0;i+1u<digits && !v[i];i++)v[i]=16;
    for(i=0;i<digits;i++) {
        uint16_t tile=(uint16_t)(v[i]+16u+((unsigned)color<<8)+text_base);
        out=(out&~((uint32_t)255<<(8*i)))|((uint32_t)v[i]<<(8*i));
        maps[offset+2*i]=(uint8_t)tile;maps[offset+2*i+1]=(uint8_t)(tile>>8);
    }
    *scratch=out;return 0;
}
