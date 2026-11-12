/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_miss.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int wait_callback(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static int next_callback(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
int main(void)
{
 ntm_shot_miss s;ntm_task task;ntm_shot_scene_object_context c;ntm_tee_setup_context tc;
 ntm_course_intro_context ic;ntm_course_cursor objects[32];ntm_hole_prepare_state h;
 ntm_course_screen q;ntm_shot_miss_resources r;uint16_t x[32],words[2];uint8_t flag;unsigned value,n;
 memset(&s,0,sizeof s);memset(&task,0,sizeof task);memset(&c,0,sizeof c);memset(&tc,0,sizeof tc);
 memset(&ic,0,sizeof ic);memset(objects,0,sizeof objects);memset(&h,0,sizeof h);memset(&q,0,sizeof q);memset(&r,0,sizeof r);memset(x,0,sizeof x);memset(words,0,sizeof words);
 c.shared=&tc;c.hole=&h;tc.intro=&ic;tc.objects=objects;tc.x=x;ic.screen=&q;ic.screen_words=words;
 for(value=0;value<65536;value++){
  x[5]=(uint16_t)value;objects[5].byte15=3;words[0]=65520;
  assert(ntm_shot_miss_object_gate(&c,38,68)==0);
  assert(objects[5].byte15==((uint16_t)(value-65520u-38u)<=68?2:3));
  s.phase=1;s.delay=wait_callback;s.continuation=next_callback;task.callback=wait_callback;task.delay=(uint16_t)value;
  assert(ntm_shot_miss_tick(&s,&task)==(((uint16_t)(value-1u)&32768u)?2:0));
  assert(task.delay==(uint16_t)(value-1u));
 }
 s.phase=0;flag=0;h.hole[0x70]=7;h.hole[0x71]=2;task.result=19;task.owner=0;
 assert(ntm_shot_miss_initialize(&s,&task,&c,&flag,&r)==0);
 assert(flag==255 && h.hole[0x71]==7 && task.delay==29 && q.stamp_bytes==8);
 for(n=0;n<29;n++)assert(ntm_shot_miss_tick(&s,&task)==0);
 assert(task.delay==0 && ntm_shot_miss_tick(&s,&task)==2 && task.callback==next_callback);
 assert(task.result==19 && !task.owner);
 puts("shot miss:65536 wrapped gate/timer cases, actual31-call wait, notice queue, untouched result passed");return 0;
}
