/* SPDX-License-Identifier: GPL-2.0-or-later. Cartridge-free cache-key checks. */
#include "selector_planar_cache.h"
#include "selector_memory.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t tiles[8192],maps[4096],sprites[256],palettes[64];
static selector_planar_entry entry;
static unsigned pump_count;
static void pump(void){pump_count++;}
static int find(nr_scene *scene){selector_planar_stats stats={0};return selector_planar_find(scene,&entry,1,0,&stats,pump);}
int main(void)
{
    nr_scene s={0};nr_bitmap bitmap={entry.planar,sizeof entry.planar,160,152,20};
    uint8_t row[160],baseline[24320],actual[24320];unsigned i,j,misses=0;
    uint8_t *controls[13];
    assert(!(offsetof(selector_planar_entry,tiles)&3) && !(offsetof(selector_planar_entry,planar)&3));
    assert(!(sizeof(selector_planar_entry)&3) && !((uintptr_t)&entry&3));
    {uint8_t left[128],right[128];unsigned start,source,length,k;
     for(start=0;start<4;start++)for(source=0;source<4;source++)for(length=0;length<100;length++){
      for(k=0;k<128;k++){left[k]=(uint8_t)(k*19+7);right[k]=0xa5;}
      selector_span_copy(right+start,left+source,length);
      assert(selector_span_equal(right+start,left+source,length));
      for(k=0;k<128;k++)assert(right[k]==(k>=start && k<start+length?left[k-start+source]:0xa5));
      if(length){right[start+length/2]^=1;assert(!selector_span_equal(right+start,left+source,length));}
     }}
    s.tiles=tiles;s.maps=maps;s.sprites=sprites;s.sprite_pal=palettes;
    s.window_w=160;s.window_h=152;s.background=128;s.outside=7;
    for(i=0;i<8192;i++)tiles[i]=(uint8_t)(i*37u+(i>>3));
    for(i=0;i<2048;i++){maps[2*i]=(uint8_t)(1+(i&1));maps[2*i+1]=(uint8_t)((i%16)<<1);}
    sprites[0]=(uint8_t)450;sprites[1]=(uint8_t)((450|0x1800)>>8);sprites[2]=40;sprites[3]=50;palettes[0]=3;
    selector_planar_key(&s,&entry);
    for(i=0;i<512;i++)assert(((entry.referenced[i/8]>>(i&7))&1)==(i==0||i==1||i==2||i==450));
    for(i=0;i<152;i++){assert(!nr_scene_row(&s,i,row));assert(!nr_store_scene_row(&bitmap,i,row));}
    memcpy(baseline,entry.planar,sizeof baseline);assert(find(&s)==0 && pump_count);
    controls[0]=&s.scroll_x[0];controls[1]=&s.scroll_x[1];controls[2]=&s.scroll_y[0];controls[3]=&s.scroll_y[1];
    controls[4]=&s.sprite_x;controls[5]=&s.sprite_y;controls[6]=&s.window_x;controls[7]=&s.window_y;
    controls[8]=&s.window_w;controls[9]=&s.window_h;controls[10]=&s.swap;controls[11]=&s.background;controls[12]=&s.outside;
    for(i=0;i<13;i++){*controls[i]^=1;assert(find(&s)<0);*controls[i]^=1;misses++;}
    for(i=0;i<4096;i++){maps[i]^=1;assert(find(&s)<0);maps[i]^=1;misses++;}
    for(i=0;i<256;i++){sprites[i]^=1;assert(find(&s)<0);sprites[i]^=1;misses++;}
    for(i=0;i<64;i++){palettes[i]^=1;assert(find(&s)<0);palettes[i]^=1;misses++;}
    for(i=0;i<512;i++)if(entry.referenced[i/8]&(1u<<(i&7)))for(j=0;j<16;j++){
        tiles[i*16+j]^=1;assert(find(&s)<0);tiles[i*16+j]^=1;misses++;
    }
    /* Stale bytes from an old label must not affect keys or visible pixels. */
    for(j=0;j<16;j++)tiles[511*16+j]^=255;
    assert(find(&s)==0);bitmap.data=actual;
    for(i=0;i<152;i++){assert(!nr_scene_row(&s,i,row));assert(!nr_store_scene_row(&bitmap,i,row));}
    assert(!memcmp(actual,baseline,sizeof baseline));
    /* All non-full fade widths must reject a full-window cache entry. */
    for(i=0;i<160;i++){s.window_w=(uint8_t)i;assert(find(&s)<0);misses++;}
    s.window_w=160;assert(find(&s)==0);
    printf("PASS exact planar key: %u invalid-key misses, all controls/maps/sprites/palette indices/referenced tiles, 160 fade widths, harmless unreferenced mutation\n",misses);
    return 0;
}
