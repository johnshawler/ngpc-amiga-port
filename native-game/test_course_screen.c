/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_screen.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t data[3][16];
int main(void)
{
    ntm_screen_asset assets[3]={{0}};unsigned course,mask,count,i,cache;unsigned long cases=0;
    for(i=0;i<3;i++){assets[i].data=data[i];assets[i].size=sizeof data[i];}
    for(course=0;course<3;course++)for(mask=0;mask<256;mask++)for(count=0;count<=32;count++) {
        ntm_course_screen a={0},b;int slot=-1;
        for(i=0;i<8;i++){a.fill_status[i]=(mask&(1u<<i))?255:7;if(a.fill_status[i]==255)slot=(int)i;}
        a.stamp_bytes=(uint16_t)(count*8);a.cached_selection=0x1234;b=a;
        b.displayed_selection=(uint8_t)course;b.cursor_x=(uint16_t)(8+48*course);
        if(slot>=0){b.fill_status[slot]=0;b.fills[slot].offset=768;b.fills[slot].width=20;b.fills[slot].height=6;b.fills[slot].fill=0;}
        if(count<32){b.stamps[count].asset=assets[course];b.stamps[count].offset=(uint16_t)(770+12*course);b.stamps[count].tile=301;b.stamp_bytes+=8;}
        assert(!ntm_course_screen_draw(&a,(uint16_t)course,assets));assert(!memcmp(&a,&b,sizeof a));cases++;
    }
    for(course=0;course<3;course++)for(cache=0;cache<65536;cache++) {
        ntm_course_screen a={0},b;a.cached_selection=(uint16_t)cache;a.stamp_bytes=256;memset(a.fill_status,7,8);b=a;
        if(cache!=course){b.cached_selection=(uint16_t)course;b.displayed_selection=(uint8_t)course;b.cursor_x=(uint16_t)(8+48*course);}
        assert(ntm_course_screen_tick(&a,(uint16_t)course,assets)==(cache!=course));assert(!memcmp(&a,&b,sizeof a));cases++;
    }
    /* Independently authored two-row map and noncontiguous tile offsets. */
    {
        uint8_t asset[]={2,0,0,0,0,0,16,0,0,0,130,0,0,1,0,255,255,255,127,255,255};
        uint8_t pixels[32],tiles[8192],maps[4096],before[4096];
        ntm_screen_stamp stamp={{asset,sizeof asset,pixels,sizeof pixels},770,301};
        for(i=0;i<32;i++)pixels[i]=(uint8_t)(i*7+3);
        memset(tiles,77,sizeof tiles);memset(maps,99,sizeof maps);
        assert(!ntm_course_screen_stamp(&stamp,tiles,maps));
        for(i=0;i<8192;i++)assert(tiles[i]==(i>=4816 && i<4848?pixels[((i-4816)+16)%32]:77));
        for(i=0;i<4096;i++) {
            unsigned expected=99;
            if(i==770)expected=45;
            if(i==771)expected=1;
            if(i==772)expected=46;
            if(i==773)expected=1;
            if(i==834)expected=44;
            if(i==835)expected=129;
            assert(maps[i]==expected);
        }
        memcpy(before,maps,sizeof maps);stamp.asset.size--;
        assert(ntm_course_screen_stamp(&stamp,tiles,maps)==-1 && !memcmp(before,maps,sizeof maps));
        stamp.asset.size++;stamp.offset=4094;
        assert(ntm_course_screen_stamp(&stamp,tiles,maps)==-1 && !memcmp(before,maps,sizeof maps));
        stamp.offset=770;stamp.tile=511;
        assert(ntm_course_screen_stamp(&stamp,tiles,maps)==-1 && !memcmp(before,maps,sizeof maps));
    }
    {
        uint8_t asset[]={1,0,0,0,0,0,0,0,129,0,0,255,255};
        uint8_t pixels[16],tiles[8192],maps[4096];ntm_course_screen screen={0};
        ntm_screen_asset a[3];
        memset(pixels,255,sizeof pixels);memset(tiles,17,sizeof tiles);memset(maps,99,sizeof maps);
        for(i=0;i<3;i++){a[i].data=asset;a[i].size=sizeof asset;a[i].tiles=pixels;a[i].tiles_size=sizeof pixels;}
        memset(screen.fill_status,255,8);screen.cached_selection=65535;
        assert(ntm_course_screen_tick(&screen,2,a)==1 && screen.stamp_bytes==8);
        for(i=0;i<4096;i++)assert(maps[i]==99); /* Producers do not draw. */
        assert(!ntm_course_screen_fill_queue(&screen,maps));
        assert(maps[794]==0 && maps[795]==0 && screen.fill_status[7]==255);
        assert(!ntm_course_screen_stamp_queue(&screen,tiles,maps));
        assert(!screen.stamp_bytes && maps[794]==45 && maps[795]==1);
        for(i=4816;i<4832;i++)assert(tiles[i]==255);
        /* Overlapping fills: lower-numbered slot wins because drain descends. */
        screen.fill_status[0]=7;screen.fill_status[7]=0;
        screen.fills[0].offset=screen.fills[7].offset=0;
        screen.fills[0].width=screen.fills[7].width=1;
        screen.fills[0].height=screen.fills[7].height=1;
        screen.fills[0].fill=0xabcd;screen.fills[7].fill=0x1234;
        assert(!ntm_course_screen_fill_queue(&screen,maps) && maps[0]==205 && maps[1]==171);
    }
    /* Narrow byte-map sentinel rows: exact final word, truncation and malformed
     * second row validate before any tile or map write. */
    {
        uint8_t asset[]={1,0,0,0,0,0,0,0,1,254,255},pixels[16]={0};
        uint8_t tiles[8192],maps[4096],oldtiles[8192],oldmaps[4096];
        ntm_screen_stamp stamp={{asset,sizeof asset,pixels,sizeof pixels},4094,1};
        memset(tiles,77,sizeof tiles);memset(maps,99,sizeof maps);
        assert(ntm_course_screen_stamp(&stamp,tiles,maps)==0 && maps[4094]==255 && maps[4095]==0);
        memcpy(oldtiles,tiles,sizeof tiles);memcpy(oldmaps,maps,sizeof maps);
        for(i=0;i<5;i++) {
            stamp.asset.size=sizeof asset;stamp.offset=4094;asset[8]=1;asset[9]=254;
            if(i==0)stamp.asset.size--; /* Missing row sentinel. */
            if(i==1)asset[9]=255; /* Empty row. */
            if(i==2)asset[8]=2; /* Missing second row. */
            if(i==3)stamp.offset=4095; /* Misaligned destination. */
            if(i==4)asset[8]=0; /* No rows. */
            assert(ntm_course_screen_stamp(&stamp,tiles,maps)==-1);
            assert(!memcmp(tiles,oldtiles,sizeof tiles) && !memcmp(maps,oldmaps,sizeof maps));
        }
    }
    {ntm_course_screen s={0},old=s;assert(ntm_course_screen_draw(&s,3,assets)==-1 && !memcmp(&s,&old,sizeof s));s.stamp_bytes=1;old=s;assert(ntm_course_screen_draw(&s,0,assets)==-1 && !memcmp(&s,&old,sizeof s));}
    printf("PASS native course screen: %lu producer/cache cases\n",cases);return 0;
}
