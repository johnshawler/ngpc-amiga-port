/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "tee_objects.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Authored format fixtures test contracts; real art is independently oracled. */
static const uint8_t animation[]={2,192,4,0,0,128,0,0};
static const uint8_t shape[]={128,0,0,0,0,0,0,0};
static const uint8_t source[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
static ntm_tee_object_assets assets;
static ntm_tee_object_engine engine;
static ntm_display_reset_state display;
static ntm_course_cursor objects[32];
static const ntm_tee_object_assets *refs[32];
static uint8_t tiles[8192],sprites[256],palettes[64];
static uint16_t x[32],scroll[2];
int main(void)
{
    unsigned selection,width;uint8_t global=16;
    assets.shapes=shape;assets.shapes_size=sizeof shape;
    assets.tiles=source;assets.tiles_size=sizeof source;
    assets.animations[4].data=animation;assets.animations[4].size=sizeof animation;assets.animations[4].frames=1;
    assets.animations[5]=assets.animations[4];refs[18]=&assets;
    for(selection=0;selection<65536;selection++) {
        ntm_course_cursor *o=objects+18;
        assert(!ntm_tee_objects_reset(&display,objects));
        o->tag=52;o->width=60;o->velocity=(uint16_t)selection;o->tile=325;o->y=96;o->byte15=2;o->flags=128;x[18]=40;
        assert(ntm_tee_object_register(&display,o)==((selection==16 || selection==20)?0:-1));
        if(selection==16 || selection==20) {
            assert(!ntm_tee_objects_tail(&engine,objects,&display,x,scroll,&global,refs));
            assert(!ntm_tee_objects_head(&engine,&display,tiles,sprites,palettes));
            assert(!memcmp(tiles+16*325,source,16));assert(sprites[2]==40 && sprites[3]==96);
        }
    }
    for(width=0;width<65536;width++) {
        assert(!ntm_tee_objects_reset(&display,objects));
        objects[18].tag=52;objects[18].width=(uint16_t)width;objects[18].velocity=16;
        assert(ntm_tee_object_register(&display,objects+18)==(width==60?0:-1));
    }
    puts("tag52: 65536 selection words, 65536 indices, both genuine supported selections, borrowed tile/head contract");return 0;
}
