/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_parent.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int renderer(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static int wait_callback(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static int cancel_callback(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static int accept_callback(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
int main(void)
{
    unsigned priority,result,flags;unsigned long cases=0;
    for(priority=0;priority<256;priority++)for(result=0;result<256;result++)for(flags=0;flags<2;flags++) {
        ntm_task n[5]={{0}},fallback={0};ntm_task_context c={0};ntm_course_parent_state s={0};int request;
        s.wait_callback=wait_callback;s.cancel_delay_callback=cancel_callback;s.accept_delay_callback=accept_callback;
        s.global_word=0xa5a5;s.global_flags=(uint8_t)(result^0x55);s.random_remaining=91;
        n[0].priority=(uint8_t)priority;n[0].next=n+1;n[1].previous=n;n[1].priority=(uint8_t)priority;
        n[2].next=n+3;n[2].result=71;n[3].result=72;c.free_head=n+2;c.current=n;
        assert(ntm_course_parent_initialize(&c,n,renderer,&fallback,&s)==0);
        assert(n[0].result==0 && n[0].callback==wait_callback && !c.free_head);
        assert(n[0].next==n+3 && n[3].next==n+2 && n[2].next==n+1);
        assert(n[3].callback==ntm_course_task && n[2].callback==renderer);
        assert(n[3].owner==n && n[2].owner==n && n[2].result==71 && n[3].result==72);
        n[0].result=(uint8_t)result;n[2].flags=n[3].flags=(uint8_t)flags;
        request=ntm_course_parent_begin(&c,n,&s);
        if(!result){assert(request==0 && s.phase==NTM_PARENT_WAIT && n[0].next==n+3);}
        else if(result==3){assert(request==4 && s.random_remaining==30 && ntm_course_parent_resume(&c,n,&s)==-1);}
        else {
            assert(request==(result==2?2:1));
            if(result==2)assert(n[0].next==n+3); /* Sound precedes cancellation cleanup. */
            assert(ntm_course_parent_begin(&c,n,&s)==-1);
            assert(ntm_course_parent_resume(&c,n,&s)==3);
            assert(n[0].callback==wait_callback);
            assert(ntm_course_parent_resume(&c,n,&s)==0);
            assert(n[0].delay==(result==2?4:29));
            assert(n[0].callback==(result==2?cancel_callback:accept_callback));
            assert(s.global_word==(result==2?0xa5a5:4));
            assert(s.global_flags==(uint8_t)((result^0x55)|(result==2?0:128)));
            assert(ntm_course_parent_resume(&c,n,&s)==-1);
        }
        if(result && priority!=255)assert(n[0].next==n+1);
        if(result && priority==255)assert(n[0].next==n+3); /* Priority wraps, original cleanup leaves nodes. */
        assert(n[0].result==result && c.current==n);cases++;
    }
    printf("PASS native parent: %lu initialization/wait sequences\n",cases);return 0;
}
