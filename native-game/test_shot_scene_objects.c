/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_scene_objects.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void put(uint8_t *p,unsigned n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
int main(void)
{
    ntm_shot_scene_object_context c;ntm_shot_scene_object_resources r;
    ntm_tee_setup_context shared;ntm_course_intro_context intro;ntm_display_reset_state display;
    ntm_course_cursor objects[32];ntm_hole_prepare_state hole;ntm_shot_player_state player;
    ntm_shot_aim aim;uint16_t x[32],words[2];uint8_t metadata[55],power[256],remainders[10],saved[32],maps[4096],world[52];
    unsigned i,j;static const unsigned velocities[11]={8,44,40,48,0,120,52,56,128,124,132};
    memset(&c,0,sizeof c);memset(&shared,0,sizeof shared);memset(&intro,0,sizeof intro);
    memset(&display,0,sizeof display);memset(objects,0,sizeof objects);memset(&hole,0,sizeof hole);
    memset(&player,0,sizeof player);memset(&aim,0,sizeof aim);memset(x,0,sizeof x);memset(world,0,sizeof world);
    for(i=0;i<11;i++){put(metadata+5*i,56);put(metadata+5*i+2,velocities[i]);metadata[5*i+4]=1;}
    for(i=0;i<256;i++)power[i]=(uint8_t)i;
    for(i=0;i<10;i++)remainders[i]=(uint8_t)(i*3);
    for(i=0;i<4096;i++)maps[i]=(uint8_t)(i^(i>>4));
    r.metadata=metadata;r.metadata_size=55;r.power_scale=power;r.power_scale_size=256;r.remainders=remainders;r.remainders_size=10;
    intro.display=&display;intro.screen_words=words;intro.maps=maps;shared.intro=&intro;shared.objects=objects;shared.x=x;
    c.shared=&shared;c.hole=&hole;c.player=&player;c.aim=&aim;c.saved_map=saved;player.other_player=1;
    assert(ntm_tee_objects_reset(&display,objects)==0);
    objects[4].tag=5;objects[4].width=68;objects[4].velocity=0;
    assert(ntm_tee_object_register(&display,objects+4)==0);
    hole.players[1][1]=255;put(hole.hole+0x46,0xfff8);put(hole.hole+0x4a,0x8010);
    put(hole.hole+0x1a,81);put(hole.hole+0x1e,33);put(hole.hole+0x32,48);put(hole.hole+0x34,52);
    hole.hole[0x78]=19;hole.players[0][0x23]=7;aim.club=1;put(world,100);put(world+2,200);
    assert(ntm_shot_scene_objects_initialize(&c,&r,world)==0);
    assert(display.zero_flags[8]==8 && display.zero_words[0]==16 && objects[4].tag==65535);
    assert(display.zero_flags[0]==0 && display.capacity==34 && words[0]==100 && words[1]==200);
    assert(x[0]==65528 && objects[0].y==0x8010 && x[1]==147 && objects[1].y==(uint16_t)(0xf002+252));
    assert(x[5]==81 && objects[5].y==33 && x[6]==158 && objects[6].y==256);
    assert(x[7]==108 && objects[7].y==332 && x[8]==100 && objects[8].y==264);
    assert(hole.hole[0x7e]==197 && hole.hole[0x7f]==173 && objects[10].y==115);
    for(i=0;i<4;i++)for(j=0;j<8;j++)assert(saved[8*i+j]==maps[0x10e +64*i+j]);
    for(i=0;i<256;i++)for(j=0;j<16;j++){
        unsigned v=(j==13?59:(uint8_t)(i+62))^128u;
        hole.players[0][0x23]=(uint8_t)i;
        assert(ntm_shot_scene_power_limit(&hole,&player,(uint8_t)j,&r)==0);
        assert(hole.hole[0x7e]==v && hole.hole[0x7f]==(uint8_t)((v/10)*8+(v%10)*3));
    }
    assert(ntm_shot_scene_gauges(0,&r)==-1);
    r.metadata_size=54;assert(ntm_shot_scene_ball(&c,&r)==-1);
    r.metadata_size=55;r.power_scale_size=255;assert(ntm_shot_scene_gauges(&c,&r)==-1);
    puts("shot scene objects:4096 power cases, genuine producers and saved HUD rows");return 0;
}
