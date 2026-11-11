/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_terrain.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t hole[256],map[8]={2,0,1,0,0},masks[8],shapes[2048];
static void set(unsigned at,unsigned v){hole[at]=(uint8_t)v;hole[at+1]=(uint8_t)(v>>8);}
int main(void)
{
    ntm_terrain_resources r={{map,5},{map,5},masks,shapes,8,2048};
    ntm_terrain_result result,saved;unsigned code,index,x,y,cases=0;uint16_t value;
    set(0x2a,100);set(0x2c,100);set(0x2e,500);set(0x30,500);set(0x3c,500);set(0x3e,500);
    for(x=0;x<8;x++)masks[x]=(uint8_t)(1u<<x);
    for(index=0;index<256;index++)for(y=0;y<8;y++)shapes[index*8+y]=(uint8_t)(index^(y*37));
    for(code=0;code<256;code++)for(index=0;index<256;index++)for(x=0;x<8;x++)for(y=0;y<8;y++){
        unsigned mask=masks[x]&shapes[index*8+y],detail=0,terrain;
        map[3]=(uint8_t)code;map[4]=(uint8_t)index;
        result.x=600;result.y=601;result.detail=602;result.terrain=99;
        if(code<208)terrain=(mask?code>>4:code)&15;
        else {unsigned v=code-208;terrain=3;if(mask)detail=(uint8_t)(((v<<4)|(v>>4))+1);}
        assert(!ntm_shot_terrain(hole,0,(uint16_t)x,(uint16_t)y,&r,&result));
        assert(result.x==x && result.y==y && result.detail==detail && result.terrain==terrain);cases++;
    }
    memset(&result,0xa5,sizeof result);saved=result;
    for(x=0;x<5;x++){r.large.size=x;assert(ntm_shot_terrain(hole,0,0,0,&r,&result)==-1);assert(!memcmp(&result,&saved,sizeof result));}
    r.large.size=5;map[2]=0;assert(ntm_shot_terrain(hole,0,0,0,&r,&result)==-1);assert(!memcmp(&result,&saved,sizeof result));map[2]=1;
    r.shapes_size=0;assert(ntm_shot_terrain(hole,0,0,0,&r,&result)==-1);assert(!memcmp(&result,&saved,sizeof result));r.shapes_size=2048;
    assert(!ntm_shot_terrain(hole,0,212,0,&r,&result));assert(result.x==saved.x && result.y==saved.y && result.detail==saved.detail && result.terrain==5);
    set(0x2e,20);set(0x30,20);result=saved;assert(!ntm_shot_terrain(hole,0,10,13,&r,&result));assert(result.x==10 && result.y==13 && result.detail==saved.detail && result.terrain==4);
    assert(ntm_terrain_map_word(&r.large,0,512,&value)==-1);assert(ntm_terrain_map_word(0,0,0,&value)==-1);
    assert(ntm_shot_terrain(0,0,0,0,&r,&result)==-1);assert(ntm_shot_terrain(hole,0,0,0,0,&result)==-1);assert(ntm_shot_terrain(hole,0,0,0,&r,0)==-1);
    printf("PASS terrain: %u tile/pixel cases, bounded RLE, untouched early fields, atomic invalid spans\n",cases);return 0;
}
