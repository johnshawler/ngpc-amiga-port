/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_title.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int cb(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static int cb2(ntm_task_context *c,ntm_task *t){return cb(c,t);}
static int cb3(ntm_task_context *c,ntm_task *t){return cb(c,t);}
static int cb4(ntm_task_context *c,ntm_task *t){return cb(c,t);}
static ntm_course_screen screen;static ntm_display_reset_state display;
static ntm_course_cursor cursor;static uint8_t maps[4096],viewport[4];static uint16_t words[2],x;
static const uint8_t desc[12]={8,0,1,0,2,0,3,0,24,0,4,0},meta[5]={20,0,0,0,7};
int main(void)
{
    ntm_course_intro_context ic={0};ntm_course_title_context c={0};ntm_course_title_resources r={0};
    ntm_course_title s;ntm_course_intro intro={0};ntm_task t={0},owner={0},end={0},child={0};ntm_task_context tc={0};
    unsigned n,phase;uint8_t linked[2];
    ic.screen=&screen;ic.display=&display;ic.maps=maps;ic.viewport=viewport;ic.screen_words=words;
    c.intro=&ic;c.cursor=&cursor;c.cursor_x=&x;c.tasks=&tc;
    r.descriptor=desc;r.descriptor_size=12;r.metadata=meta;r.metadata_size=5;
    intro.phase=NTM_INTRO_TITLE_PENDING;intro.title=cb;intro.descriptor_next=desc+4;intro.asset_group=8;
    memset(&s,0,sizeof s);s.title_wait=cb;s.tournament_delay=cb2;s.tournament_wait=cb3;s.cleanup=cb4;t.callback=cb;
    assert(ntm_display_reset(&display)==0);assert(ntm_course_title_initialize(&s,&t,&c,&r,&intro)==0);
    assert(s.phase==NTM_TITLE_WAIT && t.delay==59 && x==40 && cursor.y==48 && cursor.width==24 && cursor.tag==38);
    assert(display.list[0]==0xff00 && display.zero_words[0]==2 && display.zero_flags[8]==1);
    assert(screen.stamps[0].offset==0x188 && screen.stamps[0].tile==2);
    assert(ntm_course_title_initialize(&s,&t,&c,&r,&intro)==-1);
    for(n=0;n<65536;n++){
        linked[0]=(uint8_t)n;linked[1]=(uint8_t)(n>>8);
        assert(ntm_course_title_skip(0,1,linked)==!!((linked[0]|linked[1])&16));
        assert(ntm_course_title_skip((uint8_t)n,0,0)==!!(n&16));
    }
    assert(ntm_course_title_skip(0,1,0)==-1);
    for(phase=NTM_TITLE_WAIT;phase<=NTM_TITLE_TOURNAMENT_WAIT;phase++)for(n=0;n<65536;n++) {
        unsigned expected=(n-1u)&65535u;memset(maps,0xa5,sizeof maps);screen.stamp_bytes=0;c.control=0;
        s.phase=(uint8_t)phase;t.callback=phase==NTM_TITLE_WAIT?cb:phase==NTM_TITLE_TOURNAMENT_DELAY?cb2:cb3;t.delay=(uint16_t)n;
        t.previous=&owner;t.next=&end;t.priority=1;t.flags=0;owner.next=&t;end.previous=&t;end.priority=0;tc.current=&t;tc.free_head=0;
        assert(ntm_course_title_tick(&s,&t,&c,&r)==((expected&32768)&&phase==NTM_TITLE_TOURNAMENT_WAIT?NTM_TITLE_COMPLETE:NTM_TITLE_NONE));
        if(!(expected&32768))assert(t.delay==expected && s.phase==phase);
        else if(phase==NTM_TITLE_WAIT){assert(t.delay==1 && s.phase==NTM_TITLE_TOURNAMENT_DELAY);assert(maps[0x188]==0 && maps[0x187]==0xa5 && maps[0x1a0]==0xa5);assert(screen.stamps[0].offset==0x180);}
        else if(phase==NTM_TITLE_TOURNAMENT_DELAY)assert(t.delay==59 && s.phase==NTM_TITLE_TOURNAMENT_WAIT && viewport[2]==160);
        else assert(s.phase==NTM_TITLE_DONE && viewport[2]==0 && tc.free_head==&t && tc.current==&owner && owner.next==&end);
    }
    /* Skip preserves timer, removes real descendants, preserves owner result. */
    s.phase=NTM_TITLE_TOURNAMENT_WAIT;t.callback=cb3;t.delay=27;c.control=16;t.previous=&owner;t.next=&child;t.priority=1;owner.next=&t;owner.result=37;
    child.previous=&t;child.next=&end;child.priority=2;child.flags=0;end.previous=&child;end.priority=0;tc.current=&t;tc.free_head=0;
    assert(ntm_course_title_tick(&s,&t,&c,&r)==NTM_TITLE_COMPLETE);assert(t.delay==27 && owner.result==37 && tc.free_head==&t && t.next==&child);
    assert(ntm_course_title_tick(&s,&t,&c,&r)==NTM_TITLE_COMPLETE);
    puts("course title: 196608 timer states, 65536 linked inputs, registration and lifecycle OK");return 0;
}
