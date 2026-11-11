/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_application.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static ntm_shot_application application;
static ntm_shot_prepare_resources preparation;
static ntm_terrain_resources terrain;
static ntm_shot_banner_resources banner;
static ntm_tee_object_assets banner_objects;
static ntm_shot_palette_job exit_palettes[2];
static ntm_shot_resources shot;
static uint8_t traits[256],blend[512],masks[8],terrain_shapes[2048];
static const uint8_t banner_metadata[5]={56,0,60,0,3};
#define tee (application.base)
#define app (application.base.base)
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
    preparation.traits=traits;preparation.traits_size=256;
    preparation.wind_low=preparation.wind_step=wind;preparation.wind_low_size=preparation.wind_step_size=16;
    preparation.wind_limit=limit;preparation.wind_limit_size=18;
    terrain.masks=masks;terrain.masks_size=8;terrain.shapes=terrain_shapes;terrain.shapes_size=2048;
    banner.palettes[0]=jobs[0];banner.palettes[1]=jobs[1];
    banner.terrain=banner.tee_shot=banner.portrait=banner.hints[0]=banner.hints[1]=assets[0];
    for(i=0;i<6;i++)banner.characters[i]=assets[0];
    for(i=0;i<2;i++)banner.players[i]=assets[0];
    banner.metadata=banner_metadata;banner.metadata_size=5;
    for(i=0;i<40;i++)banner_objects.animations[i]=tee_objects.animations[0];
    banner_objects.shapes=shape;banner_objects.shapes_size=8;banner_objects.tiles=pixels;banner_objects.tiles_size=16;
    exit_palettes[0].kind=24;exit_palettes[0].speed=16;exit_palettes[0].offset=256;exit_palettes[0].count=1;
    exit_palettes[0].source=exit_palettes[0].target=colors;exit_palettes[0].source_size=exit_palettes[0].target_size=8;
    exit_palettes[0].blend=blend;exit_palettes[0].blend_size=512;
    exit_palettes[1]=exit_palettes[0];exit_palettes[1].kind=40;exit_palettes[1].offset=486;exit_palettes[1].count=0;exit_palettes[1].control_offset=0x118;
    shot.preparation=&preparation;shot.terrain=&terrain;shot.banner=&banner;shot.objects=&banner_objects;shot.exit_palettes=exit_palettes;
    for(mode=0;mode<8;mode++)for(selection=0;selection<3;selection++) {
        ntm_course_application *s=&app.selector;ntm_course_driver *d=&s->driver;
        unsigned count=0,maximum_tasks=0,seen_banner=0,seen_exit=0;uint8_t prepare_cursor=0;
        memset(&application,0,sizeof application);memset(s->screen.fill_status,255,8);
        for(i=0;i<8;i++)s->palette.slots[i]=(uint16_t)(0x8000+4*i);
        s->palette.pending[0]=s->palette.pending[1]=255;s->palette.background=128;
        d->parent.viewport[3]=152;s->screen.displayed_selection=(uint16_t)selection;random_cursor=3;
        app.records.golfer[0]=(uint8_t)((mode+selection)%6);app.records.golfer[1]=(uint8_t)((mode+selection+1)%6);
        assert(!ntm_shot_application_start(&application,&resources,courses,titles,objects,&round,&tee_resources,&holes,&shot));
        if(mode==6)tee.owner.skip_overview=1;
        if(mode==7)application.banner_context.controlling_player=1;
        for(i=0;i<8;i++)assert(!ntm_shot_application_update(&application,0));
        assert(!ntm_shot_application_update(&application,(uint8_t)(mode==1?32:mode==2?80:16)));
        while(d->running) {
            uint8_t buttons=0;int result;ntm_task *node=&d->nodes[0];unsigned active=0;
            if(mode==3 && (app.title.phase==NTM_TITLE_WAIT || app.title.phase==NTM_TITLE_TOURNAMENT_WAIT ||
                app.round.phase==NTM_ROUND_WAIT || tee.overview.phase==NTM_TEE_HOLD || application.banner.phase==NTM_SHOT_BANNER_HOLD))buttons=(d->updates&1)?16:0;
            if(mode==4 && application.banner.phase==NTM_SHOT_BANNER_HOLD)buttons=64;
            if(mode==5 && application.banner.phase)buttons=16;
            if(tee.overview.phase==NTM_TEE_DONE && !application.started)prepare_cursor=random_cursor;
            do {assert(node!=&app.fallback);active++;node=node->next;assert(active<10);}while(node && node!=&d->nodes[0]);
            if(active>maximum_tasks)maximum_tasks=active;
            result=ntm_shot_application_update(&application,buttons);assert(result==0 || result==1);assert(++count<1200);
            if(application.banner.phase)seen_banner=1;
            if(application.raster.callback)seen_exit=1;
            if(application.started)assert(random_cursor==(uint8_t)(prepare_cursor+2));
        }
        if(mode==1)assert(app.boundary==NTM_INTRO_BOUNDARY_PRIOR_SELECTION && !application.started);
        else if(mode==4 || mode==6) {
            assert(application.boundary==NTM_SHOT_BOUNDARY_UNSUPPORTED);unsupported_flows++;
            if(mode==4)assert(application.banner.phase==NTM_SHOT_BANNER_HOLD && !seen_exit);
            else assert(!application.started && !seen_banner);
        } else {
            assert(application.boundary==NTM_SHOT_BOUNDARY_SCENE_SETUP && application.scene.phase==2);
            assert(application.banner.phase==NTM_SHOT_BANNER_DONE && seen_banner && seen_exit);
            assert(application.start_update==tee.complete_update+1 && application.banner_update==application.start_update);
            assert(application.complete_update==d->updates && !application.raster.callback);
            assert(!d->parent.viewport[2] && !s->display.zero_flags[8]);
            assert(application.scene.player.stroke_snapshot==1 && tee.hole.players[0][2]==1);
            assert(tee.hole.hole[0x70]==4 && tee.hole.hole[0x73]==4 && tee.hole.hole[0x76]==4);
            assert(maximum_tasks>=6 && !app.fallback.callback);
            for(i=0;i<8;i++)assert(s->palette.slots[i]&32768);
        }
        flows++;
    }
    assert(flows==24 && unsupported_flows==6);
    puts("shot application:24 authored flows, real task pool, palette/raster lifecycle, explicit unsupported branches passed");
    return 0;
}
