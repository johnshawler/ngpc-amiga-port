/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "shot_accuracy.h"
static int wait_accuracy(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static void task_setup(ntm_task n[3],ntm_task_context *c,unsigned mode)
{
    memset(n,0,sizeof(*n)*3);memset(c,0,sizeof(*c));
    n[1].previous=n;n[1].next=n+2;n[0].next=n+1;n[2].previous=n+1;c->current=n+1;
    if(mode==0)n[2].priority=1;
    if(mode==1)n[1].flags=1;
}
int main(void)
{
    ntm_shot_accuracy s;ntm_task n[3];ntm_task_context tasks;ntm_shot_scene_object_context c;
    ntm_hole_prepare_state h;ntm_shot_player_state p;ntm_shot_aim aim;
    ntm_tee_setup_context shared;ntm_course_intro_context intro;ntm_course_cursor objects[32];
    uint16_t words[2]={0,42};unsigned club,band,state,held,golfer,modifier,lie,mode;unsigned long count=0;
    memset(&c,0,sizeof c);memset(&h,0,sizeof h);memset(&p,0,sizeof p);memset(&aim,0,sizeof aim);
    memset(&shared,0,sizeof shared);memset(&intro,0,sizeof intro);memset(objects,0,sizeof objects);
    c.hole=&h;c.player=&p;c.aim=&aim;c.shared=&shared;shared.intro=&intro;shared.objects=objects;intro.screen_words=words;
    for(club=0;club<256;club++)for(band=0;band<256;band++) {
        memset(&s,0,sizeof s);s.wait=wait_accuracy;aim.club=(uint8_t)club;h.hole[0x79]=77;h.hole[0x7a]=193;h.hole[0x7b]=(uint8_t)band;
        assert(!ntm_shot_accuracy_initialize(&s,n,&c));
        assert(s.position==0x1300 && s.velocity==(club==13?0:49485) && s.upper==(uint8_t)(38-band) && s.lower==band && s.phase==1 && n[0].callback==wait_accuracy);count++;
    }
    for(state=0;state<256;state++)if(state!=4) {
        s.position=0x24f0;s.velocity=0x123;
        assert(!ntm_shot_accuracy_tick(&s,&c,(uint8_t)state,0,0,0,0));
        assert(s.position==0x2513 && s.velocity==0xfedd && objects[7].y==156);count++;
        s.position=0x110;s.velocity=0xffa0;
        assert(!ntm_shot_accuracy_tick(&s,&c,(uint8_t)state,0,0,0,0));
        assert(s.position==0x1b0 && s.velocity==0x60 && objects[7].y==192);count++;
    }
    aim.club=0;s.upper=35;s.lower=3;
    for(golfer=0;golfer<6;golfer++)for(modifier=0;modifier<2;modifier++)for(lie=0;lie<16;lie++)for(held=0;held<256;held++) {
        int accepted=lie==0 || lie==4 || (modifier?(lie==1 || lie==2):(golfer!=1 && (lie==1 || (golfer==4 && lie==2))));
        h.players[0][1]=(uint8_t)golfer;h.players[0][0x33]=(uint8_t)(modifier*6);h.hole[0x70]=(uint8_t)lie;h.hole[0x7c]=99;
        s.position=19*256;mode=held%3;task_setup(n,&tasks,mode);
        assert(ntm_shot_accuracy_tick(&s,&c,4,10,(uint8_t)held,&tasks,n+1)==(int)mode+1);
        assert(h.hole[0x78]==19 && h.hole[0x7c]==(accepted?(held&3):99));count++;
    }
    for(band=0;band<256;band++) {
        s.position=(uint16_t)(band<<8);h.hole[0x7d]=100;h.hole[0x7e]=104;
        aim.reset_d2=aim.reset_d3=0;task_setup(n,&tasks,2);
        assert(ntm_shot_accuracy_tick(&s,&c,4,9,0,&tasks,n+1)==3);
        assert(aim.reset_d2==(band>=128 || band<3?2:band>35?1:0));
        assert(aim.reset_d3==(band>=18 && band<=20));count++;
    }
    puts("accuracy: exact initializer, fractional bounce, lifecycle/lie/input classification passed");
    printf("accuracy authored cases: %lu\n",count);return 0;
}
