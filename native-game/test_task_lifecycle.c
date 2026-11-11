/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "task_lifecycle.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned sound_count;
static void sound(void *p,uint8_t value){assert(p==&sound_count && value==2);sound_count++;}
static int idle(ntm_task_context *c,ntm_task *t){assert(c->current==t);return 0;}
static void setup(ntm_task *n,ntm_task_context *c,uint8_t *controls)
{
    unsigned i;memset(n,0,5*sizeof *n);memset(c,0,sizeof *c);memset(controls,0,16);
    for(i=0;i<3;i++){n[i].previous=n+(i+2)%3;n[i].next=n+(i+1)%3;n[i].callback=idle;}
    c->current=n+1;c->free_head=n+3;c->controls=controls;c->selection=1;
    c->sound_request=sound;c->sound_context=&sound_count;n[1].owner=n+4;n[4].result=165;
}
int main(void)
{
    ntm_task n[5];ntm_task_context c;uint8_t controls[16];unsigned a,b,f;unsigned long count=0;
    for(a=0;a<256;a++)for(b=0;b<256;b++)for(f=0;f<2;f++) {
        int result;setup(n,&c,controls);n[1].priority=(uint8_t)a;n[2].priority=(uint8_t)b;n[1].flags=(uint8_t)(254|f);
        n[1].callback=ntm_task_finish;result=ntm_task_dispatch_one(&c);count++;
        assert(c.published==n+1 && c.current==n+2 && n[1].previous==n && n[1].owner==n+4);
        assert(n[1].priority==a && n[1].flags==(254|f) && n[4].result==165);
        if(a<b) {
            assert(result==1 && n[0].next==n+1 && n[2].previous==n+1 && c.free_head==n+3 && n[1].next==n+2);
        }else {
            assert(result==(f?2:3) && n[0].next==n+2 && n[2].previous==n);
            assert(c.free_head==(f?n+3:n+1) && n[1].next==(f?n+2:n+3));
        }
    }
    /* Actual input→course→defer→remove, with no second result write/sound. */
    setup(n,&c,controls);n[1].callback=ntm_course_task;n[2].priority=1;
    ntm_input_update(controls,8);assert(ntm_task_dispatch_one(&c)==0);
    assert(c.selection==2 && sound_count==1 && c.current==n+2);
    c.current=n+1;ntm_input_update(controls,16);assert(ntm_task_dispatch_one(&c)==1);
    assert(n[4].result==1 && n[1].callback==ntm_task_finish);
    c.current=n+1;n[2].priority=0;ntm_input_update(controls,32);
    assert(ntm_task_dispatch_one(&c)==3 && n[4].result==1 && sound_count==1 && c.current==n+2);
    /* A two-node circular list aliases previous and next. */
    setup(n,&c,controls);n[1].previous=n+2;n[1].next=n+2;n[2].previous=n+1;n[2].next=n+1;
    assert(ntm_task_finish(&c,n+1)==3 && n[2].next==n+2 && n[2].previous==n+2 && c.current==n+2);
    assert(ntm_task_finish(0,n)==-1 && ntm_task_dispatch_one(0)==-1 && ntm_course_task(0,n)==-1);
    printf("PASS native lifecycle: %lu priority/flag dispatch cases plus composed sequence\n",count);return 0;
}
