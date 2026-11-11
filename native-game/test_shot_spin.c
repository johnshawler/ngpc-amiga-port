/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "shot_spin.h"
static int wait_spin(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static unsigned sounds;
static void sound(void *p,uint8_t service,uint16_t command,uint16_t parameter)
{(void)p;assert(service==2 && command==1 && !parameter);sounds++;}
static void control_test(void)
{
    ntm_shot_spin s;ntm_hole_prepare_state h;ntm_shot_scene_object_context c;
    ntm_shot_spin_draw_resources r;ntm_tee_setup_context shared;ntm_course_intro_context intro;
    ntm_course_screen screen;uint8_t maps[4096],saved[32],tiles[8192],art[11]={1,0,0,0,0,0,0,0,1,0,255},pixels[16];
    unsigned spin,pressed,timer,j;
    memset(&s,0,sizeof s);memset(&h,0,sizeof h);memset(&c,0,sizeof c);memset(&r,0,sizeof r);
    memset(&shared,0,sizeof shared);memset(&intro,0,sizeof intro);memset(pixels,0xa5,sizeof pixels);
    c.hole=&h;c.shared=&shared;c.saved_map=saved;shared.intro=&intro;shared.sound_request=sound;intro.screen=&screen;intro.maps=maps;
    s.phase=1;s.minimum=0;s.maximum=8;
    for(j=0;j<32;j++)saved[j]=(uint8_t)(j*7);
    for(j=0;j<9;j++) {r.indicators[j].data=art;r.indicators[j].size=sizeof art;r.indicators[j].tiles=pixels;r.indicators[j].tiles_size=sizeof pixels;r.highlights[j]=r.indicators[j];}
    for(spin=0;spin<9;spin++)for(pressed=0;pressed<256;pressed++)for(timer=0;timer<3;timer++) {
        unsigned edge=pressed&12u,changed=(edge==4 && spin>0)||(edge==8 && spin<8);
        unsigned expected=changed?(edge==4?spin-1:spin+1):spin;
        memset(&screen,0,sizeof screen);memset(maps,0xcc,sizeof maps);sounds=0;s.current=(uint8_t)spin;
        h.hole[0x76]=(uint8_t)spin;h.hole[0x77]=(uint8_t)timer;
        assert(!ntm_shot_spin_control(&s,&c,&r,1,0,(uint8_t)pressed,0,0));
        assert(h.hole[0x76]==expected && s.current==expected && sounds==changed);
        assert(h.hole[0x77]==(changed?31:timer?timer-1:0));
        assert(screen.stamp_bytes==(changed?16:0));
        if(!changed && timer==1)for(j=0;j<32;j++)assert(maps[0x10e +64*(j/8)+j%8]==saved[j]);
        assert(!ntm_course_screen_stamp_queue(&screen,tiles,maps));
    }
}
int main(void)
{
    uint8_t bounds[24];ntm_shot_spin_resources r={bounds,sizeof bounds};
    ntm_hole_prepare_state h;ntm_shot_player_state p;ntm_shot_spin s;
    ntm_task n[3];ntm_task_context tasks;unsigned i,state,gate,pressed;
    memset(&h,0,sizeof h);memset(&p,0,sizeof p);
    for(i=0;i<24;i++)bounds[i]=(uint8_t)i;
    for(i=0;i<12;i++) {
        memset(&s,0,sizeof s);memset(n,0,sizeof n);s.wait=wait_spin;
        p.active_player=(uint8_t)(i%2);h.players[p.active_player][1]=(uint8_t)(i%6);
        h.players[p.active_player][0x33]=(uint8_t)(i>=6?6:0);h.hole[0x76]=77;h.hole[0x77]=99;
        assert(!ntm_shot_spin_initialize(&s,n,&h,&p,&r));
        assert(s.minimum==2*i && s.maximum==2*i+1 && s.current==77 && s.phase==1);
        assert(!h.hole[0x77] && n[0].callback==wait_spin);
    }
    for(state=0;state<256;state++)if(state!=3)for(gate=0;gate<2;gate++)for(pressed=0;pressed<256;pressed++) {
        int boundary=state!=2 && !gate && ((pressed&12)==4 || (pressed&12)==8);
        assert(ntm_shot_spin_tick(&s,&h,(uint8_t)state,(uint8_t)gate,(uint8_t)pressed,0,0)==(boundary?NTM_TASK_BOUNDARY:0));
    }
    h.hole[0x77]=1;assert(ntm_shot_spin_tick(&s,&h,1,1,0,0,0)==NTM_TASK_BOUNDARY);
    h.hole[0x77]=0;h.hole[0x76]++;assert(ntm_shot_spin_tick(&s,&h,1,1,0,0,0)==NTM_TASK_BOUNDARY);
    for(i=0;i<3;i++) {
        memset(n,0,sizeof n);memset(&tasks,0,sizeof tasks);
        n[1].previous=&n[0];n[1].next=&n[2];n[0].next=&n[1];n[2].previous=&n[1];tasks.current=&n[1];
        if(i==0)n[2].priority=1;
        if(i==1)n[1].flags=1;
        assert(ntm_shot_spin_tick(&s,&h,3,1,0,&tasks,&n[1])==(i==0?NTM_TASK_DEFERRED:i==1?NTM_TASK_UNLINKED:NTM_TASK_RECYCLED));
    }
    control_test();
    puts("shot spin: bounds, input idle boundaries, lifecycle finish passed");return 0;
}
