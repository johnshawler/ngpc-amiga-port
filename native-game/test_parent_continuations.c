/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_parent.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define CB(name) static int name(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
CB(wait_cb) CB(cancel_cb) CB(accept_cb) CB(random_cb) CB(cycle_cb) CB(fade_cb) CB(setup_cb)
static void setup(ntm_task_context *c,ntm_task *n,ntm_course_parent_state *s)
{
    memset(c,0,sizeof *c);memset(n,0,2*sizeof *n);memset(s,0,sizeof *s);
    c->current=n;c->selection=1;n[0].next=n+1;n[1].previous=n;n[1].next=n;n[0].previous=n+1;
    n[0].callback=wait_cb;n[0].substate=165;s->mapped_course=204;s->random_remaining=2;
    s->wait_callback=wait_cb;s->cancel_delay_callback=cancel_cb;s->accept_delay_callback=accept_cb;
    s->random_delay_callback=random_cb;s->random_cycle_callback=cycle_cb;s->fade_callback=fade_cb;s->setup_callback=setup_cb;
}
int main(void)
{
    ntm_task_context c;ntm_task n[2];ntm_course_parent_state s;unsigned kind,timer,i;unsigned long count=0;
    for(kind=0;kind<4;kind++)for(timer=0;timer<65536;timer++) {
        unsigned dec=(timer-1)&65535;int request;
        setup(&c,n,&s);n[0].delay=(uint16_t)timer;
        n[0].callback=kind==0?cancel_cb:kind==1?accept_cb:kind==2?random_cb:fade_cb;
        s.phase=kind<2?NTM_PARENT_DELAY:kind==2?NTM_PARENT_RANDOM_DELAY:NTM_PARENT_FADE_DELAY;
        request=ntm_course_parent_tick(&c,n,&s);
        if(dec<32768){assert(request==0 && n[0].delay==dec && n[0].substate==165);}
        else if(kind==0){assert(request==0 && n[0].delay==0 && n[0].substate==0 && n[0].callback==fade_cb);}
        else if(kind==1){assert(request==NTM_PARENT_COURSE_SETUP && n[0].delay==dec && s.mapped_course==0 && n[0].callback==setup_cb);}
        else if(kind==2){assert(request==NTM_PARENT_RANDOM && s.random_remaining==1 && n[0].callback==cycle_cb);}
        else {assert(request==NTM_PARENT_FADE && n[0].delay==dec && n[0].substate==165);}
        count++;
    }
    for(i=0;i<256;i++) {
        int request;setup(&c,n,&s);n[0].callback=random_cb;s.phase=NTM_PARENT_RANDOM_DELAY;s.random_remaining=(uint8_t)i;
        request=ntm_course_parent_tick(&c,n,&s);
        assert(s.random_remaining==(uint8_t)(i-1) && request==(i==1?NTM_PARENT_SOUND_ZERO:NTM_PARENT_RANDOM));
        setup(&c,n,&s);s.phase=NTM_PARENT_RANDOM_PENDING;
        assert(ntm_course_parent_supply_random(&c,n,&s,(uint8_t)i)==NTM_PARENT_RENDER && c.selection==i%3);
        assert(ntm_course_parent_supply_random(&c,n,&s,0)==-1);
        assert(ntm_course_parent_resume(&c,n,&s)==NTM_PARENT_SOUND_TWO);
        assert(ntm_course_parent_resume(&c,n,&s)==0 && n[0].delay==1 && n[0].callback==random_cb);
    }
    {
        unsigned calls=0,draws=0,renders=0,sounds=0;int request=0;
        setup(&c,n,&s);n[0].result=3;
        do {
            request=calls?ntm_course_parent_tick(&c,n,&s):ntm_course_parent_begin(&c,n,&s);
            while(request && request!=NTM_PARENT_COURSE_SETUP) {
                assert(request>0);
                if(request==NTM_PARENT_RANDOM){draws++;request=ntm_course_parent_supply_random(&c,n,&s,(uint8_t)calls);}
                else {if(request==NTM_PARENT_RENDER)renders++;else {assert(request==NTM_PARENT_SOUND_ZERO || request==NTM_PARENT_SOUND_TWO);sounds++;}request=ntm_course_parent_resume(&c,n,&s);}
            }
            calls++;assert(calls<200);
        }while(request!=NTM_PARENT_COURSE_SETUP);
        assert(calls==91 && draws==30 && renders==31 && sounds==31 && s.random_remaining==0);
        assert(ntm_course_parent_tick(&c,n,&s)==-1 && ntm_course_parent_resume(&c,n,&s)==-1);
    }
    {
        uint8_t window[256],cursor;unsigned seed,j;
        for(seed=0;seed<256;seed++) {
            for(j=0;j<256;j++)window[j]=(uint8_t)(j*73+seed);
            cursor=(uint8_t)seed;
            for(j=0;j<256;j++){unsigned old=cursor;assert(ntm_random_next(&cursor,window)==window[old^128]);assert(cursor==(uint8_t)(old+1));}
            setup(&c,n,&s);s.phase=NTM_PARENT_RANDOM_PENDING;cursor=(uint8_t)seed;
            assert(ntm_course_parent_generate_random(&c,n,&s,&cursor,window)==NTM_PARENT_RENDER);
            assert(cursor==(uint8_t)(seed+1) && c.selection==window[seed^128]%3);
            assert(ntm_course_parent_generate_random(&c,n,&s,&cursor,window)==-1 && cursor==(uint8_t)(seed+1));
        }
        assert(ntm_random_next(0,window)==-1 && ntm_random_next(&cursor,0)==-1);
    }
    printf("PASS native parent continuations: %lu timer patterns, byte boundaries and 91-call random flow\n",count);return 0;
}
