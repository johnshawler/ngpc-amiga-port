/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_intro_application.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static ntm_course_intro_application app;
static uint8_t image[]={1,0,0,0,0,0,0,0,129,0,0,255,255},pixels[16],offsets[2],colors[8];
static uint8_t cursor_descriptor[5]={16,0,0,0,3};
static uint8_t descriptor[3][12]={{20,0,1,0,3,0,4,0,20,0,5,0},{24,0,1,0,3,0,4,0,24,0,5,0},{28,0,1,0,3,0,4,0,28,0,5,0}};
static uint8_t metadata[5]={20,0,0,0,3};
static uint8_t cursor_anim[8]={1,128,4,0,0,128,0,0},title_anim[8]={1,192,4,0,0,128,0,0};
static uint8_t shape[8]={128,0,0,0,0,0,0,0},fade[256],randoms[256],random_cursor;
int main(void)
{
    ntm_screen_asset assets[5];ntm_palette_job jobs[3]={{0}};ntm_course_resources resources={0};
    ntm_course_intro_resources courses[3]={{0}};ntm_course_title_resources titles[3]={{0}};
    ntm_cursor_assets objects[3]={0},cursor={0};unsigned i,mode,selection,flows=0;
    memset(pixels,255,sizeof pixels);
    for(i=0;i<5;i++){assets[i].data=image;assets[i].size=sizeof image;assets[i].tiles=pixels;assets[i].tiles_size=16;}
    for(i=0;i<256;i++)randoms[i]=(uint8_t)(i*29);
    for(i=0;i<7;i++)fade[129+2*i]=160;
    jobs[0].kind=1;jobs[0].count=1;jobs[0].offsets=offsets;jobs[0].offsets_size=2;jobs[0].colors=colors;jobs[0].colors_size=8;
    jobs[1].kind=jobs[2].kind=2;jobs[1].offset=480;jobs[1].control_offset=0x118;jobs[1].color=0x321;
    jobs[2].offset=510;jobs[2].control_offset=0x12;jobs[2].pending_index=1;
    cursor.animations[0].data=cursor.animations[1].data=cursor_anim;
    cursor.animations[0].size=cursor.animations[1].size=8;cursor.animations[0].frames=cursor.animations[1].frames=1;
    cursor.shapes=shape;cursor.shapes_size=8;cursor.tiles=pixels;cursor.tiles_size=16;
    resources.assets=assets;resources.palette_jobs=jobs;resources.cursor_descriptor=cursor_descriptor;
    resources.fade_in=resources.fade_out=fade;resources.random=randoms;resources.random_cursor=&random_cursor;resources.cursor=&cursor;
    for(i=0;i<3;i++){
        courses[i].descriptor=descriptor[i];courses[i].descriptor_size=12;courses[i].background=assets[0];courses[i].palettes[0]=jobs[0];courses[i].palettes[1]=jobs[1];
        titles[i].descriptor=descriptor[i];titles[i].descriptor_size=12;titles[i].metadata=metadata;titles[i].metadata_size=5;
        titles[i].title=titles[i].tournament[0]=titles[i].tournament[1]=assets[0];
        objects[i]=cursor;objects[i].animations[0].data=title_anim;
    }
    for(mode=0;mode<8;mode++)for(selection=0;selection<3;selection++) {
        ntm_course_application *s=&app.selector;ntm_course_driver *d=&s->driver;unsigned count=0,seen_title=0,seen_tournament=0,skipped=0;
        memset(&app,0,sizeof app);memset(s->screen.fill_status,255,8);
        for(i=0;i<8;i++)s->palette.slots[i]=(uint16_t)(0x8000+4*i);
        s->palette.pending[0]=s->palette.pending[1]=255;s->palette.background=128;
        d->parent.viewport[3]=152;s->screen.displayed_selection=(uint16_t)selection;random_cursor=3;
        assert(!ntm_course_intro_application_start_titles(&app,&resources,courses,titles,objects));
        if(mode==6)app.title_context.game_mode=2;
        if(mode==7)app.title_context.input_mode=1;
        for(i=0;i<8;i++)assert(!ntm_course_intro_application_update(&app,0));
        assert(!ntm_course_intro_application_update(&app,(uint8_t)(mode==1?32:mode==2?80:16)));
        while(d->running){
            uint8_t buttons=0;int result;
            if(app.title.phase==NTM_TITLE_WAIT && d->updates>=app.title_start_update+3 && (mode==3 || mode==4 || mode==5))buttons=16;
            if(app.title.phase==NTM_TITLE_TOURNAMENT_WAIT && d->updates>=app.tournament_start_update+3 && mode==4)buttons=16;
            if(mode==5 && app.title_start_update && d->updates>=app.title_start_update+3)buttons=16; /* A hold is one edge, never repeated skip. */
            if(mode==7 && app.title.phase==NTM_TITLE_WAIT && d->updates>=app.title_start_update+3)app.title_context.linked_controls[1]=16;
            else app.title_context.linked_controls[1]=0;
            if(buttons)skipped=1;
            result=ntm_course_intro_application_update(&app,buttons);assert(result==0 || result==1);assert(++count<350);
            if(app.title.phase==NTM_TITLE_WAIT){
                seen_title=1;assert(app.title_cursor.tag==38 && app.title_cursor.velocity==0);
                assert(app.title_cursor.width==20+4*d->parent.mapped_course && app.title_x==40 && app.title_cursor.y==48);
                assert(app.title_cursor.tile==4 && app.records.count[0]==1);
                if(d->updates>app.title_start_update+1){assert(app.title_cursor.flags==66 && app.title_cursor.byte30==64);assert(s->sprites[0]==4 && s->sprites[2]==40 && s->sprites[3]==48);}
            }
            if(app.title.phase==NTM_TITLE_TOURNAMENT_WAIT){
                seen_tournament=1;assert(!s->display.zero_flags[8]);assert(!memcmp(s->sprites,(uint8_t[256]){0},256));
                assert(s->maps[0x180]==5);assert(d->parent.viewport[2]==160);
            }
            if(app.title_complete_update==d->updates && app.title_complete_update){
                assert(app.owner.phase==NTM_OWNER_INTRO_WAIT && d->running); /* Owner resumes next traversal. */
                assert(d->parent.viewport[2]==0 && app.title.phase==NTM_TITLE_DONE);
            }
        }
        if(mode==1){assert(app.boundary==NTM_INTRO_BOUNDARY_PRIOR_SELECTION && !seen_title && !app.records.count[0]);}
        else{
            assert(seen_title && seen_tournament && app.boundary==NTM_INTRO_BOUNDARY_ROUND_RECORD);
            assert(app.title_start_update==app.first_visible_update+20 && app.first_visible_update==app.intro_start_update+2);
            assert(d->updates==app.title_complete_update+1 && app.records.count[0]==1);
            assert(!s->display.zero_flags[8] && !memcmp(s->sprites,(uint8_t[256]){0},256));
            assert(app.tournament_start_update-app.title_start_update==((mode==3 || mode==4 || mode==5 || mode==7)?6:62));
            assert(app.title_complete_update-app.tournament_start_update==(mode==4?4:60));
            if(mode>=3 && mode<=5)assert(skipped);
        }
        assert(ntm_course_intro_application_update(&app,0)==-1);flows++;
    }
    printf("PASS title application:%u real traversals,3 mappings,normal/random/cancel/title skip/double skip/held input/mode2/linked skip,owner boundary,sprite cleanup\n",flows);return 0;
}
