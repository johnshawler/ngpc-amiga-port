/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_fire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int spin(ntm_task_context *c,ntm_task *t){(void)c;(void)t;assert(0);return -1;}
static int trajectory(ntm_task_context *c,ntm_task *t){(void)c;(void)t;assert(0);return -1;}
static int golfer(ntm_task_context *c,ntm_task *t){(void)c;(void)t;assert(0);return -1;}
static int closed(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return -1;}
static int opened(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return -1;}
static int ready(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return -1;}
static ntm_shot_fire fire(void){ntm_shot_fire s={spin,trajectory,golfer,closed,opened,ready,0,0};return s;}
int main(void)
{
    unsigned mask,free_count,p,n,phase,total=0;static uint8_t maps[4096];
    static const uint8_t offsets[2]={0,0},colors[8]={0,1,2,3,4,5,6,7};
    ntm_shot_fire_resources res={{1,0,0,1,0,0,offsets,colors,2,8}};
    for(mask=0;mask<16;mask++)for(free_count=0;free_count<4;free_count++)for(p=0;p<9;p++){
        static const unsigned slots[4]={0,2,5,17};
        ntm_task nodes[5]={{0}},fallback={0};ntm_task_context tasks={0};ntm_course_cursor objects[32]={{0}};
        ntm_display_reset_state display={0};ntm_course_palette palette={0};ntm_course_intro_context intro={0};
        ntm_tee_setup_context shared={0};ntm_shot_aim aim={0};uint8_t overlay=43,flags=255,view[4]={1,2,3,4};
        ntm_shot_fire s=fire();ntm_shot_fire_context ctx={&shared,&aim,&overlay};unsigned i,y,x,registered=0;
        memset(maps,165,sizeof(maps));for(i=0;i<4;i++)if(mask&(1u<<i)){display.list[registered++]=(uint16_t)slots[i];objects[slots[i]].tag=3;}else objects[slots[i]].tag=65535;
        display.zero_flags[8]=(uint8_t)registered;display.zero_words[0]=(uint16_t)(2*registered);
        for(i=0;i<32;i++){objects[i].id=display.object_ids[i]=(uint8_t)i;display.object_tags[i]=objects[i].tag;}
        for(i=0;i<8;i++)palette.slots[i]=(uint16_t)(4*i+(i>=p?65280:1024));
        nodes[0].next=nodes+1;nodes[1].previous=nodes;nodes[0].priority=255;
        for(i=0;i<free_count;i++){nodes[i+2].next=i+1<free_count?nodes+i+3:0;nodes[i+2].result=77;}
        tasks.free_head=free_count?nodes+2:0;tasks.current=nodes;
        intro.maps=maps;intro.display=&display;intro.palette=&palette;intro.viewport=view;intro.flags=&flags;
        shared.intro=&intro;shared.tasks=&tasks;shared.fallback=&fallback;shared.objects=objects;
        assert(!ntm_shot_fire_initialize(&s,nodes,&ctx,&res));assert(overlay==255 && aim.reset_d1==1 && flags==247);
        assert(!view[0] && !view[1] && !view[2] && view[3]==152);assert(!display.zero_flags[8] && !display.zero_words[0]);
        for(i=0;i<4;i++)assert(objects[slots[i]].tag==65535);
        for(y=0;y<32;y++)for(x=0;x<128;x++){unsigned at=128*y+x;int cleared=at>=0x10e && at<0x1d6 && ((at-0x10e)%64)<8;assert(maps[at]==(cleared?0:165));}
        assert(nodes[0].delay==3 && nodes[0].callback==closed && s.phase==1);
        assert(palette.status==(p<8?65535:0));assert(!tasks.free_head);
        if(free_count)assert(nodes[0].next==nodes+free_count+1 && nodes[free_count+1].callback==(free_count==1?spin:free_count==2?trajectory:golfer));
        for(i=0;i<free_count;i++)assert(nodes[i+2].result==77 && nodes[i+2].priority==0);
        total++;
    }
    for(phase=1;phase<=3;phase++)for(n=0;n<65536;n++)for(p=0;p<4;p++){
        ntm_shot_fire s=fire();ntm_task t={0};ntm_course_intro_context intro={0};ntm_tee_setup_context shared={0};
        ntm_shot_aim aim={0};uint8_t overlay=255,view[4]={3,4,5,6};ntm_shot_fire_context ctx={&shared,&aim,&overlay};
        int crossing=!!(((uint16_t)(n-1u))&32768u),gating=phase==3 || (phase==2 && crossing),expected=gating && p?2:0;
        intro.viewport=view;shared.intro=&intro;s.phase=(uint8_t)phase;t.delay=(uint16_t)n;t.callback=phase==1?closed:phase==2?opened:ready;
        assert(ntm_shot_fire_tick(&s,&t,&ctx,(uint8_t)(16*p))==expected);
        if(phase==1 && crossing){assert(s.phase==2 && t.callback==opened && t.delay==3 && overlay==0 && view[2]==160);}
        if(gating){assert(t.callback==ready);assert(s.boundary==(p>=2?NTM_SHOT_FIRE_CANCEL:p?NTM_SHOT_FIRE_POWER:0));}
        if(expected){ntm_task saved=t;assert(ntm_shot_fire_tick(&s,&t,&ctx,0)==2);assert(!memcmp(&saved,&t,sizeof(t)));}
    }
    for(phase=1;phase<=2;phase++)for(n=0;n<256;n++)for(p=0;p<4;p++){
        static const uint8_t quarter[4100]={0},metadata[10]={0};
        ntm_shot_marker_resources resources={quarter,sizeof(quarter),metadata,sizeof(metadata)};
        ntm_shot_markers s={closed,opened,4,0,512};ntm_task nodes[4]={{0}};ntm_task_context tasks={0};
        ntm_hole_prepare_state hole={0};ntm_shot_aim aim={0};ntm_course_cursor objects[32]={{0}};
        ntm_display_reset_state display={0};ntm_course_intro_context intro={0};ntm_tee_setup_context shared={0};
        uint16_t x[32]={0},words[2]={0};uint8_t dirty=(uint8_t)(p&1);
        ntm_shot_marker_context ctx={&shared,&hole,&aim,&dirty,0,0};unsigned i;
        s.phase=(uint8_t)phase;aim.reset_d1=(uint8_t)(phase==1?1:n);aim.angle=1234;
        intro.screen_words=words;intro.display=&display;shared.intro=&intro;shared.objects=objects;shared.x=x;
        x[1]=44;x[2]=333;objects[1].y=55;objects[2].y=444;x[3]=99;objects[3].y=88;
        for(i=0;i<3;i++){nodes[i].next=nodes+(i+1)%3;nodes[i].previous=nodes+(i+2)%3;}
        nodes[0].flags=(uint8_t)(p>>1);nodes[0].callback=phase==1?closed:opened;
        tasks.current=nodes;tasks.free_head=nodes+3;
        assert(!ntm_shot_fire_markers_tick(&s,nodes,&tasks,&ctx,&resources));
        if(phase==2 && n==2){assert(tasks.current==nodes+2 && nodes[2].next==nodes+1);assert(tasks.free_head==((p>>1)?nodes+3:nodes));}
        else {assert(tasks.current==nodes && nodes[0].callback==opened && s.phase==2);assert(!dirty);assert(x[3]==((p&1)?44:99));assert(objects[3].y==((p&1)?55:88));}
        assert(x[2]==333 && objects[2].y==444 && aim.angle==1234);
    }
    printf("shot fire: %u entry,786432 wait/input and2048 marker cases PASS\n",total);return 0;
}
