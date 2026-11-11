/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_parent.h"
#include "render_bridge.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define CB(name) static int name(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
CB(render_cb) CB(wait_cb) CB(cancel_cb) CB(accept_cb) CB(random_cb) CB(cycle_cb) CB(fade_cb) CB(setup_cb) CB(cancel_exit_cb) CB(accept_exit_cb)
static uint8_t tiles[8192],maps[4096],sprites[256],pal[64],bitmap[24320];
int main(void)
{
    unsigned seed,i,j,mode,slot;unsigned long cases=0;
    for(seed=0;seed<256;seed++) {
        ntm_display_reset_state d;uint8_t table[256],viewport[4];ntm_task task={0};
        memset(&d,(int)seed,sizeof d);assert(ntm_display_reset(&d)==0);
        for(i=0;i<16;i++)assert(d.zero_flags[i]==0);
        for(i=0;i<32;i++)assert(d.object_ids[i]==i && d.object_tags[i]==65535 && !d.list[i]);
        for(i=0;i<256;i++){assert(!d.pools[0][i] && !d.pools[1][i]);table[i]=(uint8_t)(i*71+seed);}
        assert(!d.zero_words[0] && !d.zero_words[1] && d.capacity==64 && d.limit==80);
        assert(d.reset_words[0]==0x974c54f9u && d.reset_words[1]==0x4b4c5bf9u);
        for(i=0;i<256;i++) {
            task.substate=(uint8_t)i;viewport[1]=(uint8_t)seed;viewport[3]=(uint8_t)(255-seed);
            assert(ntm_viewport_fade(&task,viewport,table)==((uint8_t)(i+2)<14));
            assert(task.substate==(uint8_t)(i+2) && viewport[0]==table[i^128] && viewport[2]==table[((i+1)&255)^128]);
            assert(viewport[1]==seed && viewport[3]==255-seed);cases++;
        }
    }
    for(mode=0;mode<256;mode++)for(slot=0;slot<9;slot++)for(seed=0;seed<16;seed++) {
        ntm_service54_state a,b;uint8_t payload[9];unsigned index;
        memset(&a,(int)seed,sizeof a);a.mode=(uint8_t)mode;
        for(i=0;i<8;i++)a.slots[i]=(uint16_t)((i>=slot?0x8000:0x2000)|(((i+seed)&7)*4));
        for(i=0;i<9;i++)payload[i]=(uint8_t)(seed*17+i*31);
        b=a;
        if(slot<8) {
            ntm_service54_record *r;index=(b.slots[slot]&255)/4;r=b.records+index;b.slots[slot]=(uint16_t)(0x2000|index*4);r->command=84;b.status=65535;
            if(mode<16){r->code=84;r->value8=(uint16_t)(payload[6]+256u*payload[7]);r->value12=(uint16_t)((r->value12/256)*256+payload[8]);}
            else {r->code=32;r->value8=(uint16_t)(480+payload[0]);r->value12=(uint16_t)(payload[1]+256u*payload[2]);r->value14=(uint16_t)(payload[3]+256u*payload[4]);r->value26=payload[5];}
        }
        assert(ntm_service54(&a,payload)==(slot==8));assert(!memcmp(&a,&b,sizeof a));cases++;
    }
    for(i=0;i<3;i++){uint8_t index=255,order[3]={255,255,255};assert(!ntm_course_setup((uint8_t)i,&index,order));assert(!index && order[0]==i && order[1]==(i+1)%3 && order[2]==(i+2)%3);}
    /* Bridge and planar output: transparent layers expose exact window geometry. */
    for(i=0;i<256;i++) {
        nr_scene s={0};nr_bitmap b={bitmap,sizeof bitmap,160,152,20};uint8_t row[160],view[4]={(uint8_t)i,(uint8_t)(i/3),(uint8_t)(255-i),(uint8_t)(255-i/2)};
        unsigned x,y,plane;s.tiles=tiles;s.maps=maps;s.sprites=sprites;s.sprite_pal=pal;s.background=3;s.outside=5;
        assert(ntm_scene_set_viewport(&s,view)==0);
        for(y=0;y<152;y++) {
            assert(!nr_scene_row(&s,y,row));assert(!nr_store_scene_row_packed(&b,y,row));
            for(x=0;x<160;x++) {
                unsigned pen=(x>=view[0] && x<(unsigned)view[0]+view[2] && y>=view[1] && y<(unsigned)view[1]+view[3])?243:253;
                assert(row[x]==pen);
                for(plane=0;plane<8;plane++)assert(((bitmap[plane*3040+y*20+x/8]>>(7-x%8))&1)==((pen>>plane)&1));
            }
        }
    }
    for(seed=0;seed<256;seed++)for(mode=1;mode<4;mode++)for(slot=0;slot<2;slot++) {
        ntm_task n[5]={{0}};ntm_task_context c={0};ntm_course_parent_state s={0};
        ntm_service54_state queue={0};ntm_display_reset_state display;uint8_t table[256],payload[9],cursor=(uint8_t)seed;unsigned calls=0;int request;
        for(i=0;i<256;i++)table[i]=(uint8_t)(i*73+seed);
        for(i=0;i<9;i++)payload[i]=(uint8_t)(i*31+seed);
        for(i=0;i<8;i++)queue.slots[i]=(uint16_t)((slot?0x2000:0x8000)|i*4);
        queue.mode=(uint8_t)seed;queue.status=0x1234;
        n[0].next=n+1;n[0].previous=n+1;n[0].owner=n+4;n[0].priority=7;n[0].flags=(uint8_t)(seed&1);n[4].result=99;
        n[1].previous=n;n[1].next=n;n[1].priority=7;n[2].next=n+3;c.free_head=n+2;c.current=n;c.selection=1;
        s.wait_callback=wait_cb;s.cancel_delay_callback=cancel_cb;s.accept_delay_callback=accept_cb;s.random_delay_callback=random_cb;
        s.random_cycle_callback=cycle_cb;s.fade_callback=fade_cb;s.setup_callback=setup_cb;s.cancel_exit_callback=cancel_exit_cb;s.accept_exit_callback=accept_exit_cb;
        s.viewport[1]=5;s.viewport[3]=147;
        assert(!ntm_course_parent_initialize(&c,n,render_cb,n+4,&s));n[0].result=(uint8_t)mode;
        do {
            request=calls?ntm_course_parent_tick(&c,n,&s):ntm_course_parent_begin(&c,n,&s);
            while(request>0) {
                if(request==NTM_PARENT_RANDOM)request=ntm_course_parent_generate_random(&c,n,&s,&cursor,table);
                else if(request==NTM_PARENT_FADE)request=ntm_course_parent_fade(&c,n,&s,table);
                else if(request==NTM_PARENT_COURSE_SETUP)request=ntm_course_parent_setup(&c,n,&s);
                else if(request==NTM_PARENT_SERVICE54)request=ntm_course_parent_service54(&c,n,&s,&queue,payload);
                else if(request==NTM_PARENT_EXIT)request=ntm_course_parent_exit(&c,n,&s,&display);
                else {assert(request==NTM_PARENT_RENDER || request==NTM_PARENT_SOUND_ZERO || request==NTM_PARENT_SOUND_ONE || request==NTM_PARENT_SOUND_TWO);request=ntm_course_parent_resume(&c,n,&s);}
            }
            assert(request==0);calls++;assert(calls<200);
        }while(s.phase!=NTM_PARENT_DONE);
        assert(calls==(mode==1?32u:mode==2?13u:92u) && n[4].result==(mode==2?252:0));
        assert(c.current==n+1 && n[1].next==n+1 && n[1].previous==n+1);
        assert(c.free_head==(seed&1?n+2:n) && s.finish_outcome==(seed&1?NTM_TASK_UNLINKED:NTM_TASK_RECYCLED));
        assert(queue.status==((mode!=2 && !slot)?65535:0x1234));
        if(mode==2)assert(n[0].substate==14 && s.viewport[1]==5 && s.viewport[3]==147);
        else assert(!s.viewport[0] && !s.viewport[1] && !s.viewport[2] && s.viewport[3]==152);
        assert(ntm_course_parent_exit(&c,n,&s,&display)==-1);
    }
    assert(ntm_display_reset(0)==-1 && ntm_scene_set_viewport(0,tiles)==-1);
    for(j=3;j<256;j++){uint8_t index=91,order[3]={1,2,3};assert(ntm_course_setup((uint8_t)j,&index,order)==-1 && index==91 && order[0]==1);}
    printf("PASS native services: %lu fade/queue cases and 256 planar fixtures plus 1536 complete parent exits\n",cases);return 0;
}
