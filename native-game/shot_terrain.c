/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_terrain.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
int ntm_terrain_map_word(const ntm_terrain_map *map,uint16_t x,uint16_t y,uint16_t *out)
{
    size_t at,row=(size_t)(y>>3)*2;unsigned column=x>>3;
    if(!map || !map->data || !out || row>map->size || map->size-row<2)return -1;
    at=word(map->data+row);
    for(;;){unsigned count;uint16_t value;
        if(at>map->size || map->size-at<3)return -1;
        count=map->data[at];value=word(map->data+at+1);at+=3;
        if(column<count){*out=value;return 0;}
        column-=count;
    }
}
static uint16_t magnitude(uint16_t a,uint16_t b)
{uint16_t d=(uint16_t)(a-b);return d&32768u?(uint16_t)(0u-d):d;}
int ntm_shot_terrain(const uint8_t hole[256],uint8_t status,uint16_t x,uint16_t y,
    const ntm_terrain_resources *r,ntm_terrain_result *result)
{
    ntm_terrain_result n;const ntm_terrain_map *map;uint16_t tile,dx,dy;unsigned code,mask;
    if(!hole || !r || !result)return -1;
    n=*result;map=&r->large;
    if(status!=255){
        if(x>=(uint16_t)(word(hole+0x2a)+112u) || y>=(uint16_t)(word(hole+0x2c)+152u)){
            n.terrain=5;*result=n;return 0;
        }
        n.x=x;n.y=y;
        if(magnitude(x,word(hole+0x2e))<=10 && magnitude(y,word(hole+0x30))<=7){
            n.terrain=4;*result=n;return 0;
        }
        dy=(uint16_t)(word(hole+0x3e)-y);dx=(uint16_t)(word(hole+0x3c)-x);
        if(!(dy&32768u) || ((uint16_t)(dy+55u)&32768u) ||
           !(dx&32768u) || ((uint16_t)(dx+55u)&32768u))goto lookup;
    }
    x=(uint16_t)(2u*(uint16_t)(x-word(hole+0x3c))+16u);
    y=(uint16_t)(2u*(uint16_t)(y-word(hole+0x3e))+8u);
    n.x=x;n.y=y;map=&r->close;
lookup:
    if(ntm_terrain_map_word(map,x,y,&tile)<0 || !r->masks || !r->shapes ||
       r->masks_size<8 || (size_t)(tile>>8)*8+(y&7u)>=r->shapes_size)return -1;
    code=tile&255u;mask=r->masks[x&7u]&r->shapes[(tile>>8)*8+(y&7u)];n.detail=0;
    if(code<208){n.terrain=(uint8_t)((mask?code>>4:code)&15u);}
    else {n.terrain=3;if(mask){code-=208;n.detail=(uint16_t)(((code<<4)|(code>>4))+1u);n.detail=(uint16_t)((uint8_t)n.detail);}}
    *result=n;return 0;
}
