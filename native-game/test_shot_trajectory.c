/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_trajectory.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void)
{
 ntm_shot_trajectory s;ntm_task t;ntm_shot_scene_object_context c;
 ntm_shot_trajectory_resources r;ntm_tee_setup_context q;ntm_course_intro_context i;
 ntm_course_cursor objects[32];ntm_display_reset_state d;ntm_course_palette palette;
 ntm_course_screen screen;ntm_hole_prepare_state hole;ntm_shot_player_state player;
 ntm_shot_aim aim;uint16_t x[32],words[2]={65530,65530};unsigned label,k;
 static const uint8_t metadata[]={60,0,64,0,3},bases[]={16,80,48,96,32,64},hud[]={25,157,91,190,58,124};
 memset(&s,0,sizeof s);memset(&t,0,sizeof t);memset(&r,0,sizeof r);memset(&q,0,sizeof q);memset(&i,0,sizeof i);memset(objects,0,sizeof objects);memset(&d,0,sizeof d);memset(&palette,0,sizeof palette);memset(&screen,0,sizeof screen);memset(&hole,0,sizeof hole);memset(&player,0,sizeof player);memset(&aim,0,sizeof aim);memset(x,0,sizeof x);
 for(k=0;k<32;k++)objects[k].id=d.object_ids[k]=(uint8_t)k;
 /* Full palette queue deliberately drops the resolved request. */
 for(k=0;k<8;k++)palette.slots[k]=(uint16_t)(0x400+4*k);
 for(k=0;k<6;k++)r.palettes[k].kind=2,r.palettes[k].control_offset=0x12;
 r.metadata=metadata;r.metadata_size=5;r.sprite_bases=bases;r.sprite_bases_size=6;r.hud_bases=hud;r.hud_bases_size=6;
 i.display=&d;i.palette=&palette;i.screen=&screen;i.screen_words=words;q.intro=&i;q.objects=objects;q.x=x;
 c.shared=&q;c.hole=&hole;c.player=&player;c.aim=&aim;c.saved_map=0;
 s.wait=(ntm_task_callback)1;screen.stamp_bytes=256;aim.reset_d1=1;
 assert(ntm_shot_trajectory_initialize(&s,&t,&c,&r,0)==0);
 assert(s.phase==1 && s.sprite_base==64 && s.hud_base==100 && s.counter==0);
 assert(x[16]==34 && objects[16].y==82 && objects[16].tag==27 && objects[16].tile==84);
 assert(d.zero_flags[8]==1 && objects[16].flags==192);
 for(label=0;label<256;label++){
  s.last_label=(uint8_t)(label+1);assert(ntm_shot_trajectory_tick(&s,&c,&r,(uint8_t)label)==0);
  k=(label^128u)<=137u?0:(label^128u)<=148u?1:2;
  assert(objects[16].velocity==64+8*k && s.hud_delta==60*k && s.last_label==label);
 }
 aim.reset_d1=4;s.last_label=0;objects[16].velocity=73;
 assert(ntm_shot_trajectory_tick(&s,&c,&r,21)==2 && objects[16].velocity==73 && s.last_label==0);
 puts("shot trajectory: initialization, all labels, queue drops and lifecycle4 boundary passed");return 0;
}
