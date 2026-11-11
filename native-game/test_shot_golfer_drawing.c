/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_golfer_drawing.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int callback(ntm_task_context *c,ntm_task *t){(void)c;(void)t;assert(0);return -1;}
static int animation(ntm_task_context *c,ntm_task *t){(void)c;(void)t;assert(0);return -1;}
static void putw(uint8_t *p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void putl(uint8_t *p,uint32_t v){unsigned i;for(i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
int main(void)
{
    static const uint8_t art[11]={1,0,0,0,0,0,0,0,1,0,255},tile[16]={60};
    static uint8_t angles[2050],eligible[16],depths[20],clip[4]={138,144,0,8};
    static const uint8_t metadata[5]={60,0,16,0,2};
    ntm_shot_golfer_drawing_resources r={0};ntm_shot_golfer_animation_resources animations;
    ntm_shot_aim_resources aim={0};ntm_screen_asset asset={art,sizeof(art),tile,sizeof(tile)};
    unsigned present,blocked,full,q,depth,kind,i,j,row,cases=0;
    for(i=0;i<5;i++){putw(depths+4*i,i==4?65535:10u*(i+1u));putw(depths+4*i+2,4*i);}
    aim.angles=angles;aim.angles_size=sizeof(angles);r.features.aim=&aim;r.animation=&animations;
    r.rim=asset;for(i=0;i<4;i++)r.green[i]=asset;
    for(i=0;i<5;i++)for(j=0;j<4;j++)animations.frames[i][j]=asset;
    r.eligible=eligible;r.eligible_size=16;r.depth_records=depths;r.depth_records_size=20;
    r.clip_records=clip;r.clip_records_size=4;r.clip_origin=0;r.metadata=metadata;r.metadata_size=5;
    for(present=0;present<2;present++)for(blocked=0;blocked<2;blocked++)for(full=0;full<2;full++)for(q=0;q<3;q++)for(depth=5;depth<=50;depth+=5)for(kind=0;kind<5;kind++) {
        ntm_shot_golfer_drawing s={0};ntm_shot_golfer golfer={0};ntm_task task={0};ntm_hole_prepare_state hole={0};
        ntm_course_screen screen={0};ntm_display_reset_state display={0};ntm_course_cursor objects[32]={{0}};
        uint16_t x[32]={0},words[2]={37,51};uint8_t maps[4096],tiles[8192],saved[32],features[6];
        ntm_course_intro_context intro={0};ntm_tee_setup_context shared={0};ntm_shot_scene_object_context scene={0};
        unsigned before=q==0?0:q==1?248:256,draw=present&&!blocked,variant=(depth-1)/10;
        memset(maps,51,sizeof(maps));memset(tiles,119,sizeof(tiles));memset(saved,165,sizeof(saved));
        s.center_x=123;s.half_width=456;golfer.phase=1;golfer.drawing=callback;golfer.surface=4;task.callback=callback;
        golfer.task_bytes[1]=golfer.task_bytes[5]=1;golfer.task_bytes[2]=5;golfer.task_bytes[6]=7;golfer.task_bytes[3]=(uint8_t)(kind==4?252:4*kind);golfer.task_bytes[7]=0;
        screen.stamp_bytes=(uint16_t)before;eligible[4]=(uint8_t)blocked;
        for(i=0;i<32;i++){objects[i].id=display.object_ids[i]=(uint8_t)i;objects[i].tag=display.object_tags[i]=123;display.list[i]=(uint16_t)i;}
        display.zero_flags[8]=(uint8_t)(full?32:0);display.zero_words[0]=(uint16_t)(full?64:0);
        putl(hole.hole+0x44,100u<<16);putl(hole.hole+0x48,100u<<16);putl(hole.hole+0xd4,65536);putw(hole.hole+0x6a,192);
        features[0]=(uint8_t)(present?7:255);putw(features+1,100+depth);putw(features+3,100);features[5]=255;
        r.features.features=features;r.features.features_size=sizeof(features);
        intro.screen=&screen;intro.display=&display;intro.maps=maps;intro.screen_words=words;
        shared.intro=&intro;shared.objects=objects;shared.x=x;shared.tiles=tiles;scene.shared=&shared;scene.hole=&hole;scene.saved_map=saved;
        assert(!ntm_shot_golfer_draw_initialize(&s,&golfer,&task,&scene,&r,animation));
        assert(s.phase==1 && task.callback==animation && golfer.phase==1);
        assert(s.features.view[0]==(draw?7:255));
        assert(word(maps+0x1c4)==0x1f5 && tiles[16*501]==60);
        if(draw && variant<4) {
            assert(s.center_x==0 && s.half_width==(variant==0?32:variant==3?16:24));
            assert(display.reset_words[0]==0 && tiles[16*292]==60);
        }else assert(s.center_x==123 && s.half_width==456);
        if(draw && variant<2) {
            assert(objects[18].tag==52 && display.object_tags[18]==52 && objects[18].velocity==(variant?16:20));
            assert(x[18]==109 && objects[18].y==147 && objects[18].tile==325 && objects[18].flags==128);
            assert(display.zero_flags[8]==(full?32:1));
        }else assert(objects[18].tag==123 && display.zero_flags[8]==(full?32:0));
        for(row=0;row<4;row++)for(j=0;j<8;j++)assert(saved[8*row+j]==maps[0x10e +64*row+j]);
        assert(screen.stamp_bytes==(before+8*(kind==4?1:2)>256?256:before+8*(kind==4?1:2)));
        assert(golfer.task_bytes[4]==2 && golfer.task_bytes[5]==7);
        assert(golfer.task_bytes[0]==(kind==4?0:2) && golfer.task_bytes[1]==(kind==4?1:5));
        assert(ntm_shot_golfer_draw_initialize(&s,&golfer,&task,&scene,&r,animation)==-1);cases++;
    }
    /* All rejection happens before live drawing, registration, queue writes,
     * feature scratch, callback replacement, or task scratch advances. */
    {
        ntm_shot_golfer_drawing s={0},old;ntm_shot_golfer g={0},oldg;ntm_task task={0};
        ntm_hole_prepare_state hole={0};ntm_course_screen screen={0},oldscreen;ntm_display_reset_state display={0};ntm_course_cursor objects[32]={{0}};
        uint8_t maps[4096],tiles[8192],saved[32],features[6]={7,110,0,100,0,255};uint16_t x[32]={0},words[2]={0};
        ntm_course_intro_context intro={0};ntm_tee_setup_context shared={0};ntm_shot_scene_object_context scene={0};
        intro.screen=&screen;intro.display=&display;intro.maps=maps;intro.screen_words=words;
        shared.intro=&intro;shared.objects=objects;shared.x=x;shared.tiles=tiles;scene.shared=&shared;scene.hole=&hole;scene.saved_map=saved;
        putl(hole.hole+0x44,100u<<16);putl(hole.hole+0x48,100u<<16);putl(hole.hole+0xd4,65536);
        g.phase=1;g.drawing=callback;g.task_bytes[3]=g.task_bytes[7]=252;task.callback=callback;
        r.features.features=features;r.features.features_size=6;r.metadata_size=0;old=s;oldg=g;oldscreen=screen;
        memset(maps,165,sizeof(maps));memset(tiles,90,sizeof(tiles));memset(saved,51,sizeof(saved));
        assert(ntm_shot_golfer_draw_initialize(&s,&g,&task,&scene,&r,animation)==-1);
        assert(!memcmp(&s,&old,sizeof(s)) && !memcmp(&g,&oldg,sizeof(g)) && !memcmp(&screen,&oldscreen,sizeof(screen)) && task.callback==callback);
        for(i=0;i<4096;i++)assert(maps[i]==165);
        for(i=0;i<8192;i++)assert(tiles[i]==90);
        for(i=0;i<32;i++)assert(saved[i]==51);
        r.metadata_size=5;r.rim.size=3;
        assert(ntm_shot_golfer_draw_initialize(&s,&g,&task,&scene,&r,animation)==-1 && !memcmp(&s,&old,sizeof(s)));
        for(i=0;i<4096;i++)assert(maps[i]==165);
        for(i=0;i<8192;i++)assert(tiles[i]==90);
    }
    printf("shot golfer drawing:%u authored whole callback cases PASS\n",cases);return 0;
}
