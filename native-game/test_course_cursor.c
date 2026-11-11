/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_cursor.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t normal[]={3,0,8,0,3,128,12,0,0,128,0,0,8,128,8,8};
static uint8_t accept[]={1,0,8,0,1,128,12,0,0,128,0,0,8,128,8,8};
static uint8_t shapes[]={128,0,0,0,0,0,1,0,128,0,0,0,0,0,2,0},source[48];
static uint8_t tiles[8192],sprites[256],palettes[64];
static ntm_course_cursor_engine e;static ntm_display_reset_state d;static ntm_course_cursor c;
static ntm_cursor_assets a={{{normal,sizeof normal,2},{accept,sizeof accept,2}},shapes,source,sizeof shapes,sizeof source};
static uint16_t scroll[2];static uint8_t flags;
static void start(void)
{
    memset(&e,0,sizeof e);memset(&c,0,sizeof c);memset(sprites,7,sizeof sprites);memset(palettes,7,sizeof palettes);memset(tiles,7,sizeof tiles);
    assert(!ntm_display_reset(&d));d.zero_flags[8]=1;d.zero_words[0]=2;d.list[0]=0xff00;d.object_tags[0]=46;
    c.tag=46;c.width=16;c.second=255;c.y=28;c.byte15=3;c.flags=128;c.tile=450;c.link=65535;flags=16;
}
int main(void)
{
    unsigned i,x;for(i=0;i<48;i++)source[i]=(uint8_t)(i*13);
    start();assert(!ntm_course_cursor_tail(&e,&c,&d,8,scroll,&flags,&a));
    assert(e.frame==0 && e.delay==3 && !c.flags && d.zero_flags[10]==1 && d.zero_words[1]==6 && sprites[0]==7);
    assert(!ntm_course_cursor_head(&e,&d,tiles,sprites,palettes,&a));
    assert(sprites[0]==194 && sprites[1]==25 && sprites[2]==8 && sprites[3]==28);
    for(i=0;i<16;i++)assert(tiles[7200+i]==source[16+i]);
    for(i=0;i<3;i++){assert(!ntm_course_cursor_tail(&e,&c,&d,8,scroll,&flags,&a));assert(!ntm_course_cursor_head(&e,&d,tiles,sprites,palettes,&a));}
    assert(e.frame==4 && e.delay==3 && c.byte30==128 && sprites[2]==16 && sprites[3]==36);
    c.velocity=4;c.flags|=128;assert(!ntm_course_cursor_tail(&e,&c,&d,56,scroll,&flags,&a));assert(e.frame==0 && e.delay==1 && c.byte30==0);
    assert(!ntm_course_cursor_head(&e,&d,tiles,sprites,palettes,&a) && sprites[2]==56);
    c.flags|=16;assert(!ntm_course_cursor_tail(&e,&c,&d,56,scroll,&flags,&a));assert(!d.zero_flags[8] && c.tag==65535 && sprites[2]==56);
    assert(!ntm_course_cursor_head(&e,&d,tiles,sprites,palettes,&a));for(i=0;i<256;i++)assert(!sprites[i]);
    start();c.flags=192;assert(!ntm_course_cursor_tail(&e,&c,&d,0,scroll,&flags,&a));assert(e.delay==4 && c.flags==66);
    assert(!ntm_course_cursor_head(&e,&d,tiles,sprites,palettes,&a));
    for(x=0;x<65536;x++) {
        unsigned visible=(uint16_t)(x+7)<=166;c.flags=64;
        assert(!ntm_course_cursor_tail(&e,&c,&d,(uint16_t)x,scroll,&flags,&a));assert(d.zero_flags[13]==visible);
        assert(!ntm_course_cursor_head(&e,&d,tiles,sprites,palettes,&a));
        assert(sprites[0]==(visible?194:0) && sprites[2]==(visible?(uint8_t)x:0));
    }
    for(x=0;x<65536;x++) {
        unsigned visible=(uint16_t)(x+7)<=158;c.flags=64;c.y=(uint16_t)x;
        assert(!ntm_course_cursor_tail(&e,&c,&d,8,scroll,&flags,&a));assert(d.zero_flags[13]==visible);
        assert(!ntm_course_cursor_head(&e,&d,tiles,sprites,palettes,&a));
        assert(sprites[0]==(visible?194:0) && sprites[3]==(visible?(uint8_t)x:0));
    }
    c.y=28;c.flags=32;assert(!ntm_course_cursor_tail(&e,&c,&d,8,scroll,&flags,&a) && !d.zero_flags[13] && (c.flags&1));
    c.byte30=1;assert(ntm_course_cursor_tail(&e,&c,&d,8,scroll,&flags,&a)==-1);c.byte30=0;
    d.zero_flags[1]|=8;assert(ntm_course_cursor_head(&e,&d,tiles,sprites,palettes,&a)==-1);
    puts("PASS native course cursor:65536 horizontal + 65536 vertical clipping positions, deferred upload, animation, restart, pause, hide and removal");return 0;
}
