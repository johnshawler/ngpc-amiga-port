/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_club.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int tick(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static void put(uint8_t *p,unsigned n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
int main(void)
{
    ntm_shot_club_resources r;ntm_shot_aim_resources math;ntm_shot_scene_object_resources scene;
    ntm_shot_club s;ntm_task t;ntm_shot_scene_object_context c;ntm_tee_setup_context shared;
    ntm_course_intro_context intro;ntm_course_screen screen;ntm_display_reset_state display;
    ntm_hole_prepare_state hole;ntm_shot_player_state player;ntm_shot_aim aim;ntm_course_cursor objects[32];
    uint8_t powers[56],lies[42],overflow[42],minimums[256],scale[256],remainders[10];uint16_t words[2];
    unsigned i,j,k,cases=0;
    memset(&r,0,sizeof r);memset(&math,0,sizeof math);memset(&scene,0,sizeof scene);memset(&c,0,sizeof c);
    memset(&shared,0,sizeof shared);memset(&intro,0,sizeof intro);memset(&hole,0,sizeof hole);memset(&player,0,sizeof player);
    memset(&aim,0,sizeof aim);memset(objects,0,sizeof objects);memset(&display,0,sizeof display);memset(minimums,1,sizeof minimums);
    memset(scale,100,sizeof scale);memset(remainders,0,sizeof remainders);words[0]=7;words[1]=40;
    for(i=0;i<14;i++){put(powers+4*i,400-i*20);put(powers+4*i+2,i);lies[3*i]=2;put(lies+3*i+1,300+i);overflow[3*i]=247;put(overflow+3*i+1,800+i);}
    for(i=0;i<6;i++){math.minimums[i]=minimums;math.minimum_sizes[i]=256;}
    scene.power_scale=scale;scene.power_scale_size=256;scene.remainders=remainders;scene.remainders_size=10;
    r.aim=&math;r.scene=&scene;
    for(i=0;i<12;i++){r.powers[i].data=powers;r.powers[i].size=56;for(j=0;j<15;j++){
        r.lies[i][j].data=lies;r.lies[i][j].size=42;r.overflow_lies[i][j].data=overflow;r.overflow_lies[i][j].size=42;}}
    intro.screen=&screen;intro.display=&display;intro.screen_words=words;shared.intro=&intro;shared.objects=objects;
    c.shared=&shared;c.hole=&hole;c.player=&player;c.aim=&aim;
    for(i=0;i<14;i++)for(j=0;j<15;j++)for(k=0;k<3;k++) {
        unsigned expected=i?i:13;
        memset(&s,0,sizeof s);memset(&t,0,sizeof t);memset(&screen,0,sizeof screen);
        s.wait=tick;s.change_flag=31;aim.club=(uint8_t)i;hole.hole[0x70]=(uint8_t)j;hole.hole[0x77]=255;
        put(hole.hole+0x46,100);put(hole.hole+0x4a,100);screen.stamp_bytes=(uint16_t)(k==0?0:k==1?224:240);
        if(j==2 || j==6 || j==11){if(i==0)expected=12;else if(i==13)expected=1;}
        assert(ntm_shot_club_initialize(&s,&t,&c,&r)==0);
        assert(s.phase==1 && t.callback==tick && s.change_flag==31 && !s.delta && !s.restore_delay && !hole.hole[0x77]);
        assert(aim.club==expected && s.label==expected && s.minimum==1);
        assert(hole.hole[0x7b]==(k==2?247:2));
        assert(screen.stamp_bytes==(k?256:40));
        assert(hole.hole[0x7e]==100 && hole.hole[0x7f]==80 && objects[10].y==48);
        assert(ntm_shot_club_tick(&s,&c)==0);aim.reset_4416=1;assert(ntm_shot_club_tick(&s,&c)==2);aim.reset_4416=0;
        cases++;
    }
    assert(cases==630);assert(ntm_shot_club_initialize(0,&t,&c,&r)==-1);
    puts("shot club:630 authored bounds, queue saturation, early tick and unsupported control cases");return 0;
}
