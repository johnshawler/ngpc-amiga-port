/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_perfect.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int callback(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
int main(void)
{
 unsigned k,j,calls;int result;ntm_shot_perfect s;ntm_task nodes[4];ntm_task_context tasks;
 ntm_shot_scene_object_context c;ntm_tee_setup_context tc;ntm_course_intro_context ic;ntm_course_screen q;
 ntm_course_palette p;ntm_shot_perfect_resources r;ntm_hole_prepare_state h;ntm_shot_player_state player;
 uint8_t viewport[4],sx[2],sy[2];
 for(k=0;k<1024;k++){
  memset(&s,0,sizeof s);memset(nodes,0,sizeof nodes);memset(&tasks,0,sizeof tasks);memset(&c,0,sizeof c);memset(&tc,0,sizeof tc);
  memset(&ic,0,sizeof ic);memset(&q,0,sizeof q);memset(&p,0,sizeof p);memset(&r,0,sizeof r);memset(&h,0,sizeof h);memset(&player,0,sizeof player);
  memset(viewport,255,sizeof viewport);memset(sx,255,sizeof sx);memset(sy,255,sizeof sy);
  for(j=0;j<4;j++){nodes[j].previous=nodes+(j+3)%4;nodes[j].next=nodes+(j+1)%4;nodes[j].owner=nodes;nodes[j].priority=2;nodes[j].result=71;}
  nodes[1].flags=(uint8_t)(k%3==1);nodes[2].priority=(uint8_t)(k%3==0?3:2);tasks.current=nodes+1;tasks.free_head=nodes+3;
  s.first_delay=s.visible_delay=s.finish=callback;
  for(j=0;j<8;j++)p.slots[j]=(uint16_t)(4*j+(j<k%9?0x2000:0xff00));
  for(j=0;j<2;j++){r.palettes[j].kind=2;r.palettes[j].control_offset=0x12;}
  c.shared=&tc;c.hole=&h;c.player=&player;tc.intro=&ic;tc.tasks=&tasks;ic.screen=&q;ic.palette=&p;ic.viewport=viewport;ic.scroll_x=sx;ic.scroll_y=sy;
  h.players[0][1]=(uint8_t)(k%6);q.stamp_bytes=256;
  assert(ntm_shot_perfect_initialize(&s,nodes+1,&c,&r)==0);calls=1;
  assert(s.kind==0 && s.duration==30 && nodes[1].delay==1 && viewport[2]==0 && viewport[3]==152);
  do {result=ntm_shot_perfect_tick(&s,nodes+1,&c);assert(result>=0);calls++;assert(calls<=33);}while(s.phase!=3);
  assert(calls==33 && result==(k%3==0?1:k%3==1?2:3));
  assert(viewport[0]==0 && viewport[1]==0 && viewport[2]==160 && viewport[3]==152);
  assert(!sx[0] && !sx[1] && !sy[0] && !sy[1] && nodes[0].result==71);
 }
 puts("shot perfect:1024 real33-dispatch notices, palette saturation, viewport and lifecycle outcomes passed");return 0;
}
