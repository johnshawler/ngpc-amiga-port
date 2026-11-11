/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_markers.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Authored linear quarter-wave fixture, not the cartridge's trigonometry. */
static uint8_t quarter[4100],metadata[]={56,0,40,0,2,56,0,48,0,3};
static ntm_shot_marker_resources resources={quarter,sizeof quarter,metadata,sizeof metadata};
static ntm_course_cursor objects[32];static uint16_t x[32],words[2];
static ntm_display_reset_state display;static ntm_course_intro_context intro;
static ntm_tee_setup_context shared;static ntm_hole_prepare_state hole;
static ntm_shot_aim aim;static uint8_t dirty;
static ntm_shot_marker_context context;
static int tick_callback(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return -1;}
static int boundary_callback(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return -1;}
static void put(uint8_t *p,uint32_t v,unsigned n){unsigned i;for(i=0;i<n;i++)p[i]=(uint8_t)(v>>(8*i));}
static uint32_t read4(const uint8_t *p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint32_t random_word(void)
{static uint32_t v=0x20d1e9;v^=v<<13;v^=v>>17;v^=v<<5;return v;}
static uint32_t product(uint32_t a,uint32_t b)
{
    unsigned negative=(a^b)>>31;uint64_t wide;uint32_t out;
    if(a>>31)a=0u-a;
    if(b>>31)b=0u-b;
    wide=(uint64_t)a*b;out=(uint32_t)(wide>>16);
    if(negative && (a>=65536u || b>=65536u) && (wide&65535u))out++;
    return negative?0u-out:out;
}
static uint32_t direction(unsigned a)
{int v;a&=4095;v=a<=2048?(int)a-1024:3072-(int)a;return (uint32_t)(v*64);}
static void start(void)
{
    memset(objects,0,sizeof objects);memset(x,0,sizeof x);memset(words,0,sizeof words);
    memset(&intro,0,sizeof intro);memset(&shared,0,sizeof shared);memset(&hole,0,sizeof hole);
    memset(&aim,0,sizeof aim);memset(&context,0,sizeof context);dirty=0;
    assert(!ntm_tee_objects_reset(&display,objects));
    intro.display=&display;intro.screen_words=words;
    shared.intro=&intro;shared.objects=objects;shared.x=x;
    context.shared=&shared;context.hole=&hole;context.aim=&aim;context.dirty=&dirty;
}
int main(void)
{
    static const uint32_t edges[]={0,1,2,65535,65536,65537,0x7fffffff,0x80000000,0xffff0000,0xffff0001,0xfffffffe,0xffffffff};
    static const uint16_t lengths[]={0,48,49,160,161,32767,32768,65535};
    ntm_task task,saved_task,nodes[4];ntm_task_context tasks;
    ntm_shot_markers markers;ntm_shot_hint_delay hint,saved_hint;ntm_shot_camera_delay camera,saved_camera;
    unsigned i,j,k,count;uint32_t a,b,dx,dy,distance;uint16_t length,v,expected;
    for(i=0;i<=1024;i++)put(quarter+4*i,i*64,4);
    for(i=0;i<sizeof edges/sizeof edges[0];i++)for(j=0;j<sizeof edges/sizeof edges[0];j++)
        assert(ntm_shot_marker_multiply(edges[i],edges[j])==product(edges[i],edges[j]));
    for(i=0;i<50000;i++){a=random_word();b=random_word();assert(ntm_shot_marker_multiply(a,b)==product(a,b));}
    /* Preserve the original sub-unit negative-product rounding exception. */
    assert(ntm_shot_marker_multiply(0xffffffffu,1)==0);
    assert(ntm_shot_marker_multiply(0xfffeffffu,1)==0xfffffffeu);
    for(i=0;i<65536;i++)assert(ntm_shot_marker_direction(quarter,(uint16_t)i)==direction(i));
    start();put(hole.hole+0x44,0xffff8123u,4);put(hole.hole+0x48,0x80005a27u,4);
    x[1]=65000;objects[1].y=400;words[0]=65500;
    for(j=0;j<sizeof lengths/sizeof lengths[0];j++)for(i=0;i<4096;i++){
        length=lengths[j];put(hole.hole+0x6a,length,2);aim.angle=(uint16_t)i;objects[3].flags=0xa5;
        assert(!ntm_shot_markers_position(&context,&resources));dx=direction(i+1024);dy=direction(i);
        assert(read4(hole.hole+0xd4)==dx && read4(hole.hole+0xd8)==dy);
        distance=(length<32768 && length>48?40u:20u)<<16;
        assert(x[2]==(uint16_t)((product(dx,distance)+0xffff8123u)>>16));
        assert(objects[2].y==(uint16_t)((product(dy,distance)+0x80005a27u)>>16));
        distance=(length<32768 && length>160?30u:length>>3)<<16;
        assert(x[3]==(uint16_t)(65000+(product(dx,distance)>>16)));
        assert(objects[3].y==(uint16_t)(400+(product(dy,distance)>>16)));
        v=(uint16_t)(x[3]-words[0]);assert(objects[3].flags==(uint8_t)(0x85|((v>=32768 || v<=128)?32:0)));
    }
    for(count=30;count<=32;count++){
        start();j=0;
        for(i=0;i<32;i++)if(count==32 || (i!=2 && (count==31 || i!=3)))display.list[j++]=(uint16_t)i;
        assert(j==count);display.zero_flags[8]=(uint8_t)count;display.zero_words[0]=(uint16_t)(2*count);
        memset(&markers,0,sizeof markers);memset(&task,0,sizeof task);
        markers.tick=tick_callback;markers.waiting=boundary_callback;
        assert(!ntm_shot_markers_initialize(&markers,&task,&context,&resources));
        assert(markers.phase==1 && markers.step==4 && markers.limit==512 && dirty==255 && task.callback==tick_callback);
        assert(display.zero_flags[8]==32 && display.zero_words[0]==64);
        for(i=2;i<4;i++)assert(objects[i].tag==i+1 && display.object_tags[i]==i+1 && objects[i].second==16 && objects[i].tile==58+i);
    }
    start();memset(&markers,0,sizeof markers);memset(&task,0,sizeof task);
    markers.tick=tick_callback;markers.waiting=boundary_callback;
    assert(!ntm_shot_markers_initialize(&markers,&task,&context,&resources));
    assert(display.zero_flags[8]==2 && display.list[0]==0x1002 && display.list[1]==0x1003);
    /* Exhaust every wrapped delta for each direction at the inclusive limits. */
    for(j=4;j<=8;j+=4)for(i=0;i<65536;i++){
        context.held=(uint8_t)j;aim.angle_delta=(uint16_t)i;aim.initial_angle=4000;aim.angle=123;dirty=0;
        expected=(uint16_t)(i+(j==4?65532u:4u));
        if(expected>512 && expected<65024)expected=(uint16_t)i;
        assert(!ntm_shot_markers_tick(&markers,&task,&context,&resources));
        assert(aim.angle_delta==expected && !dirty);
        assert(aim.angle==(expected==i?123:(uint16_t)((4000u+expected)&4095u)));
    }
    context.held=12;aim.angle_delta=7;dirty=0;assert(!ntm_shot_markers_tick(&markers,&task,&context,&resources));assert(aim.angle_delta==7);
    context.held=8;aim.reset_4416=1;assert(!ntm_shot_markers_tick(&markers,&task,&context,&resources));assert(aim.angle_delta==7);
    dirty=255;aim.initial_angle=4090;assert(!ntm_shot_markers_tick(&markers,&task,&context,&resources));assert(aim.angle==1 && !dirty);
    aim.reset_d1=1;assert(ntm_shot_markers_tick(&markers,&task,&context,&resources)==2);
    assert(task.callback==boundary_callback && markers.phase==2);
    saved_task=task;assert(ntm_shot_markers_tick(&markers,&task,&context,&resources)==2);assert(!memcmp(&task,&saved_task,sizeof task));
    context.player_status=1;a=read4(hole.hole+0xd4);assert(ntm_shot_markers_position(&context,&resources)==2);assert(read4(hole.hole+0xd4)==a);
    context.player_status=0;resources.quarter_size=4099;assert(ntm_shot_markers_position(&context,&resources)==-1);resources.quarter_size=4100;
    resources.metadata_size=9;assert(ntm_shot_markers_position(&context,&resources)==-1);resources.metadata_size=10;
    assert(ntm_shot_markers_position(0,&resources)==-1);
    memset(&hint,0,sizeof hint);memset(&camera,0,sizeof camera);memset(&task,0,sizeof task);memset(&tasks,0,sizeof tasks);
    hint.delay=camera.delay=tick_callback;hint.active=camera.active=boundary_callback;
    assert(!ntm_shot_hint_initialize(&hint,&task,&tasks,1));
    assert(task.delay==29 && hint.phase==1 && hint.repeat==1 && hint.last==255 && !hint.pending && !hint.changed);
    for(i=0;i<29;i++)assert(!ntm_shot_hint_tick(&hint,&task));
    assert(!task.delay && ntm_shot_hint_tick(&hint,&task)==2 && task.delay==65535 && task.callback==boundary_callback && hint.phase==2);
    saved_hint=hint;saved_task=task;assert(ntm_shot_hint_tick(&hint,&task)==-1);assert(!memcmp(&hint,&saved_hint,sizeof hint) && !memcmp(&task,&saved_task,sizeof task));
    assert(!ntm_shot_camera_initialize(&camera,&task));assert(task.delay==7 && camera.phase==1);
    for(i=0;i<7;i++)assert(!ntm_shot_camera_tick(&camera,&task));
    assert(!task.delay && ntm_shot_camera_tick(&camera,&task)==2 && task.delay==65535 && task.callback==boundary_callback && camera.phase==2);
    saved_camera=camera;saved_task=task;assert(ntm_shot_camera_tick(&camera,&task)==-1);assert(!memcmp(&camera,&saved_camera,sizeof camera) && !memcmp(&task,&saved_task,sizeof task));
    for(i=0;i<65536;i++){
        hint.phase=1;task.callback=tick_callback;task.delay=(uint16_t)i;
        assert(ntm_shot_hint_tick(&hint,&task)==(((uint16_t)(i-1)&32768)?2:0));assert(task.delay==(uint16_t)(i-1));
        camera.phase=1;task.callback=tick_callback;task.delay=(uint16_t)i;
        assert(ntm_shot_camera_tick(&camera,&task)==(((uint16_t)(i-1)&32768)?2:0));assert(task.delay==(uint16_t)(i-1));
    }
    for(i=1;i<256;i++)for(j=0;j<4;j++)for(k=0;k<2;k++){
        memset(nodes,0,sizeof nodes);memset(&tasks,0,sizeof tasks);memset(&hint,0,sizeof hint);
        hint.delay=tick_callback;hint.active=boundary_callback;nodes[1].callback=tick_callback;
        nodes[0].next=nodes+1;nodes[1].previous=nodes;nodes[1].next=nodes+2;nodes[2].previous=nodes+1;
        nodes[1].priority=(uint8_t)(j&2?i:0);nodes[2].priority=(uint8_t)(j&1?i:0);nodes[1].flags=(uint8_t)k;
        tasks.current=nodes+1;tasks.free_head=nodes+3;
        assert(!ntm_shot_hint_initialize(&hint,nodes+1,&tasks,0));
        assert(!hint.phase && !nodes[1].delay);
        if(j==1)assert(nodes[1].callback==ntm_task_finish && tasks.current==nodes+1 && tasks.free_head==nodes+3);
        else assert(nodes[0].next==nodes+2 && nodes[2].previous==nodes && tasks.current==nodes && tasks.free_head==(k?nodes+3:nodes+1));
    }
    for(i=1;i<256;i++){
        memset(&hint,0,sizeof hint);hint.delay=tick_callback;hint.active=boundary_callback;
        assert(!ntm_shot_hint_initialize(&hint,&task,&tasks,(uint8_t)i));assert(task.delay==29 && hint.phase==1);
    }
    puts("PASS shot markers:50144 signed products,65536 directions,32768 positions,131072 delta cases,131072 countdown cases,2040 hint exits,255 hint delays; explicit active boundaries");return 0;
}
