/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_trajectory_swing.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int callback(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static unsigned sound_count;static uint16_t command;
static void sound(void *c,uint8_t service,uint16_t value,uint16_t param)
{(void)c;assert(param==0);assert(service==0 || service==2);sound_count++;command=value;}
int main(void)
{
 ntm_shot_trajectory_swing s;ntm_shot_trajectory p;ntm_task_context tasks;ntm_task nodes[4];
 ntm_shot_scene_object_context c;ntm_tee_setup_context tc;ntm_course_intro_context ic;
 ntm_course_screen screen;ntm_course_cursor objects[32];ntm_hole_prepare_state h;ntm_shot_aim aim;
 ntm_shot_trajectory_swing_resources r;unsigned power,j,mode;
 /* Authored fixture tables, not original resource bytes. */
 static const uint8_t hud[]={10,20,30,40,50,60},putter[16]={0},terrain[]={255},clubs[]={7};
 memset(&r,0,sizeof r);r.hud_bases=hud;r.hud_bases_size=6;r.putter_frames=putter;r.putter_frames_size=16;
 r.terrain_sounds=terrain;r.terrain_sounds_size=1;r.club_sounds=clubs;r.club_sounds_size=1;
 for(mode=0;mode<4;mode++)for(power=0;power<256;power++){
  memset(&s,0,sizeof s);memset(&p,0,sizeof p);memset(&tasks,0,sizeof tasks);memset(nodes,0,sizeof nodes);
  memset(&c,0,sizeof c);memset(&tc,0,sizeof tc);memset(&ic,0,sizeof ic);memset(&screen,0,sizeof screen);
  memset(objects,0,sizeof objects);memset(&h,0,sizeof h);memset(&aim,0,sizeof aim);
  for(j=0;j<4;j++){nodes[j].previous=&nodes[(j+3)%4];nodes[j].next=&nodes[(j+1)%4];nodes[j].owner=nodes;nodes[j].priority=2;}
  tasks.current=nodes+1;tasks.free_head=nodes+3;s.animate=callback;p.phase=1;p.hud_base=40;h.hole[0x7d]=(uint8_t)power;
  c.shared=&tc;c.hole=&h;c.aim=&aim;tc.intro=&ic;tc.objects=objects;tc.sound_request=sound;ic.screen=&screen;screen.stamp_bytes=256;
  objects[16].velocity=64;objects[16].flags=64;sound_count=0;aim.club=(uint8_t)(mode&1?13:4);aim.reset_d2=(uint8_t)(mode>>1);
  assert(ntm_shot_trajectory_swing_enter(&s,&p,&tasks,nodes+1,&c,&r,0,0)==0);
  assert(p.last_label==(power>=60 && power<128?3:4));assert(p.counter==1 && s.timer==p.last_label);
  assert(objects[16].velocity==(p.last_label==4?68:64) && objects[16].flags==128);
  for(j=0;j<100 && !nodes[0].result;j++)assert(ntm_shot_trajectory_swing_tick(&s,&p,&tasks,nodes+1,&c,&r,0,0)>=0);
  assert(nodes[0].result==(mode<2?1:255) && s.result==nodes[0].result);
  assert(p.counter==(mode<2?(mode&1?11:10):(mode&1?16:15)));
  assert(sound_count==2 && command==(mode<2?0x107:0x104));
  assert(tasks.free_head==nodes+1 && tasks.current==nodes);
  /* Negative counter rejection must not partially enter the animation. */
  s.phase=0;s.timer=99;p.counter=128;objects[16].velocity=64;nodes[1].callback=0;
  assert(ntm_shot_trajectory_swing_enter(&s,&p,&tasks,nodes+1,&c,&r,0,0)==NTM_TASK_BOUNDARY);
  assert(s.phase==0 && s.timer==99 && objects[16].velocity==64 && !nodes[1].callback);
 }
 puts("trajectory swing: 1024 power/club/accuracy flows, genuine hit/miss lifecycle, sound and atomic boundaries passed");return 0;
}
