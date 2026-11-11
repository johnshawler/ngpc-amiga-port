/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_application.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef NTM_TEST_AIM
#include "shot_aim_application.h"
#ifdef NTM_TEST_OPEN
#include "shot_open_application.h"
static ntm_shot_open_application open_application;
#define aim_application (open_application.base)
static ntm_shot_open_resources open_resources;
static ntm_shot_scene_object_resources scene_resources;
static ntm_shot_club_resources club_resources;
static ntm_shot_marker_resources marker_resources;
static uint8_t scene_metadata[55],scene_power[256],scene_remainders[10],club_powers[56],club_lies[42];
#else
static ntm_shot_aim_application aim_application;
#endif
#define application (aim_application.base)
static ntm_shot_aim_resources aim_math;
static ntm_shot_aim_application_resources aim_resources;
static ntm_shot_aim_setup_resources aim_setup;
static ntm_shot_hud_resources aim_hud;
static ntm_aim_world_resources aim_worlds[54];
static uint8_t aim_angles[2050],aim_powers[52],aim_minimums[256],aim_layout[65568],aim_block[32],aim_indices[2],aim_commands[35];
static const uint8_t aim_metadata[5]={68,0,0,0,3};
#else
static ntm_shot_application application;
#endif
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
    #ifdef NTM_TEST_AIM
    aim_math.angles=aim_angles;aim_math.angles_size=sizeof aim_angles;
    for(i=0;i<12;i++){aim_math.powers[i]=aim_powers;aim_math.power_sizes[i]=sizeof aim_powers;}
    for(i=0;i<6;i++){aim_math.minimums[i]=aim_minimums;aim_math.minimum_sizes[i]=sizeof aim_minimums;}
    aim_setup.metadata=aim_metadata;aim_setup.metadata_size=5;
    aim_hud.font=aim_hud.header=aim_hud.label=assets[0];aim_hud.label_id=4;
    for(i=0;i<17;i++)aim_hud.wind[i]=assets[0];
    memset(aim_commands,255,sizeof aim_commands);
    for(i=0;i<54;i++) {
        ntm_aim_world_resources *w=aim_worlds+i;
        w->layout.data=aim_layout;w->layout.size=sizeof aim_layout;w->layout.origin=32768;
        w->blocks.data=aim_block;w->blocks.size=sizeof aim_block;
        w->tile_indices.data=aim_indices;w->tile_indices.size=2;
        w->tiles.data=pixels;w->tiles.size=16;
        w->commands.data=aim_commands;w->commands.size=sizeof aim_commands;
        w->colors.data=colors;w->colors.size=sizeof colors;w->width=w->height=8;
        aim_setup.panorama[i/18][i%18]=assets[0];aim_resources.objects[i]=&banner_objects;
    }
    aim_resources.aim=&aim_math;aim_resources.worlds=aim_worlds;aim_resources.world_count=54;
    aim_resources.hud=&aim_hud;aim_resources.setup=&aim_setup;aim_resources.palette=jobs[0];
#ifdef NTM_TEST_OPEN
    {static const uint8_t velocity[11]={8,44,40,48,0,120,52,56,128,124,132};
    for(i=0;i<11;i++){scene_metadata[5*i]=56;scene_metadata[5*i+2]=velocity[i];scene_metadata[5*i+4]=3;}}
    scene_resources.metadata=scene_metadata;scene_resources.metadata_size=55;
    scene_resources.power_scale=scene_power;scene_resources.power_scale_size=256;
    scene_resources.remainders=scene_remainders;scene_resources.remainders_size=10;
    club_resources.aim=&aim_math;club_resources.scene=&scene_resources;
    for(i=0;i<12;i++){unsigned j;club_resources.powers[i].data=club_powers;club_resources.powers[i].size=56;
        for(j=0;j<15;j++){club_resources.lies[i][j].data=club_lies;club_resources.lies[i][j].size=42;club_resources.overflow_lies[i][j]=club_resources.lies[i][j];}}
    for(i=0;i<23;i++)club_resources.labels[i]=assets[0];
    for(i=0;i<11;i++)club_resources.digits[i]=assets[0];
    for(i=0;i<19;i++)club_resources.lie_icons[i]=assets[0];
    marker_resources.quarter=quarter;marker_resources.quarter_size=4100;marker_resources.metadata=scene_metadata+10;marker_resources.metadata_size=10;
    open_resources.scene=&scene_resources;open_resources.club=&club_resources;open_resources.markers=&marker_resources;open_resources.objects=&banner_objects;
