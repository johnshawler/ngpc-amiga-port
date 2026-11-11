/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_application.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static ntm_course_application app;
static uint8_t image[]={1,0,0,0,0,0,0,0,129,0,0,255,255},pixels[16],offsets[2],colors[8],descriptor[5]={12,0,17,0,7};
static uint8_t fade[256],randoms[256],cursor;
int main(void)
{
    ntm_screen_asset assets[5];ntm_palette_job jobs[3]={{0}};ntm_course_resources resources;
    unsigned i,mode,count;uint8_t row[160];nr_scene scene;
    for(i=0;i<16;i++)pixels[i]=255;
    for(i=0;i<5;i++){assets[i].data=image;assets[i].size=sizeof image;assets[i].tiles=pixels;assets[i].tiles_size=16;}
    for(i=0;i<256;i++){fade[i]=0;randoms[i]=(uint8_t)(i*29);}
    for(i=0;i<7;i++)fade[129+2*i]=160;
    jobs[0].kind=1;jobs[0].count=1;jobs[0].offsets=offsets;jobs[0].offsets_size=2;jobs[0].colors=colors;jobs[0].colors_size=8;
    jobs[1].kind=jobs[2].kind=2;jobs[1].offset=480;jobs[1].control_offset=0x118;jobs[1].color=0x321;
    jobs[2].offset=510;jobs[2].control_offset=0x12;jobs[2].pending_index=1;
    resources.assets=assets;resources.palette_jobs=jobs;resources.cursor_descriptor=descriptor;
    resources.fade_in=resources.fade_out=fade;resources.random=randoms;resources.random_cursor=&cursor;
    for(mode=0;mode<3;mode++) {
        memset(&app,0,sizeof app);memset(app.screen.fill_status,255,8);
        for(i=0;i<8;i++)app.palette.slots[i]=(uint16_t)(0x8000+4*i);
        app.palette.pending[0]=app.palette.pending[1]=255;app.palette.background=128;
        app.driver.parent.viewport[3]=152;app.screen.displayed_selection=1;cursor=3;
        assert(!ntm_course_application_start(&app,&resources));assert(app.palette.copy_bytes==8 && app.screen.stamp_bytes==24);
        for(i=0;i<8;i++)assert(!ntm_course_application_update(&app,0));
        assert(app.driver.context.selection==1 && app.driver.nodes[1].next==app.driver.nodes+3);
        assert(app.palette.palette[480]==33 && app.palette.palette[481]==3 && app.palette.background==128);
        assert(!ntm_course_application_scene(&app,&scene) && scene.window_w==160 && scene.window_h==152);
        assert(!nr_scene_row(&scene,96,row));for(i=0;i<160;i++)assert(row[i]==(i>=56 && i<64?67:240));
        assert(!ntm_course_application_update(&app,(uint8_t)(mode==0?16:mode==1?32:80)));
        count=0;while(app.driver.running){int r=ntm_course_application_update(&app,0);assert(r==0 || r==1);assert(++count<120);}
        assert(app.driver.nodes[4].result==(mode==1?252:0));
        if(mode!=1){assert(app.palette.window_control==7 && app.palette.palette[510]==0 && app.palette.palette[511]==0);}
        assert(!app.palette.copy_bytes);
    }
    puts("PASS native course application:3 complete initialized interactive input flows with shared palette pool");return 0;
}
