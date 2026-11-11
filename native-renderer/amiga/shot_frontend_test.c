/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_frontend.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t tiles[8192],maps[4096],sprites[256],pal[64],next_tiles[8192],next_maps[4096];
int main(void)
{
 unsigned hi,lo,mode,y,f;uint32_t cases=0;
 for(hi=0;hi<256;hi++)for(lo=0;lo<256;lo++)for(mode=0;mode<4;mode++){
  ntm_shot_raster r={(uint8_t)lo,(uint8_t)hi,(uint8_t)(mode&1?137:136),0,1,(uint8_t)(mode&2?192:128),1,0,0};
  shot_rows a={(uint8_t)(hi^lo),(uint8_t)(hi+lo),{0},0};uint8_t raw=a.raw,latch=a.latched,shown,expected[152];
  for(f=0;f<2;f++)for(y=0;y<199;y++){
   unsigned line=y<47?y+152:y-47;
   assert(!ntm_shot_raster_step(&r,line,198,&raw,&latch,&shown));
   if(line<152)expected[line]=shown;
  }
  assert(!shot_rows_update(&a,&r,17));assert(a.raw==raw && a.latched==latch && a.frames==2 && !memcmp(a.plan,expected,152));cases++;
 }
 {shot_rows a={0},old;ntm_shot_raster r={0};
  assert(!shot_rows_update(&a,&r,91));for(y=0;y<152;y++)assert(a.plan[y]==91);
  old=a;r.callback=2;assert(shot_rows_update(&a,&r,0)<0 && !memcmp(&a,&old,sizeof a));
 }
 {nr_scene a={0},b;uint8_t old[152]={0},next[152]={0},dirty[152];
  a.tiles=tiles;a.maps=maps;a.sprites=sprites;a.sprite_pal=pal;a.window_w=160;a.window_h=152;b=a;
  shot_dirty_rows(&a,old,&b,next,dirty,0);for(y=0;y<152;y++)assert(!dirty[y]);
  assert(shot_rows_uniform(&a,next));next[72]=12;assert(!shot_rows_uniform(&a,next));
  shot_dirty_rows(&a,old,&b,next,dirty,0);assert(dirty[72] && !dirty[71] && !dirty[73]);
  b.tiles=next_tiles;b.maps=next_maps;next_maps[0]=1;
  shot_dirty_rows(&a,old,&b,next,dirty,0);for(y=0;y<152;y++)assert(dirty[y]);
 }
 printf("shot frontend:%u coalesced/full-frame cases;152 row dependencies;invalid atomic rejection\n",cases);return 0;
}