#endif
#endif
    for(mode=0;mode<8;mode++)for(selection=0;selection<3;selection++) {
        ntm_course_application *s=&app.selector;ntm_course_driver *d=&s->driver;
        unsigned count=0,maximum_tasks=0,seen_banner=0,seen_exit=0;uint8_t prepare_cursor=0;
        #ifdef NTM_TEST_AIM
#ifdef NTM_TEST_OPEN
        memset(&open_application,0,sizeof open_application);
#else
        memset(&aim_application,0,sizeof aim_application);
#endif
#else
        memset(&application,0,sizeof application);
#endif
        memset(s->screen.fill_status,255,8);
        for(i=0;i<8;i++)s->palette.slots[i]=(uint16_t)(0x8000+4*i);
        s->palette.pending[0]=s->palette.pending[1]=255;s->palette.background=128;
        d->parent.viewport[3]=152;s->screen.displayed_selection=(uint16_t)selection;random_cursor=3;
        app.records.golfer[0]=(uint8_t)((mode+selection)%6);app.records.golfer[1]=(uint8_t)((mode+selection+1)%6);
        assert(!ntm_shot_application_start(&application,&resources,courses,titles,objects,&round,&tee_resources,&holes,&shot));
        #ifdef NTM_TEST_AIM
        assert(!ntm_shot_aim_application_enable(&aim_application,&aim_resources));
#ifdef NTM_TEST_OPEN
        assert(!ntm_shot_open_application_enable(&open_application,&open_resources));
#endif
#endif
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
            #ifdef NTM_TEST_AIM
#ifdef NTM_TEST_OPEN
            assert(application.boundary==NTM_SHOT_BOUNDARY_AIM_OPEN && open_application.phase==2);
            assert(open_application.club.phase==1 && open_application.markers.phase==1 && open_application.hint.phase==0 && open_application.camera.phase==1);
            assert(open_application.handlers_installed && !open_application.input_dispatch && open_application.open_update==d->updates);
            assert(tee.objects[4].tag==65535 && s->display.zero_flags[8]>=10);
#else
            assert(application.boundary==NTM_SHOT_BOUNDARY_AIM_SETUP && aim_application.phase==1 && application.scene.phase==2);
            assert(s->display.zero_flags[8]==1 && tee.objects[4].tag==5);
            assert(tee.objects[4].width==68u+4u*tee.hole.hole[4] && tee.objects[4].velocity==4u*tee.hole.hole[5]);
            assert(aim_application.world.bytes[44]==0 && aim_application.world.bytes[45]==152);
            assert(s->screen.stamp_bytes==24);
#endif
#else
            assert(application.boundary==NTM_SHOT_BOUNDARY_SCENE_SETUP && application.scene.phase==2);
            assert(!s->display.zero_flags[8]);
#endif
            assert(application.banner.phase==NTM_SHOT_BANNER_DONE && seen_banner && seen_exit);
            assert(application.start_update==tee.complete_update+1 && application.banner_update==application.start_update);
            assert(application.complete_update==d->updates && !application.raster.callback);
#ifdef NTM_TEST_OPEN
            assert(d->parent.viewport[2]==160 && d->parent.viewport[3]==152 && !d->parent.viewport[0] && !d->parent.viewport[1]);
#else
            assert(!d->parent.viewport[2]);
#endif
            assert(application.scene.player.stroke_snapshot==1 && tee.hole.players[0][2]==1);
            assert(tee.hole.hole[0x70]==4 && tee.hole.hole[0x73]==4 && tee.hole.hole[0x76]==4);
            assert(maximum_tasks>=6 && !app.fallback.callback);
            for(i=0;i<8;i++)assert(s->palette.slots[i]&32768);
        }
        flows++;
    }
    assert(flows==24 && unsupported_flows==6);
    #ifdef NTM_TEST_AIM
#ifdef NTM_TEST_OPEN
    puts("shot open application:24 authored flows, genuine scene children and first-visible boundary passed");
#else
    puts("shot aim application:24 authored flows, real world/HUD/object services, explicit next-dispatch boundary passed");
#endif
#else
    puts("shot application:24 authored flows, real task pool, palette/raster lifecycle, explicit unsupported branches passed");
#endif
    return 0;
}
