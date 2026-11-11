/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_initialization.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t maps[4096],tiles[8192],sprites[256],sprite_palettes[64];
static uint8_t image[]={1,0,0,0,0,0,0,0,129,0,0,255,255},pixels[16];
static uint8_t offsets[]={8,0,0,0},colors[16],descriptor[5]={16,0,17,0,3};
static ntm_course_palette palette;
int main(void)
{
    unsigned seed,i;ntm_screen_asset assets[5];ntm_palette_job jobs[2]={{0}};
    for(i=0;i<5;i++){assets[i].data=image;assets[i].size=sizeof image;assets[i].tiles=pixels;assets[i].tiles_size=16;}
    jobs[0].kind=1;jobs[0].count=2;jobs[0].offsets=offsets;jobs[0].offsets_size=4;jobs[0].colors=colors;jobs[0].colors_size=16;
    jobs[1].kind=2;jobs[1].offset=480;jobs[1].color=0xabc;jobs[1].control_offset=0x118;jobs[1].pending_index=0;
    for(i=0;i<16;i++)colors[i]=(uint8_t)(i*13);
    for(seed=0;seed<256;seed++) {
        ntm_course_initialization s={0};ntm_course_screen screen={0};ntm_display_reset_state display;
        memset(&palette,0,sizeof palette);memset(&display,77,sizeof display);memset(sprites,99,sizeof sprites);memset(sprite_palettes,99,sizeof sprite_palettes);
        memset(palette.staging,19,512);memset(palette.palette,71,512);palette.pending[0]=palette.pending[1]=255;palette.background=(uint8_t)seed;
        for(i=0;i<8;i++)palette.slots[i]=(uint16_t)(0x8000+4*((i+seed)%8));
        s.flags=(uint8_t)seed;screen.displayed_selection=(uint8_t)(seed%3);memset(screen.fill_status,255,8);
        assert(!ntm_course_initialize(&s,&screen,&display,&palette,sprites,sprite_palettes,assets,descriptor,jobs));
        assert(s.flags==((seed&39)|16) && !s.plane_swap && s.selection==seed%3 && screen.cached_selection==65535);
        assert(screen.stamp_bytes==24 && screen.cursor_x==8+48*(seed%3));
        assert(s.cursor.tag==46 && s.cursor.width==16 && !s.cursor.velocity && s.cursor.y==28 && s.cursor.tile==450 && s.cursor.flags==128 && s.cursor.link==65535);
        assert(display.list[0]==0xff00 && display.zero_words[0]==2 && display.zero_flags[8]==1 && display.zero_flags[9]==1);
        for(i=0;i<256;i++)assert(!sprites[i]);
        for(i=0;i<64;i++)assert(!sprite_palettes[i]);
        assert(!palette.copy_bytes);for(i=0;i<512;i++)assert(palette.palette[i]==71 && palette.staging[i]==19);
        assert(!ntm_course_palette_tail(&palette));assert(palette.copy_bytes==8 && palette.background==seed && palette.pending[0]==(seed&248));
        assert(!ntm_course_screen_fill_queue(&screen,maps));assert(!ntm_course_screen_stamp_queue(&screen,tiles,maps));
        assert(!ntm_course_palette_head(&palette));assert(!palette.copy_bytes && palette.background==(seed&248) && palette.pending[0]==255);
        for(i=0;i<16;i++)assert(palette.palette[i]==colors[(i+8)%16]);
        assert(palette.palette[480]==188 && palette.palette[481]==10);
        palette.pending[0]=7;assert(!ntm_course_palette_head(&palette) && palette.pending[0]==7); /* Empty head preserves pending controls. */
    }
    puts("PASS native course initialization:256 palette, background, reset and cursor setup cases");return 0;
}
