/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_golfer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int drawing(ntm_task_context *c,ntm_task *t){(void)c;(void)t;assert(0);return -1;}
static void putw(uint8_t *p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void putl(uint8_t *p,uint32_t v){unsigned i;for(i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
int main(void)
{
    static uint8_t map[320],masks[8],shapes[2048],ground[60],foreground[60],frames[64],art[9],tiles[16];
    static const uint8_t selectors[3]={0,0,0},overrides[8]={2,0,2,0,2,0,2,0},green[6]={255,255,2,0,0,0};
    static const uint8_t pin[4]={255,255,92,0},metadata[5]={60,0,88,0,2},indices[2]={0,0},colors[8]={0};
    ntm_shot_golfer_asset assets[3];ntm_shot_aim_resources aim_resources={0};ntm_shot_golfer_resources resources={0};
    ntm_terrain_resources terrain={{map,sizeof(map)},{map,sizeof(map)},masks,shapes,sizeof(masks),sizeof(shapes)};
    unsigned i,course,code,queue,occupied,full,cases=0;
    for(i=0;i<64;i++){putw(map+2*i,128+3*i);map[128+3*i]=64;}
    for(i=0;i<15;i++){putw(ground+4*i,2);ground[4*i+2]=252;putw(foreground+4*i,4);foreground[4*i+2]=252;}
    frames[1]=7;frames[63]=9;art[0]=1;
    for(i=0;i<3;i++){assets[i].selector=(uint16_t)(2*i);assets[i].asset.data=art;assets[i].asset.size=sizeof(art);assets[i].asset.tiles=tiles;assets[i].asset.tiles_size=sizeof(tiles);}
    resources.course_selectors=selectors;resources.course_selectors_size=3;resources.ground_records=ground;resources.ground_records_size=60;
    resources.foreground_records=foreground;resources.foreground_records_size=60;resources.frames=frames;resources.frames_size=64;
    resources.overrides=overrides;resources.overrides_size=8;resources.green_records=green;resources.green_records_size=6;
    resources.pin_records=pin;resources.pin_records_size=4;resources.metadata=metadata;resources.metadata_size=5;resources.assets=assets;resources.asset_count=3;resources.aim=&aim_resources;
    for(course=0;course<3;course++)for(i=0;i<2;i++){ntm_palette_job job={1,0,0,1,0,0,indices,colors,2,8};resources.palettes[course][i]=job;}
    for(course=0;course<3;course++)for(code=0;code<15;code++)for(queue=0;queue<3;queue++)for(occupied=0;occupied<9;occupied++)for(full=0;full<2;full++){
        ntm_shot_golfer s={0};ntm_task task={0};ntm_hole_prepare_state h={0};ntm_shot_aim aim={0};
        ntm_course_screen screen={0};ntm_course_palette palette={0};ntm_display_reset_state display={0};ntm_course_cursor objects[32]={{0}};
        uint16_t x[32]={0},words[2]={31,47},detail=456;uint8_t scroll_x[2]={17,33},scroll_y[2]={27,43};
        ntm_course_intro_context intro={0};ntm_tee_setup_context shared={0};ntm_shot_scene_object_context scene={0};ntm_shot_golfer_context ctx={&scene,&terrain,&detail,0};
        unsigned before=(queue==0?0:queue==1?248:256),requests=code==6?2:3;
        screen.stamp_bytes=(uint16_t)before;s.drawing=drawing;memset(s.task_bytes,165,sizeof(s.task_bytes));
        for(i=0;i<8;i++)palette.slots[i]=(uint16_t)(4*i+(i<occupied?1024:65280));
        for(i=0;i<32;i++){objects[i].id=display.object_ids[i]=(uint8_t)i;display.list[i]=(uint16_t)i;}
        display.zero_flags[8]=(uint8_t)(full?32:0);display.zero_words[0]=(uint16_t)(full?64:0);
        intro.screen=&screen;intro.palette=&palette;intro.display=&display;intro.screen_words=words;intro.scroll_x=scroll_x;intro.scroll_y=scroll_y;
        shared.intro=&intro;shared.objects=objects;shared.x=x;scene.shared=&shared;scene.hole=&h;scene.aim=&aim;
        h.hole[4]=(uint8_t)course;h.hole[0x70]=(uint8_t)code;putw(h.auxiliary+7,65535);
        putw(h.hole+0x2a,300);putw(h.hole+0x2c,300);putw(h.hole+0x2e,400);putw(h.hole+0x30,400);putw(h.hole+0x3c,200);putw(h.hole+0x3e,200);putw(h.hole+0x6a,300);
        putl(h.hole+0x44,50u<<16);putl(h.hole+0x48,50u<<16);putl(h.hole+0x50,50u<<16);putl(h.hole+0x54,50u<<16);putl(h.hole+0x18,230u<<16);putl(h.hole+0x1c,230u<<16);putl(h.hole+0xd4,65536);
        assert(!ntm_shot_golfer_initialize(&s,&task,&ctx,&resources));assert(s.phase==1 && task.callback==drawing);
        assert(scroll_x[0]==17 && scroll_y[0]==27 && !scroll_x[1] && !scroll_y[1]);
        assert(objects[5].tag==51 && display.object_tags[5]==51 && objects[5].velocity==92 && objects[5].y==119);
        assert(display.zero_flags[8]==(full?32:1));assert(screen.stamp_bytes==(before+8*requests>256?256:before+8*requests));
        assert(s.task_bytes[0]==0 && s.task_bytes[4]==0 && s.task_bytes[5]==9 && s.task_bytes[6]==9 && s.task_bytes[7]==252);
        if(code==6)assert(s.surface==6 && s.task_bytes[1]==1 && s.task_bytes[2]==7 && s.task_bytes[3]==4 && detail==456);
        else assert(s.surface==0 && s.task_bytes[1]==165 && s.task_bytes[2]==165 && s.task_bytes[3]==252 && detail==0 && s.sample_x==(code>=11?78u:74u)*65536u);
        assert(ntm_shot_golfer_initialize(&s,&task,&ctx,&resources)==-1);cases++;
    }
    printf("shot golfer:%u authored initializer/queue/resource cases PASS\n",cases);return 0;
}
