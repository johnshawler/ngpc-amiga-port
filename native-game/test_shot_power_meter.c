/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "shot_power_meter.h"
static int wait_meter(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
int main(void)
{
    ntm_shot_power_meter s;ntm_shot_power_meter_resources r;ntm_shot_scene_object_context c;
    ntm_hole_prepare_state h;ntm_shot_player_state p;ntm_shot_aim aim;ntm_tee_setup_context shared;
    ntm_course_intro_context intro;ntm_course_cursor objects[32];ntm_task nodes[3];ntm_task_context tasks;
    uint8_t speed[257],scale[256],remainders[10],tiles[8192],maps[4096],header[10]={2,0,0,0,0,0,0,0,16,0},pixels[32];
    uint16_t words[2]={0,50};unsigned i,k,lifecycle;uint32_t count=0;
    memset(&r,0,sizeof r);memset(&c,0,sizeof c);memset(&h,0,sizeof h);memset(&p,0,sizeof p);
    memset(&aim,0,sizeof aim);memset(&shared,0,sizeof shared);memset(&intro,0,sizeof intro);memset(objects,0,sizeof objects);
    memset(speed,0,sizeof speed);for(i=0;i<256;i++)scale[i]=(uint8_t)((i^128u)%121);
    for(i=0;i<10;i++)remainders[i]=(uint8_t)(i*8/10);
    for(i=0;i<32;i++)pixels[i]=(uint8_t)(3*i);
    r.speed_player=r.speed_lie=speed;r.speed_player_size=r.speed_lie_size=257;
    r.scale.power_scale=scale;r.scale.power_scale_size=256;r.scale.remainders=remainders;r.scale.remainders_size=10;
    for(i=0;i<9;i++){r.glyphs[i].data=header;r.glyphs[i].size=10;r.glyphs[i].tiles=pixels;r.glyphs[i].tiles_size=32;}
    c.shared=&shared;c.hole=&h;c.player=&p;c.aim=&aim;shared.intro=&intro;shared.tiles=tiles;shared.objects=objects;
    intro.maps=maps;intro.screen_words=words;
    memset(&s,0,sizeof s);memset(nodes,0,sizeof nodes);s.wait=wait_meter;h.players[0][0x23]=4;
    assert(!ntm_shot_power_meter_initialize(&s,nodes,&c,&r));assert(s.position==256 && s.upper==66 && s.lower==1 && s.hold==1 && s.phase==1 && nodes[0].callback==wait_meter);
    for(i=0;i<9;i++)assert(!memcmp(tiles+0xe40+32*i,pixels,32));
    for(i=0;i<121;i++)for(k=0;k<256;k++) {
        memset(maps,0xcc,sizeof maps);s.position=2560;s.velocity=0;s.lower=1;s.upper=66;s.hold=(uint8_t)k;h.hole[0x7d]=(uint8_t)i;
        assert(!ntm_shot_power_meter_tick(&s,&c,&r,2,0,0));
        assert(s.hold==(k==1?1:(uint8_t)(k-1)));
        assert(h.hole[0x7d]==(k==1?10:i));
        assert(objects[10].y==(uint16_t)(88-h.hole[0x7f]+50));count++;
    }
    h.players[0][1]=2;s.position=0x3f80;s.velocity=256;s.upper=64;s.lower=1;s.hold=1;
    assert(!ntm_shot_power_meter_advance(&s,&h,&p,&r));assert(s.position==0x4000 && s.velocity==65280 && s.hold==4);
    for(lifecycle=0;lifecycle<256;lifecycle++)if(lifecycle!=2)for(i=0;i<3;i++) {
        memset(nodes,0,sizeof nodes);memset(&tasks,0,sizeof tasks);
        nodes[1].previous=nodes;nodes[1].next=nodes+2;nodes[0].next=nodes+1;nodes[2].previous=nodes+1;tasks.current=nodes+1;
        if(i==0)nodes[2].priority=1;
        if(i==1)nodes[1].flags=1;
        assert(ntm_shot_power_meter_tick(&s,&c,0,(uint8_t)lifecycle,&tasks,nodes+1)==(int)i+1);count++;
    }
    /* Unsupported corrupt visible power does not partially update scratch. */
    s.position=2560;s.velocity=0;s.hold=2;h.hole[0x7d]=255;
    assert(ntm_shot_power_meter_tick(&s,&c,&r,2,0,0)==NTM_TASK_BOUNDARY);assert(s.hold==2 && h.hole[0x7d]==255);
    printf("power meter: %lu rendering/hold/lifecycle cases, glyphs, oscillator and atomic boundary passed\n",(unsigned long)count);return 0;
}
