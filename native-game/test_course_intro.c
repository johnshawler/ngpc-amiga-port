/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_intro.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int cb0(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static int cb1(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static int cb2(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static int cb3(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static const uint16_t counts[]={0,1,32767,32768,65534,65535};
static void records_test(void)
{
    ntm_course_records r,old;unsigned bank,index,value,i;
    for(bank=0;bank<256;bank++)for(index=0;index<6;index++)for(value=0;value<6;value++) {
        memset(&r,0xa5,sizeof r);r.active_player=(uint8_t)bank;r.golfer[bank!=0]=(uint8_t)index;
        for(i=0;i<6;i++)r.count[i]=counts[value];
        old=r;
        assert(ntm_course_records_prepare(&r)==0);
        for(i=0;i<6;i++){assert(r.count[i]==(uint16_t)(old.count[i]+(i==index)));assert(r.active[i]==(i==index?255:0));}
        assert(r.active_player==old.active_player && !memcmp(r.golfer,old.golfer,2));
        r.record_kind[bank!=0]=6;old=r;assert(ntm_course_records_prepare(&r)==1);assert(!memcmp(&r,&old,sizeof r));
        r.record_kind[bank!=0]=0;r.golfer[bank!=0]=6;old=r;assert(ntm_course_records_prepare(&r)==-1);assert(!memcmp(&r,&old,sizeof r));
    }
}
static void owner_test(void)
{
    ntm_task_context c;ntm_task n[4];ntm_course_owner s;ntm_course_records r;unsigned value;
    for(value=0;value<256;value++) {
        memset(&c,0,sizeof c);memset(n,0,sizeof n);memset(&s,0,sizeof s);memset(&r,0,sizeof r);
        n[0].next=n+1;n[0].previous=n+1;n[1].next=n;n[1].previous=n;n[1].priority=1;n[1].result=(uint8_t)value;n[1].callback=cb0;
        c.current=n+1;c.free_head=n+2;s.intro_entry=cb1;s.wait_intro=cb2;
        assert(ntm_course_owner_tick(&c,n+1,&s,&r,n+3)==(value && value!=249?NTM_OWNER_PRIOR_SELECTION:0));
        if(!value){assert(r.count[0]==1 && n[1].next==n+2 && n[2].callback==cb1 && n[1].callback==cb2);}
        else {assert(!r.count[0] && n[1].next==n && c.free_head==n+2 && n[1].callback==cb0);}
        assert(ntm_course_owner_tick(&c,n+1,&s,&r,n+3)==(value && value!=249?NTM_OWNER_PRIOR_SELECTION:0));
        assert(r.count[0]==(!value));
    }
    /* Special path is not silently treated as a normal golfer. */
    s.phase=NTM_OWNER_SELECTOR_WAIT;n[1].result=0;r.record_kind[0]=6;
    assert(ntm_course_owner_tick(&c,n+1,&s,&r,n+3)==NTM_OWNER_SPECIAL_RECORD);
    assert(!r.count[0] && c.free_head==n+2);
    assert(ntm_course_owner_tick(&c,n+1,&s,&r,n+3)==NTM_OWNER_SPECIAL_RECORD);
    assert(ntm_course_owner_special_done(&c,n+1,&s,&r,n+3)==0);
    assert(r.count[0]==1 && c.free_head==0);assert(ntm_course_owner_special_done(&c,n+1,&s,&r,n+3)==-1);
    /* Wait sees depth, not a result byte; no allocation on repeats. */
    assert(ntm_course_owner_tick(&c,n+1,&s,&r,n+3)==0);n[2].priority=1;
    assert(ntm_course_owner_tick(&c,n+1,&s,&r,n+3)==NTM_OWNER_ROUND_RECORD);
    assert(ntm_course_owner_tick(&c,n+1,&s,&r,n+3)==NTM_OWNER_ROUND_RECORD);
    s.phase=NTM_OWNER_SELECTOR_WAIT;n[1].next=n;r.record_kind[0]=0;
    assert(ntm_course_owner_tick(&c,n+1,&s,&r,n+3)==0);assert(n[1].next==n && r.count[0]==2 && n[1].callback==cb2);
}
static ntm_course_screen screen;
static ntm_display_reset_state display;
static ntm_course_palette palette;
static uint8_t maps[4096],tiles[8192],sx[2],sy[2],flags,viewport[4];
static uint16_t words[2];
static ntm_course_intro_context ctx={&screen,&display,&palette,maps,sx,sy,&flags,viewport,words};
static const uint8_t descriptor[]={12,0,40,0},asset_data[]={1,0,0,0,0,0,0,0,129,0,0,255,255},tile_data[16]={255};
static const uint8_t offsets[2]={0},colors[8]={0,1,2,3,4,5,6,7};
static const ntm_course_intro_resources resource={descriptor,4,{asset_data,13,tile_data,16},
    {{1,0,0,1,0,0,offsets,colors,2,8},{2,0,480,0,123,0x118,0,0,0,0}}};
static void scenery_test(void)
{
    ntm_course_intro s;ntm_task t;unsigned i,j,phase,delay,occupancy;
    for(occupancy=0;occupancy<=32;occupancy++) {
        memset(&s,0,sizeof s);memset(&t,0,sizeof t);memset(&screen,0,sizeof screen);memset(&palette,0,sizeof palette);
        memset(&display,0xa5,sizeof display);memset(maps,0xa5,sizeof maps);memset(tiles,0xa5,sizeof tiles);
        memset(sx,5,2);memset(sy,6,2);flags=255;words[0]=42;words[1]=99;
        s.scenery_delay=cb1;s.visible_delay=cb2;s.title=cb3;
        for(i=0;i<8;i++){screen.fill_status[i]=255;palette.slots[i]=(uint16_t)(0xff00+4*i);}
        for(i=0;i<occupancy;i++)assert(ntm_course_screen_enqueue_stamp(&screen,&resource.background,0x800,40)==0);
        if(occupancy>24)for(i=0;i<occupancy-24;i++)assert(ntm_course_palette_enqueue(&palette,resource.palettes)==0);
        assert(ntm_course_screen_enqueue_fill(&screen,0,1,1,0x1234)==0);
        assert(ntm_course_intro_initialize(&s,&t,&ctx,&resource)==0);
        assert(t.delay==1 && t.callback==cb1 && s.phase==NTM_INTRO_SCENERY_DELAY);
        assert(s.asset_group==12 && s.descriptor_next==descriptor+4);
        assert(screen.stamp_bytes==(occupancy==32?256:8*(occupancy+1)));
        assert(flags==63 && !words[0] && !words[1] && !sx[0] && !sx[1] && !sy[0] && !sy[1]);
        for(i=0;i<4096;i++)assert(maps[i]==(i/64<19 && i%64<40?0:0xa5));
        for(i=0;i<8192;i++)assert(tiles[i]==0xa5);
        assert(display.object_tags[0]==65535 && !display.zero_flags[8]);
        assert(ntm_course_intro_initialize(&s,&t,&ctx,&resource)==-1);
        assert(ntm_course_palette_tail(&palette)==0);assert(ntm_course_palette_head(&palette)==0);
        assert(ntm_course_screen_fill_queue(&screen,maps)==0);assert(maps[0]==0x34 && maps[1]==0x12);
        assert(ntm_course_screen_stamp_queue(&screen,tiles,maps)==0);assert(maps[0x800]==40 && tiles[40*16]==255);
        assert(ntm_course_intro_tick(&s,&t,&ctx)==0 && t.delay==0);
        assert(ntm_course_intro_tick(&s,&t,&ctx)==0 && t.delay==19 && s.phase==NTM_INTRO_VISIBLE_DELAY);
        assert(viewport[0]==0 && viewport[1]==0 && viewport[2]==160 && viewport[3]==152);
        for(j=0;j<19;j++)assert(ntm_course_intro_tick(&s,&t,&ctx)==0);
        assert(ntm_course_intro_tick(&s,&t,&ctx)==NTM_INTRO_TITLE && t.callback==cb3);
        assert(ntm_course_intro_tick(&s,&t,&ctx)==NTM_INTRO_TITLE && t.delay==65535);
    }
    /* Exhaust all 16-bit input patterns for both signed-delay callbacks. */
    for(phase=1;phase<=2;phase++)for(delay=0;delay<65536;delay++) {
        s.phase=(uint8_t)phase;t.callback=phase==1?cb1:cb2;t.delay=(uint16_t)delay;
        i=(delay-1u)&65535u;j=ntm_course_intro_tick(&s,&t,&ctx);
        if(!(i&32768)){assert(!j && t.delay==i && s.phase==phase);}
        else if(phase==1){assert(!j && t.delay==19 && s.phase==NTM_INTRO_VISIBLE_DELAY);}
        else {assert(j==NTM_INTRO_TITLE && t.delay==i && s.phase==NTM_INTRO_TITLE_PENDING);}
    }
}
/* Actual traversal: owner is visited before selector exits; new child only
 * runs on next owner visit, later in THAT traversal. */
typedef struct {ntm_task_context c;ntm_task n[5];ntm_course_owner s;ntm_course_records r;unsigned child_calls;} schedule;
static int entry(ntm_task_context *c,ntm_task *t){schedule *a=(schedule *)c;(void)t;a->child_calls++;return 0;}
static int owner(ntm_task_context *c,ntm_task *t){schedule *a=(schedule *)c;return ntm_course_owner_tick(c,t,&a->s,&a->r,a->n+4);}
static int exit_selector(ntm_task_context *c,ntm_task *t){t->owner->result=0;return ntm_task_finish(c,t);}
static void schedule_test(void)
{
    schedule a;memset(&a,0,sizeof a);a.n[0].next=a.n+1;a.n[0].previous=a.n+2;
    a.n[1].next=a.n+2;a.n[1].previous=a.n;a.n[1].priority=1;a.n[1].result=249;a.n[1].callback=owner;
    a.n[2].next=a.n;a.n[2].previous=a.n+1;a.n[2].priority=2;a.n[2].owner=a.n+1;a.n[2].callback=exit_selector;
    a.c.current=a.n+1;a.c.free_head=a.n+3;a.s.intro_entry=entry;a.s.wait_intro=owner;
    assert(ntm_task_dispatch_one(&a.c)==0);assert(ntm_task_dispatch_one(&a.c)==NTM_TASK_RECYCLED);
    assert(a.c.current==a.n && !a.child_calls && !a.r.count[0]);
    a.c.current=a.n[0].next;assert(ntm_task_dispatch_one(&a.c)==0);assert(a.c.current==a.n+2);
    assert(ntm_task_dispatch_one(&a.c)==0);assert(a.child_calls==1 && a.r.count[0]==1 && a.c.current==a.n);
}
int main(void){records_test();owner_test();scenery_test();schedule_test();puts("course intro: record/owner/queue/traversal tests and 131072 signed-delay cases passed");return 0;}
