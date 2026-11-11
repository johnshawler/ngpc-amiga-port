/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_intro_application.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static ntm_course_intro_application app;
static uint8_t image[]={1,0,0,0,0,0,0,0,129,0,0,255,255},pixels[16],offsets[2],colors[8];
static uint8_t cursor_descriptor[5]={12,0,17,0,7},course_descriptor[4]={20,0,1,0};
static uint8_t fade[256],randoms[256],random_cursor;
int main(void)
{
    ntm_screen_asset assets[5];ntm_palette_job jobs[3]={{0}};ntm_course_resources resources={0};
    ntm_course_intro_resources courses[3]={{0}};
    unsigned i,mode,selection,count,finished;
    memset(pixels,255,sizeof pixels);
    for(i=0;i<5;i++){assets[i].data=image;assets[i].size=sizeof image;assets[i].tiles=pixels;assets[i].tiles_size=16;}
    for(i=0;i<256;i++)randoms[i]=(uint8_t)(i*29);
    for(i=0;i<7;i++)fade[129+2*i]=160;
    jobs[0].kind=1;jobs[0].count=1;jobs[0].offsets=offsets;jobs[0].offsets_size=2;jobs[0].colors=colors;jobs[0].colors_size=8;
    jobs[1].kind=jobs[2].kind=2;jobs[1].offset=480;jobs[1].control_offset=0x118;jobs[1].color=0x321;
    jobs[2].offset=510;jobs[2].control_offset=0x12;jobs[2].pending_index=1;
    resources.assets=assets;resources.palette_jobs=jobs;resources.cursor_descriptor=cursor_descriptor;
    resources.fade_in=resources.fade_out=fade;resources.random=randoms;resources.random_cursor=&random_cursor;
    for(i=0;i<3;i++){courses[i].descriptor=course_descriptor;courses[i].descriptor_size=4;courses[i].background=assets[0];courses[i].palettes[0]=jobs[0];courses[i].palettes[1]=jobs[1];}
    for(mode=0;mode<4;mode++)for(selection=0;selection<3;selection++) {
        ntm_course_application *s=&app.selector;ntm_course_driver *d=&s->driver;
        memset(&app,0,sizeof app);memset(s->screen.fill_status,255,8);
        for(i=0;i<8;i++)s->palette.slots[i]=(uint16_t)(0x8000+4*i);
        s->palette.pending[0]=s->palette.pending[1]=255;s->palette.background=128;
        d->parent.viewport[3]=152;s->screen.displayed_selection=(uint16_t)selection;random_cursor=3;
        app.records.active_player=1;app.records.golfer[1]=5;app.records.count[5]=65535;
        if(mode==3)app.records.record_kind[1]=6;
        assert(!ntm_course_intro_application_start(&app,&resources,courses));
        assert(d->nodes[0].next==d->nodes+4 && d->nodes[4].next==d->nodes+1);
        assert(d->nodes[1].priority==2 && d->nodes[4].result==249);
        for(i=0;i<8;i++)assert(!ntm_course_intro_application_update(&app,0));
        assert(!ntm_course_intro_application_update(&app,(uint8_t)(mode==1?32:mode==2?80:16)));
        count=finished=0;
        while(d->running){
            int r=ntm_course_intro_application_update(&app,0);assert(r==0 || r==1);assert(++count<200);
            if(d->parent.phase==NTM_PARENT_DONE && !finished){
                finished=1;assert(app.owner.phase==NTM_OWNER_SELECTOR_WAIT && app.intro.phase==NTM_INTRO_NEW);
                assert(app.records.count[5]==65535);assert(d->running); /* Exit never reruns owner. */
            }
            if(app.intro_start_update==d->updates && app.intro_start_update){
                assert(d->updates==app.selector_exit_update+1);assert(app.records.count[5]==0 && app.records.active[5]==255);
                assert(app.intro.phase==NTM_INTRO_SCENERY_DELAY && d->nodes[4].next->delay==1);
                assert(s->screen.stamp_bytes==8 && s->palette.copy_bytes==8); /* Queued until next head. */
            }
        }
        assert(finished && d->nodes[4].result==(mode==1?252:0));
        if(mode==1 || mode==3){
            assert(app.boundary==(mode==1?NTM_INTRO_BOUNDARY_PRIOR_SELECTION:NTM_INTRO_BOUNDARY_SPECIAL_RECORD));
            assert(!app.intro_start_update && app.records.count[5]==65535);
        }else{
            static const uint8_t mapped[3]={2,0,1};
            if(mode==0)assert(d->parent.mapped_course==mapped[selection]);
            assert(app.boundary==NTM_INTRO_BOUNDARY_TITLE && app.intro.phase==NTM_INTRO_TITLE_PENDING);
            assert(app.first_visible_update==app.intro_start_update+2);
            assert(d->updates==app.first_visible_update+20 && app.records.count[5]==0);
            assert(d->parent.viewport[0]==0 && d->parent.viewport[2]==160 && d->parent.viewport[3]==152);
            assert(!s->screen.stamp_bytes && !s->palette.copy_bytes);
            assert(s->maps[2048]==1 && s->maps[2049]==0);
        }
        assert(ntm_course_intro_application_update(&app,0)==-1);
    }
    puts("PASS intro application:12 real selector/owner traversals,3 course mappings,normal/random/cancel/special boundaries,queue ordering,no duplicate records");return 0;
}
