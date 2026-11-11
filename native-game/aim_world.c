/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "aim_world.h"
#include <string.h>
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static void putword(uint8_t *p,uint16_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static void putlong(uint8_t *p,uint32_t n){putword(p,(uint16_t)n);putword(p+2,(uint16_t)(n>>16));}
static int signed_word(uint16_t n){return n<32768u?(int)n:(int)n-65536;}
static uint16_t arithmetic_shift(uint16_t n,unsigned bits)
{return (uint16_t)((n>>bits)|((n&32768u)?(65535u<<(16-bits)):0));}
static const uint8_t *resolve(const ntm_aim_world_span *s,int offset,size_t n)
{
    size_t at;
    if(!s || !s->data || s->origin>s->size)return NULL;
    if(offset<0){size_t back=(size_t)(-offset);if(back>s->origin)return NULL;at=s->origin-back;}
    else {if((size_t)offset>s->size-s->origin)return NULL;at=s->origin+(size_t)offset;}
    if(n>s->size-at)return NULL;
    return s->data+at;
}
/* The original first shifts signed coordinates by3 for circular tile-map
 * addressing, then by2 for32-pixel blocks. Both address additions use signed
 * sixteen-bit offsets, including doubled row indices and block index<<5. */
static const uint8_t *cell(const ntm_aim_world_resources *r,uint16_t x,uint16_t y,unsigned *destination)
{
    uint16_t tx=arithmetic_shift(x,3),ty=arithmetic_shift(y,3),index,block;
    const uint8_t *p;
    *destination=((ty&31u)*32u+(tx&31u))*2u;
    index=(uint16_t)(arithmetic_shift(ty,2)*r->width+arithmetic_shift(tx,2));
    p=resolve(&r->layout,signed_word((uint16_t)(2u*index)),2);
    if(!p)return NULL;
    block=(uint16_t)(word(p)<<5);
    return resolve(&r->blocks,signed_word(block),32);
}
int ntm_aim_world_load(ntm_aim_world_state *s,const ntm_aim_world_resources *r,
    uint8_t mode,uint8_t tiles[8192],uint8_t maps[4096],uint8_t palette[512],
    uint8_t mono[24],uint8_t *background)
{
    const uint8_t *indices,*commands,*source;unsigned count,i,j,k,dest,mapbase,paldest;
    uint16_t x0,y0,x,y;size_t command_pos;uint8_t bg;
    if(!s || !r || !tiles || !maps || !palette || !background)return -1;
    mapbase=word(s->bytes+44);
    if(mapbase!=0x9000u && mapbase!=0x9800u)return -1;
    mapbase-=0x9000u;
    indices=resolve(&r->tile_indices,0,2);if(!indices)return -1;
    count=indices[0];indices=resolve(&r->tile_indices,2,2u*count);
    if(!indices)return -1;
    /* Only C, the low count byte, controls2014e5/201510. */
    for(i=0;i<count;i++)if(!resolve(&r->tiles,(int)word(indices+2*i)*16,16))return -1;
    commands=resolve(&r->commands,0,1);if(!commands)return -1;bg=commands[0];
    command_pos=1;
    if(mode>=16){
        paldest=word(s->bytes+48);
        if(paldest<0x8200u || paldest>0x8380u || (paldest&1u))return -1;
        commands=resolve(&r->commands,1,34);if(!commands)return -1;
        for(i=0;i<16;i++){
            uint16_t entry=word(commands+2+2*i);
            if(!(entry&32768u) && !resolve(&r->colors,signed_word(entry),8))return -1;
        }
    }else{
        if(!mono)return -1;
        paldest=word(s->bytes+46);
        if(paldest<0x8100u || paldest>0x8110u || (paldest&3u))return -1;
        /* Monochrome skips color entries through the actual negative sentinel. */
        do {
            if(command_pos>32767u)return -1;
            commands=resolve(&r->commands,(int)command_pos,2);if(!commands)return -1;
            command_pos+=2;
        }while(!(word(commands)&32768u));
        for(i=0;i<2;i++){
            commands=resolve(&r->commands,(int)(command_pos+2*i),2);if(!commands)return -1;
            if(!(word(commands)&32768u) && !resolve(&r->mono,signed_word(word(commands)),4))return -1;
        }
    }
    x0=(uint16_t)(word(s->bytes)&0xffe0u);y0=(uint16_t)(word(s->bytes+2)&0xffe0u);
    for(j=0;j<7;j++)for(i=0;i<7;i++){
        x=(uint16_t)(x0-32u+32u*i);y=(uint16_t)(y0-32u+32u*j);
        if(!cell(r,x,y,&dest) || dest+200u>2048u)return -1;
    }
    memset(s->bytes+4,0,40);
    putword(s->bytes+4,x0);putword(s->bytes+6,y0);
    putlong(s->bytes+8,r->layout_tag);putlong(s->bytes+12,r->blocks_tag);
    s->bytes[16]=r->width;s->bytes[17]=r->height;
    putlong(s->bytes+32,r->terrain_tag);putlong(s->bytes+36,r->tiles_tag);
    if(count){
        putword(s->bytes+28,0x1300);
        /* Later writes target unmappedc000..c2ef, not character RAM. */
        for(i=0;i<count && i<208;i++)memcpy(tiles+0x1300u+16u*i,resolve(&r->tiles,(int)word(indices+2*i)*16,16),16);
    }
    *background=bg&0xc7u;
    if(mode>=16){
        commands=resolve(&r->commands,1,34);
        memcpy(palette+0x1e0u+2u*(bg&7u),commands,2);
        for(i=0;i<16;i++){
            uint16_t entry=word(commands+2+2*i);
            if(!(entry&32768u))memcpy(palette+paldest-0x8200u+8*i,resolve(&r->colors,signed_word(entry),8),8);
        }
    }else{
        for(i=0;i<2;i++){
            commands=resolve(&r->commands,(int)(command_pos+2*i),2);
            if(!(word(commands)&32768u)){
                source=resolve(&r->mono,signed_word(word(commands)),4);
                /* Every fourth register is unmapped and ignores writes. */
                for(k=1;k<4;k++)mono[paldest-0x8100u+4*i+k]=source[k]&7u;
            }
        }
    }
    for(j=0;j<7;j++)for(i=0;i<7;i++){
        x=(uint16_t)(x0-32u+32u*i);y=(uint16_t)(y0-32u+32u*j);
        source=cell(r,x,y,&dest);
        for(k=0;k<4;k++)memcpy(maps+mapbase+dest+64*k,source+8*k,8);
    }
    putword(s->bytes+40,(uint16_t)(x0+192u));putword(s->bytes+42,(uint16_t)(y0+192u));
    return 0;
}
