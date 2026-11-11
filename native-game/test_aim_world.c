/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "aim_world.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t layout[65536],blocks[1024],indices[512],tilesource[4096],commands[43],colors[128],monosource[8];
static uint8_t tiles[8192],maps[4096],palette[512],mono[24],bg;
static uint8_t before_tiles[8192],before_maps[4096],before_palette[512],before_mono[24],before_bg;
static ntm_aim_world_state state,before_state;
static ntm_aim_world_resources resources;
static void w(uint8_t *p,unsigned n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static unsigned word(const uint8_t *p){return p[0]+256u*p[1];}
static void initialize(void)
{
 unsigned i;
 memset(&resources,0,sizeof resources);
 for(i=0;i<65536;i+=2)w(layout+i,(i/2)&31u);
 for(i=0;i<1024;i++)blocks[i]=(uint8_t)(i^(i>>8));
 for(i=0;i<4096;i++)tilesource[i]=(uint8_t)(i^(i>>7));
 for(i=0;i<128;i++)colors[i]=(uint8_t)(i^77u);
 for(i=0;i<8;i++)monosource[i]=(uint8_t)(i+253u);
 for(i=0;i<255;i++)w(indices+2+2*i,(i+1)&255u);
 w(indices,0xab03); /* Only low count byte is consumed. */
 commands[0]=0xff;w(commands+1,0x0def);
 for(i=0;i<16;i++)w(commands+3+2*i,i==5?0x8000:8*i);
 w(commands+35,0xffff);w(commands+37,0);w(commands+39,4);w(commands+41,0xffff);
 resources.layout=(ntm_aim_world_span){layout,sizeof layout,32768};
 resources.blocks=(ntm_aim_world_span){blocks,sizeof blocks,0};
 resources.tile_indices=(ntm_aim_world_span){indices,sizeof indices,0};
 resources.tiles=(ntm_aim_world_span){tilesource,sizeof tilesource,0};
 resources.commands=(ntm_aim_world_span){commands,sizeof commands,0};
 resources.colors=(ntm_aim_world_span){colors,sizeof colors,0};
 resources.mono=(ntm_aim_world_span){monosource,sizeof monosource,0};
 resources.width=7;resources.height=19;
 resources.layout_tag=0x12345678;resources.blocks_tag=0x23456789;
 resources.terrain_tag=0x3456789a;resources.tiles_tag=0x456789ab;
}
static void reset(unsigned x,unsigned y,unsigned layer)
{
 memset(&state,0x5a,sizeof state);w(state.bytes,x);w(state.bytes+2,y);
 w(state.bytes+44,0x9000+2048*layer);w(state.bytes+46,0x8108+8*layer);w(state.bytes+48,0x8280+128*layer);
 memset(tiles,0x37,sizeof tiles);memset(maps,0x75,sizeof maps);memset(palette,0x89,sizeof palette);memset(mono,6,sizeof mono);bg=0x81;
}
static void save(void)
{before_state=state;memcpy(before_tiles,tiles,sizeof tiles);memcpy(before_maps,maps,sizeof maps);memcpy(before_palette,palette,sizeof palette);memcpy(before_mono,mono,sizeof mono);before_bg=bg;}
static int call(unsigned mode)
{return ntm_aim_world_load(&state,&resources,(uint8_t)mode,tiles,maps,palette,mono,&bg);}
static void rejected(unsigned mode)
{
 assert(call(mode)==-1);assert(!memcmp(&state,&before_state,sizeof state));assert(!memcmp(tiles,before_tiles,sizeof tiles));assert(!memcmp(maps,before_maps,sizeof maps));assert(!memcmp(palette,before_palette,sizeof palette));assert(!memcmp(mono,before_mono,sizeof mono));assert(bg==before_bg);
}
static int floor32(unsigned n)
{int x=(int)(n&65535u);if(x>=32768)x-=65536;return x>=0?x/32:-((-x+31)/32);}
int main(void)
{
 unsigned n,i,j,k,layer,base,x,y,index,block,dest;ntm_aim_world_resources saved;
 initialize();
 for(n=0;n<65536;n++){
  layer=n&1u;reset(n,65535u-n,layer);memcpy(before_maps,maps,sizeof maps);
  assert(call(16)==0);assert(bg==0xc7);assert(word(state.bytes)==n);assert(word(state.bytes+2)==65535u-n);
  assert(word(state.bytes+4)==(n&65504u));assert(word(state.bytes+6)==((65535u-n)&65504u));
  assert(word(state.bytes+28)==0x1300);assert(word(state.bytes+40)==(uint16_t)((n&65504u)+192));
  assert(state.bytes[50]==0x5a && state.bytes[51]==0x5a);
  base=2048*layer;
  for(j=0;j<7;j++)for(i=0;i<7;i++){
   x=(n&65504u)-32+32*i;y=((65535u-n)&65504u)-32+32*j;
   index=(unsigned)(floor32(y)*7+floor32(x));block=index&31u;
   dest=base+(((y>>3)&31u)*32u+((x>>3)&31u))*2u;
   for(k=0;k<4;k++)memcpy(before_maps+dest+64*k,blocks+32*block+8*k,8);
  }
  assert(!memcmp(maps,before_maps,sizeof maps));
 }
 reset(160,240,0);assert(call(16)==0);
 assert(!memcmp(tiles+0x1300,tilesource+16,48));assert(tiles[0x12ff]==0x37 && tiles[0x1330]==0x37);
 assert(!memcmp(palette+128,colors,8));assert(palette[168]==0x89);assert(word(palette+0x1ee)==0x0def);
 reset(160,240,1);save();rejected(15);
 /* The first negative command terminates monochrome's scan, even before the
  * fixed16 color entries end. Here it is entry5, leaving entries6 and7. */
 w(commands+13,40);w(commands+35,0xffff);w(commands+37,0);w(commands+39,4);
 reset(160,240,1);assert(call(0)==0);assert(mono[16]==6 && mono[20]==6);
 for(i=1;i<4;i++){assert(mono[16+i]==(monosource[i]&7u));assert(mono[20+i]==(monosource[4+i]&7u));}
 w(indices,0xff00);reset(0,0,1);assert(call(16)==0);assert(word(state.bytes+28)==0);
 for(i=0;i<8192;i++)assert(tiles[i]==0x37);
 w(indices,255);reset(0,0,1);assert(call(16)==0);assert(!memcmp(tiles+0x1300+207*16,tilesource+208*16,16));
 /* Originals write remaining tile copies into unmappedc000..c2ef. */
 w(indices,3);reset(160,240,0);save();saved=resources;
 resources.layout.data=NULL;rejected(16);resources=saved;
 resources.layout.origin=65537;rejected(16);resources=saved;
 resources.blocks.size=1;rejected(16);resources=saved;
 resources.tile_indices.size=7;rejected(16);resources=saved;
 resources.tiles.size=63;rejected(16);resources=saved;
 resources.commands.size=34;rejected(16);resources=saved;
 resources.colors.size=127;rejected(16);resources=saved;
 resources.mono.size=7;rejected(0);resources=saved;
 resources.commands.size=36;rejected(0);resources=saved;
 w(state.bytes+44,0x9400);save();rejected(16);
 reset(160,240,0);w(state.bytes+48,0x8382);save();rejected(16);
 reset(160,240,0);w(state.bytes+46,0x8114);save();rejected(0);
 puts("aim world: 65536 wrapped origin pairs, both layers, exact color/mono branches, tile count limits, and atomic malformed-span rejection");return 0;
}
