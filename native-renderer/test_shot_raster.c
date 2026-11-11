/* SPDX-License-Identifier: GPL-2.0-or-later
 * Synthetic per-row composition proof; no original art or target timing claim. */
#include "scene_renderer.h"
#include "../native-game/shot_raster.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t tiles[8192],maps[4096],sprites[256],palette[64],planes[24320],row[160];
static void word(uint8_t *p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
int main(void)
{
    ntm_shot_raster raster={37,159,0,0,0,128,0,0,0};
    uint8_t raw=159,latched=37,shown=0;unsigned x,y,i;
    nr_scene base={0};nr_scene_cache cache;nr_bitmap target={planes,sizeof planes,160,152,20};
    for(i=0;i<8;i++){word(tiles+16+2*i,0x5500);word(tiles+32+2*i,0x9555);word(tiles+48+2*i,0xffff);}
    for(i=0;i<1024;i++){word(maps+2*i,1);word(maps+2048+2*i,2);}
    word(sprites,3|(3<<11));sprites[2]=80;sprites[3]=70;
    base.tiles=tiles;base.maps=maps;base.sprites=sprites;base.sprite_pal=palette;
    base.window_w=160;base.window_h=152;base.background=129;base.outside=7;
    assert(!ntm_shot_raster_install(&raster));nr_scene_cache_init(&cache);
    for(y=0;y<=198;y++){
        nr_scene current=base;
        assert(!ntm_shot_raster_step(&raster,y,198,&raw,&latched,&shown));
        if(y>=152)continue;
        current.scroll_x[0]=shown;
        assert(!nr_scene_row(&current,y,row));
        assert(!nr_scene_row_planar_words(&current,y,&target,&cache));
        for(x=0;x<160;x++){
            unsigned expected=(y>=70 && y<78 && x>=80 && x<88)?3:
                (((x+shown)&7)<4?65:(x&7)?129:130);
            unsigned reconstructed=0;
            for(i=0;i<8;i++)if(planes[i*3040+y*20+x/8]&(128>>(x&7)))reconstructed|=1u<<i;
            assert(row[x]==expected && reconstructed==expected);
        }
    }
    assert(raw==159 && latched==37);
    puts("shot raster compositor:24320 synthetic pixels/8bitplanes;only plane1scrolls,sprites/plane2 remain anchored");return 0;
}
