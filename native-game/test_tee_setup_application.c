/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "tee_setup_application.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static ntm_tee_setup_application application;
#define app (application.base)
static ntm_tee_setup_resources tee_resources;
static ntm_tee_object_assets tee_objects;
static ntm_hole_geometry geometry[54];
static ntm_hole_prepare_resources holes;
static uint8_t pins[64],quarter[4100],variation[22],threshold[8],wind[16],limit[18];
static uint8_t hazards[2]={255,255};
static const uint8_t tee_metadata[4][5]={{52,0,12,0,3},{52,0,0,0,3},{52,0,4,0,3},{52,0,16,0,3}};
static uint8_t image[]={1,0,0,0,0,0,0,0,129,0,0,255,255},pixels[16],offsets[2],colors[8];
static uint8_t cursor_descriptor[5]={16,0,0,0,3};
static uint8_t descriptor[3][12]={{20,0,1,0,3,0,4,0,20,0,5,0},{24,0,1,0,3,0,4,0,24,0,5,0},{28,0,1,0,3,0,4,0,28,0,5,0}};
static uint8_t metadata[5]={20,0,0,0,3};
static uint8_t cursor_anim[8]={1,128,4,0,0,128,0,0},title_anim[8]={1,192,4,0,0,128,0,0};
static uint8_t shape[8]={128,0,0,0,0,0,0,0},fade[256],randoms[256],random_cursor;
static uint8_t round_meta[5]={32,0,0,0,3},empty_score[1]={255};
int main(void)
{
    ntm_round_record_resources round={0};ntm_screen_asset assets[5];ntm_palette_job jobs[3]={{0}};ntm_course_resources resources={0};
    ntm_course_intro_resources courses[3]={{0}};ntm_course_title_resources titles[3]={{0}};
    ntm_cursor_assets objects[3]={0},cursor={0};unsigned i,mode,selection,flows=0,unsupported_flows=0;
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
    round.palettes[0]=jobs[0];round.palettes[1]=jobs[1];round.font=round.header=round.footer=round.labels[0]=round.labels[1]=assets[0];
    for(i=0;i<3;i++)round.course[i][0]=round.course[i][1]=assets[0];
    for(i=0;i<6;i++)round.golfer[i][0]=round.golfer[i][1]=assets[0];
    round.missing_score=empty_score;round.missing_score_size=1;round.metadata=round_meta;round.metadata_size=5;round.object=objects;
    tee_resources.font=tee_resources.header=assets[0];tee_resources.palettes[0]=jobs[0];tee_resources.palettes[1]=jobs[1];
    for(i=0;i<18;i++)tee_resources.hole_numbers[i]=assets[0];
    for(i=0;i<3;i++)tee_resources.pars[i]=assets[0];
    for(i=0;i<54;i++) {
        unsigned j;ntm_tee_hole_art *art=&tee_resources.holes[i/18][i%18];
        for(j=0;j<3;j++)art->views[j]=assets[0];
        art->marker_x=60;art->marker_y=40;
        geometry[i].descriptor[1]=4;geometry[i].descriptor[2]=(uint8_t)(100+i);geometry[i].dimensions[0]=12;geometry[i].dimensions[1]=20;
        geometry[i].pins=pins;geometry[i].pins_size=64;
    }
    for(i=0;i<4;i++){tee_resources.metadata[i]=tee_metadata[i];tee_resources.metadata_size[i]=5;}
    for(i=0;i<8;i++){tee_objects.animations[i]=cursor.animations[0];tee_objects.animations[i].data=title_anim;}
    tee_objects.shapes=shape;tee_objects.shapes_size=8;tee_objects.tiles=pixels;tee_objects.tiles_size=16;tee_resources.objects=&tee_objects;
    memset(limit,15,sizeof limit);holes.holes=geometry;holes.hole_count=54;holes.hazards=hazards;holes.hazards_size=2;
    holes.quarter=quarter;holes.quarter_size=4100;holes.variation=variation;holes.variation_size=22;holes.threshold=threshold;holes.threshold_size=8;
    holes.wind_low=holes.wind_high=wind;holes.wind_low_size=holes.wind_high_size=16;holes.wind_limit=limit;holes.wind_limit_size=18;
    for(mode=0;mode<11;mode++)for(selection=0;selection<3;selection++) {
        ntm_course_application *s=&app.selector;ntm_course_driver *d=&s->driver;unsigned count=0,seen_title=0,seen_tournament=0,skipped=0;
        unsigned preparation_expected=0;uint8_t prepare_cursor=0;
        memset(&application,0,sizeof application);memset(s->screen.fill_status,255,8);
        for(i=0;i<8;i++)s->palette.slots[i]=(uint16_t)(0x8000+4*i);
        s->palette.pending[0]=s->palette.pending[1]=255;s->palette.background=128;
        d->parent.viewport[3]=152;s->screen.displayed_selection=(uint16_t)selection;random_cursor=3;
        assert(!ntm_tee_setup_application_start(&application,&resources,courses,titles,objects,&round,&tee_resources,&holes));
        if(mode==8)application.owner.skip_overview=1;
        if(mode==9)memset(application.saved_hole_scores,1,sizeof application.saved_hole_scores);
        if(mode==10)memset(application.saved_hole_scores,255,sizeof application.saved_hole_scores);
        if(mode==6)app.title_context.game_mode=2;
        if(mode==7)app.title_context.input_mode=1;
        for(i=0;i<8;i++)assert(!ntm_tee_setup_application_update(&application,0));
        assert(!ntm_tee_setup_application_update(&application,(uint8_t)(mode==1?32:mode==2?80:16)));
        while(d->running){
            uint8_t buttons=0;int result;
            if(app.title.phase==NTM_TITLE_WAIT && d->updates>=app.title_start_update+3 && (mode==3 || mode==4 || mode==5))buttons=16;
            if(app.title.phase==NTM_TITLE_TOURNAMENT_WAIT && d->updates>=app.tournament_start_update+3 && mode==4)buttons=16;
            if(mode==5 && app.title_start_update && d->updates>=app.title_start_update+3)buttons=16; /* A hold is one edge, never repeated skip. */
            if(mode==7 && app.title.phase==NTM_TITLE_WAIT && d->updates>=app.title_start_update+3)app.title_context.linked_controls[1]=16;
            else app.title_context.linked_controls[1]=0;
            if(mode==4 && app.round.phase==NTM_ROUND_WAIT && d->updates>=app.round_open_update+3)buttons=16;
            if(mode==4 && application.overview.phase==NTM_TEE_HOLD && d->updates>=application.hold_update+3)buttons=16;
            if(buttons)skipped=1;
            if(mode==10 && app.round.phase==NTM_ROUND_DONE && !application.started) {
                assert(!preparation_expected && !application.hole.ready_flag && !application.hole.selected_difficulty);
                assert(!application.context.par);prepare_cursor=random_cursor;preparation_expected=1;
            }
            result=ntm_tee_setup_application_update(&application,buttons);assert(result==0 || result==1);assert(++count<1100);
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
            assert(seen_title && seen_tournament);
            if(mode==6 || mode==7)assert(app.boundary==NTM_INTRO_BOUNDARY_ROUND_UNSUPPORTED && app.round.phase==0);
            else {
                assert(app.round.phase==NTM_ROUND_DONE);
                assert(application.start_update==app.round_complete_update+1);
                if(mode==10) {
                    /* The saved-score byte is not a missing-course-record flag.
                     * Its wrapped badge offset is rejected only after real hole
                     * preparation, without registering an invalid animation. */
                    assert(preparation_expected && application.boundary==NTM_TEE_BOUNDARY_UNSUPPORTED);
                    assert(application.started && application.owner.phase==1 && application.overview.phase==NTM_TEE_NEW);
                    assert(application.overview_update==application.start_update && d->updates==application.start_update);
                    assert(!application.open_update && !application.hold_update && !application.complete_update);
                    assert(application.hole.ready_flag==255 && application.hole.selected_difficulty==2);
                    assert(application.context.par==4 && application.hole.hole[0x15]==4);
                    assert(application.context.distance==100+18*application.context.course+application.context.round_index);
                    assert(random_cursor==(uint8_t)(prepare_cursor+6));
                    assert(ntm_tee_badge_offset(255,application.context.par)==252);
                    assert(!s->display.zero_flags[8] && !s->display.zero_flags[10]);
                    for(i=0;i<32;i++)assert(application.objects[i].tag==65535 && s->display.object_tags[i]==65535);
                }else {
                    assert(application.boundary==NTM_TEE_BOUNDARY_SHOT_SCENE);
                    if(mode==8)assert(application.overview.phase==0 && d->updates==application.start_update);
                    else {
                        assert(application.overview.phase==NTM_TEE_DONE);
                        assert(application.overview_update==application.start_update && application.open_update==application.overview_update+3);
                        assert(application.hold_update==application.open_update+4 && d->updates==application.complete_update+1);
                    }
                }
                assert(app.round_start_update==app.title_complete_update+1 && app.round_open_update==app.round_start_update+2);
                assert(app.round_drawing_update==app.round_start_update);
                assert(app.round_complete_update-app.round_open_update==(mode==4?4:240));
            }
            assert(app.title_start_update==app.first_visible_update+20 && app.first_visible_update==app.intro_start_update+2);
            assert(app.records.count[0]==1);
            assert(!s->display.zero_flags[8] && !memcmp(s->sprites,(uint8_t[256]){0},256));
            assert(app.tournament_start_update-app.title_start_update==((mode==3 || mode==4 || mode==5 || mode==7)?6:62));
            assert(app.title_complete_update-app.tournament_start_update==(mode==4?4:60));
            if(mode>=3 && mode<=5)assert(skipped);
        }
        assert(ntm_tee_setup_application_update(&application,0)==-1);
        if(mode==10)unsupported_flows++;else flows++;
    }
    printf("PASS tee application:%u real traversals,3 mappings,normal/random/cancel/title skip/double skip/held input/mode2/linked skip,round allocation,preparation,draw,blink,skip,real tee overview,nested children,hole preparation,multiobjects,next child boundary; %u malformed saved-score boundaries after preparation\n",flows,unsupported_flows);return 0;
}
