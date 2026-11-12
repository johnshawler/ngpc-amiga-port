/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_swing_exit.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int callback(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
int main(void)
{
    ntm_shot_swing_exit s;ntm_shot_scene_object_context c;ntm_tee_setup_context q;
    ntm_course_intro_context i;ntm_shot_aim aim;ntm_display_reset_state display;
    ntm_course_cursor objects[32];ntm_task n[4];ntm_task_context tasks;
    uint8_t view[4];uint16_t words[2];unsigned flags,kind,cases=0;int r;
    for(flags=0;flags<256;flags++)for(kind=0;kind<4;kind++) {
        memset(&s,0,sizeof s);memset(&c,0,sizeof c);memset(&q,0,sizeof q);memset(&i,0,sizeof i);
        memset(&aim,0,sizeof aim);memset(&tasks,0,sizeof tasks);memset(n,0,sizeof n);
        memset(&display,0x5a,sizeof display);memset(objects,0x5a,sizeof objects);memset(view,3,sizeof view);
        words[0]=0x1234;words[1]=0x5678;
        s.perfect_entry=s.perfect_wait=s.miss_entry=s.miss_wait=callback;
        n[0].callback=callback;n[0].next=n+1;n[0].previous=n+1;n[0].priority=40;n[0].flags=(uint8_t)flags;
        n[1].next=n;n[1].previous=n;n[1].priority=40;
        tasks.current=n;tasks.free_head=n+2;n[2].next=0;
        c.shared=&q;c.aim=&aim;q.intro=&i;q.tasks=&tasks;q.fallback=n+3;q.objects=objects;
        i.display=&display;i.viewport=view;i.screen_words=words;
        if(kind==0) {
            r=ntm_shot_swing_exit_hit(&s,n,&c);
            assert(r==((flags&1)?NTM_TASK_UNLINKED:NTM_TASK_RECYCLED));
            assert(s.phase==3 && tasks.current==n+1 && n[1].next==n+1);
            assert(view[0]==0 && view[1]==0 && view[2]==0 && view[3]==152);
            assert(words[0]==0x1200 && words[1]==0x5600 && objects[31].id==31 && objects[31].tag==65535);
        }else if(kind==1) {
            aim.reset_d3=1;r=ntm_shot_swing_exit_hit(&s,n,&c);
            assert(!r && s.phase==1 && n[0].next==n+2 && n[2].owner==n && n[2].priority==41);
            assert(!ntm_shot_swing_exit_tick(&s,n,&c));assert(s.phase==1); /* Child still live. */
        }else if(kind==2) {
            r=ntm_shot_swing_exit_miss(&s,n,&c);assert(!r && s.phase==2);
            assert(n[0].next==n+2 && n[2].owner==n && display.capacity==0x5a);
            assert(!ntm_shot_swing_exit_tick(&s,n,&c));assert(s.phase==2 && !aim.reset_e6);
        }else {
            s.phase=2;aim.reset_e6=255;r=ntm_shot_swing_exit_tick(&s,n,&c);
            assert(r==((flags&1)?NTM_TASK_UNLINKED:NTM_TASK_RECYCLED) && s.phase==3);
        }
        cases++;
    }
    assert(ntm_shot_swing_exit_hit(0,n,&c)==-1);
    printf("swing exit:%u authored reset/allocation/wait/lifecycle cases PASS\n",cases);return 0;
}
