/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_camera.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t quarter[4100],viewport[4],dirty,maps[4096],saved_maps[4096],scroll[2];
static uint8_t layout[65568],blocks[65568];
static ntm_course_cursor objects[32];static uint16_t x[32],words[2];
static ntm_course_intro_context intro;static ntm_tee_setup_context shared;
static ntm_hole_prepare_state hole;static ntm_shot_aim aim;
static ntm_shot_marker_context context;static ntm_shot_marker_resources resources;
static void putw(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void putl(uint8_t *p,uint32_t v){putw(p,(uint16_t)v);putw(p+2,(uint16_t)(v>>16));}
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static void start(void)
{
    memset(&aim,0,sizeof aim);memset(&hole,0,sizeof hole);memset(objects,0,sizeof objects);
    memset(x,0,sizeof x);memset(words,0,sizeof words);memset(&context,0,sizeof context);
    intro.screen_words=words;intro.viewport=viewport;shared.intro=&intro;shared.objects=objects;shared.x=x;
    context.shared=&shared;context.hole=&hole;context.aim=&aim;context.dirty=&dirty;
    resources.quarter=quarter;resources.quarter_size=sizeof quarter;
    putw(hole.hole+0x2a,500);putw(hole.hole+0x2c,600);aim.origin_x=64;aim.origin_y=96;
    viewport[0]=viewport[1]=0;viewport[2]=160;viewport[3]=152;
}
int main(void)
{
    static const int8_t dx[16]={0,0,0,0,-8,-8,-8,0,8,8,8,0,0,0,0,0};
    static const int8_t dy[16]={0,-8,8,0,0,-8,8,0,0,-8,8,0,0,0,0,0};
    ntm_task task,nodes[5];ntm_task_context tasks;ntm_shot_camera_state state;
    ntm_aim_world_state world,snapshot;ntm_aim_world_resources wr;unsigned i,j,k,count=0;int rc;
    for(i=0;i<=1024;i++)putl(quarter+4*i,64u*i);
    memset(&task,0,sizeof task);memset(&tasks,0,sizeof tasks);
    for(i=0;i<256;i++)for(j=0;j<256;j++){
        start();state.saved_x=17;state.saved_y=23;context.held=(uint8_t)i;
        assert(!ntm_shot_camera_active(&state,&task,&tasks,&context,&resources,(uint8_t)j));
        if(!(j&32)){assert(!aim.reset_4416 && state.saved_x==17 && state.saved_y==23);continue;}
        assert(state.saved_x==64 && state.saved_y==96);
        if(!(i&32)){assert(aim.reset_4416==4 && aim.origin_x==64 && aim.origin_y==96);continue;}
        assert(aim.reset_4416==2 && aim.origin_x==(uint16_t)(64+dx[i&15]) && aim.origin_y==(uint16_t)(96+dy[i&15]));
        /*20ca34 must never move the main direction marker2. */
        assert(x[2]==0 && objects[2].y==0);
    }
    for(i=0;i<256;i++){
        start();aim.reset_4416=2;aim.origin_x=500;aim.origin_y=600;state.saved_x=0xfff0;state.saved_y=0;dirty=255;
        context.held=0;assert(!ntm_shot_camera_active(&state,&task,&tasks,&context,&resources,0));
        assert(aim.reset_4416==4 && aim.origin_x==0xfff0 && !aim.origin_y && !dirty);
        assert(!viewport[2] && viewport[3]==152 && words[0]==0xfff0 && !words[1]);
        assert(!ntm_shot_camera_active(&state,&task,&tasks,&context,&resources,255));assert(!aim.reset_4416 && viewport[2]==160);
        aim.reset_4416=(uint8_t)i;if(i!=0 && i!=2 && i!=4)assert(ntm_shot_camera_active(&state,&task,&tasks,&context,&resources,0)==2);
    }
    for(i=1;i<256;i++)for(j=0;j<4;j++){
        start();memset(nodes,0,sizeof nodes);memset(&tasks,0,sizeof tasks);aim.reset_d1=(uint8_t)i;
        for(k=0;k<5;k++){nodes[k].next=nodes+(k+1)%5;nodes[k].previous=nodes+(k+4)%5;}
        nodes[1].priority=1;nodes[2].priority=2;nodes[3].priority=2;nodes[2].flags=(uint8_t)(j&1);nodes[3].flags=(uint8_t)(j>>1);
        tasks.current=nodes+1;tasks.free_head=NULL;
        assert(!ntm_shot_camera_active(&state,nodes+1,&tasks,&context,&resources,0));
        assert(tasks.current==nodes && nodes[0].next==nodes+4 && nodes[4].previous==nodes && tasks.free_head==nodes+1);
    }
    memset(&wr,0,sizeof wr);wr.layout.data=layout;wr.layout.size=sizeof layout;wr.layout.origin=32768;
    wr.blocks.data=blocks;wr.blocks.size=sizeof blocks;wr.blocks.origin=32768;wr.width=16;wr.layout_tag=1;wr.blocks_tag=2;
    for(i=0;i<sizeof layout;i+=2)putw(layout+i,(uint16_t)((i/2)&1023));
    for(i=0;i<sizeof blocks;i++)blocks[i]=(uint8_t)(i^(i>>5));
    /* Every origin low-byte combination, every direction threshold, and both
     * circular map layers, with authored resources and preservation checks. */
    for(i=0;i<256;i++)for(j=0;j<256;j++){
        memset(&world,0,sizeof world);putw(world.bytes,(uint16_t)(i-64));putw(world.bytes+2,(uint16_t)(j-64));
        putw(world.bytes+4,64);putw(world.bytes+6,64);putl(world.bytes+8,1);putl(world.bytes+12,2);world.bytes[16]=16;
        putw(world.bytes+44,(uint16_t)(0x9000+2048*(i&1)));world.bytes[50]=0xa5;world.bytes[51]=0x5a;
        memset(maps,0xcd,sizeof maps);assert(!ntm_shot_camera_scroll(&world,&wr,maps,scroll));
        assert(scroll[0]==(uint8_t)(i-64) && scroll[1]==(uint8_t)(j-64));
        assert(world.bytes[50]==0xa5 && world.bytes[51]==0x5a);
        for(k=0;k<2048;k++)assert(maps[k+2048*((i&1)^1)]==0xcd);
        assert((uint16_t)(word(world.bytes+2)-word(world.bytes+6)+32u)<64u);count++;
    }
    /* A failed source lookup is atomic, including raw scroll registers. */
    putw(world.bytes,96);putw(world.bytes+2,96);putw(world.bytes+4,64);putw(world.bytes+6,64);
    snapshot=world;memcpy(saved_maps,maps,sizeof maps);scroll[0]=91;scroll[1]=37;wr.layout.size=0;
    rc=ntm_shot_camera_scroll(&world,&wr,maps,scroll);assert(rc==-1 && !memcmp(&world,&snapshot,sizeof world));
    assert(!memcmp(maps,saved_maps,sizeof maps) && scroll[0]==91 && scroll[1]==37);
    printf("PASS camera:65536 input pairs,256 restore/state cases,1020 real child/task exits,%u scroll cases,atomic malformed rejection\n",count);return 0;
}
