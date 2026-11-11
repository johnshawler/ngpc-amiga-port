/* SPDX-License-Identifier: GPL-2.0-or-later. Cartridge-free frontend checks. */
#include "selector_frontend.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t seed=0x51ec701u;
static uint32_t random32(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static uint8_t tiles[2][8192],maps[2][4096],sprites[2][256],pals[2][64];
static void fill(uint8_t *p,unsigned n){while(n--)*p++=(uint8_t)random32();}
int main(void)
{
    unsigned i,j,test,marked=0;selector_clock c={0};uint8_t palette[512]={0};
    assert(selector_joystick(0,255,65535)==0);
    assert(selector_joystick(0x300,255,65535)==4);
    assert(selector_joystick(3,255,65535)==8);
    assert(selector_joystick(0x100,255,65535)==1);
    assert(selector_joystick(1,255,65535)==2);
    assert(selector_joystick(0,127,65535)==16);
    assert(selector_joystick(0,255,0xbfff)==32);
    assert(selector_joystick(0x100,127,65535)==81);
    for(i=0;i<400;i++)selector_due(&c,(uint32_t)(((uint64_t)(i+1)*SELECTOR_CIA_HZ)/50-((uint64_t)i*SELECTOR_CIA_HZ)/50),8);
    assert(c.updates==241 && c.dropped==0 && c.phase==0 && c.sample==8);
    assert(selector_due(&c,SELECTOR_CIA_HZ*8u,0)==0 && c.dropped==241);
    palette[2]=0x23;palette[3]=1;
    assert(selector_color(palette,1,0)==0x321 && selector_color(palette,1,1)==0xcde);
    for(test=0;test<384;test++){
        nr_scene a={0},b={0};uint8_t dirty[152],oldrow[160],newrow[160];
        fill(tiles[0],8192);fill(maps[0],4096);fill(sprites[0],256);fill(pals[0],64);
        memcpy(tiles[1],tiles[0],8192);memcpy(maps[1],maps[0],4096);memcpy(sprites[1],sprites[0],256);memcpy(pals[1],pals[0],64);
        a.tiles=tiles[0];a.maps=maps[0];a.sprites=sprites[0];a.sprite_pal=pals[0];
        a.scroll_x[0]=(uint8_t)random32();a.scroll_x[1]=(uint8_t)random32();a.scroll_y[0]=(uint8_t)random32();a.scroll_y[1]=(uint8_t)random32();
        a.sprite_x=(uint8_t)random32();a.sprite_y=(uint8_t)random32();
        a.window_x=(uint8_t)(random32()%80);a.window_y=(uint8_t)(random32()%76);a.window_w=(uint8_t)(80+random32()%80);a.window_h=(uint8_t)(76+random32()%76);
        a.background=(uint8_t)random32();a.outside=(uint8_t)random32();a.swap=(uint8_t)(random32()&1);
        b=a;b.tiles=tiles[1];b.maps=maps[1];b.sprites=sprites[1];b.sprite_pal=pals[1];
        switch(test%12){
            case 0:break;
            case 1:for(i=0;i<5;i++)tiles[1][random32()%8192]^=(uint8_t)random32();break;
            case 2:for(i=0;i<5;i++)maps[1][random32()%4096]^=(uint8_t)random32();break;
            case 3:for(i=0;i<5;i++)sprites[1][random32()%256]^=(uint8_t)random32();break;
            case 4:pals[1][random32()%64]^=7;break;
            case 5:b.sprite_x++;break;case 6:b.sprite_y--;break;
            case 7:b.window_x++;break;case 8:b.window_h--;break;
            case 9:b.scroll_x[0]++;break;case 10:b.background++;break;
            case 11:fill(tiles[1],8192);fill(sprites[1],256);break;
        }
        selector_dirty_rows(&a,&b,dirty);
        for(j=0;j<152;j++){
            assert(!nr_scene_row(&a,j,oldrow) && !nr_scene_row(&b,j,newrow));
            if(memcmp(oldrow,newrow,160))assert(dirty[j]);
            if(test%12==0)assert(!dirty[j]);
            marked+=dirty[j]!=0;
        }
        selector_dirty_rows(0,&b,dirty);for(j=0;j<152;j++)assert(dirty[j]);
    }
    printf("PASS selector frontend: joystick, both palette polarities, 241/8 source cadence, bounded long stalls, 384 exact invalidation cases (%u marked rows)\n",marked);
    return 0;
}
