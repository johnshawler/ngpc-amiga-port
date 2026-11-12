/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_swing.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int callback(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static unsigned sounds;
static void sound(void *c,uint8_t service,uint16_t command,uint16_t parameter)
{(void)c;assert(service==0 && command==65532 && !parameter);sounds++;}
int main(void)
{
    ntm_shot_swing s;ntm_task t;ntm_shot_aim aim;ntm_tee_setup_context q;
    ntm_shot_scene_object_context scene;ntm_shot_swing_context c;ntm_course_cursor objects[32];
    uint8_t overlay;unsigned v,r,d,cases=0;int result;
    memset(&q,0,sizeof q);memset(&scene,0,sizeof scene);memset(&aim,0,sizeof aim);
    memset(objects,0,sizeof objects);q.objects=objects;q.sound_request=sound;scene.shared=&q;scene.aim=&aim;
    c.scene=&scene;c.overlay=&overlay;
    for(v=0;v<256;v++) {
        memset(&s,0,sizeof s);memset(&t,0,sizeof t);s.waiting=s.hit_delay=s.hit_reset=callback;
        t.result=77;overlay=3;c.suppress_sound=(uint8_t)v;sounds=0;
        assert(!ntm_shot_swing_initialize(&s,&t,&c));
        assert(aim.reset_d1==4 && !t.result && overlay==255 && t.callback==callback && s.phase==1);
        assert(sounds==(v==0));assert(ntm_shot_swing_initialize(&s,&t,&c)==-1);cases++;
    }
    for(r=0;r<256;r++)for(v=0;v<256;v++) {
        s.waiting=s.hit_delay=s.hit_reset=callback;s.phase=1;s.boundary=0;t.callback=callback;t.result=(uint8_t)r;t.delay=999;
        objects[16].flags=(uint8_t)v;result=ntm_shot_swing_tick(&s,&t,&c);
        if(!r)assert(!result && s.phase==1 && t.delay==999 && objects[16].flags==v);
        else if(r>=128)assert(result==2 && s.boundary==2 && objects[16].flags==v && t.delay==999);
        else assert(!result && s.phase==2 && t.delay==3 && objects[16].flags==(v|64));
        cases++;
    }
    for(d=0;d<65536;d++) {
        s.phase=2;s.boundary=0;t.callback=callback;t.delay=(uint16_t)d;
        result=ntm_shot_swing_tick(&s,&t,&c);
        assert(t.delay==(uint16_t)(d-1));
        assert(result==((t.delay&32768)?2:0));
        if(result==2){assert(s.boundary==1 && s.phase==3);assert(ntm_shot_swing_tick(&s,&t,&c)==-1);}
        cases++;
    }
    assert(ntm_shot_swing_initialize(0,&t,&c)==-1);
    printf("shot swing parent:%u authored lifecycle/result/delay cases PASS\n",cases);return 0;
}
