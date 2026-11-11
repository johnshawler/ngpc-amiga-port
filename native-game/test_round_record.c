/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "round_record.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int cb(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static int cb2(ntm_task_context *c,ntm_task *t){return cb(c,t);}
static int cb3(ntm_task_context *c,ntm_task *t){return cb(c,t);}
static int cb4(ntm_task_context *c,ntm_task *t){return cb(c,t);}
static ntm_course_screen screen;static ntm_display_reset_state display;static ntm_course_palette palette;
static ntm_course_cursor cursor;static uint8_t maps[4096],tiles[8192],vp[4],sx[2],sy[2],flags;static uint16_t words[2],x;
static const uint8_t data[]={1,0,0,0,0,0,0,0,1,0,255},tiledata[16]={3},meta[5]={32,0,0,0,3},missing[]={255};
static ntm_cursor_assets objects;
static unsigned events[4],events_used;
static void sound(void *p,uint16_t value){(void)p;events[events_used++]=value;}
static int prepare(void *p){(void)p;events[events_used++]=1;return 0;}
int main(void)
{
    ntm_course_intro_context ic={0};ntm_round_record_context c={0};ntm_round_record_resources r={0};
    ntm_round_record s={0};ntm_round_record_blink blink={0};ntm_round_record_drawing drawing={0};
    ntm_task t={0},owner={0},end={0},child={0},fallback={0};ntm_task_context tc={0};unsigned n,phase;
    ic.screen=&screen;ic.display=&display;ic.palette=&palette;ic.maps=maps;ic.viewport=vp;ic.screen_words=words;ic.scroll_x=sx;ic.scroll_y=sy;ic.flags=&flags;
    c.intro=&ic;c.tasks=&tc;c.fallback=&fallback;c.cursor=&cursor;c.cursor_x=&x;c.tiles=tiles;c.score=255;c.sound_request=sound;c.prepare=prepare;
    r.metadata=meta;r.metadata_size=5;r.missing_score=missing;r.missing_score_size=1;r.object=&objects;
    r.font.data=r.footer.data=data;r.font.size=r.footer.size=sizeof data;r.font.tiles=r.footer.tiles=tiledata;r.font.tiles_size=r.footer.tiles_size=16;
    for(n=0;n<8;n++)palette.slots[n]=(uint16_t)(0xff00+4*n);
    r.palettes[0].kind=r.palettes[1].kind=2;r.palettes[0].offset=r.palettes[1].offset=480;r.palettes[0].pending_index=r.palettes[1].pending_index=0;
    r.palettes[0].control_offset=r.palettes[1].control_offset=0x118;
    s.drawing_entry=cb;s.blink_entry=cb2;s.allocate_blink=cb;s.open=cb2;s.wait=cb3;s.cleanup=cb4;
    t.next=&end;t.previous=&owner;t.priority=1;owner.next=&t;end.previous=&t;tc.current=&t;
    assert(ntm_round_record_initialize(&s,&t,&c,&r)==0);assert(events_used==2 && events[0]==65533 && events[1]==1);
    assert(s.phase==1 && screen.stamp_bytes==0 && tiles[16*0xe0]==3 && maps[0x4c0]==0xe0 && c.player_markers[0]==1);
    assert(ntm_round_record_initialize(&s,&t,&c,&r)==-1);
    assert(ntm_round_record_tick(&s,&t,&c,&r)==0 && s.phase==2);
    assert(ntm_round_record_tick(&s,&t,&c,&r)==0 && s.phase==3 && t.delay==239 && vp[2]==160);
    assert(events_used==4 && events[2]==65535 && events[3]==65534);
    c.prepare=0;c.sound_request=0;
    drawing.finish=cb;t.callback=cb;assert(ntm_round_record_drawing_initialize(&drawing,&t,&c,&r)==0);
    assert(drawing.asset_group==48 && screen.stamp_bytes==24 && maps[0x3c0]==0x90 && maps[0x3c1]==1);
    assert(ntm_round_record_drawing_finish(&drawing,&t,&c,&r)==1 && screen.stamp_bytes==56);
    assert(ntm_round_record_drawing_finish(&drawing,&t,&c,&r)==1 && screen.stamp_bytes==56);
    blink.initial_delay=cb;blink.shown_delay=cb2;blink.hidden_delay=cb3;c.text_mode=1;c.text_base=0x4c0;
    assert(ntm_round_record_blink_initialize(&blink,&t,&c,&r)==0 && t.delay==29 && blink.phase==1);
    for(phase=0;phase<4;phase++)for(n=0;n<65536;n++) {
        unsigned expected=(n-1u)&65535u;memset(&display,0,sizeof display);tc.current=&t;tc.free_head=0;
        t.previous=&owner;t.next=&end;t.priority=1;t.flags=0;owner.next=&t;end.previous=&t;end.priority=0;c.control=0;
        t.delay=(uint16_t)n;
        if(!phase) {
            s.phase=NTM_ROUND_WAIT;t.callback=cb3;
            assert(ntm_round_record_tick(&s,&t,&c,&r)==(expected&32768?1:0));
            assert(t.delay==expected);if(expected&32768)assert(tc.free_head==&t && owner.next==&end && vp[2]==0);
        }else {
            blink.phase=(uint8_t)phase;t.callback=phase==1?cb:phase==2?cb2:cb3;cursor.flags=0;
            assert(ntm_round_record_blink_tick(&blink,&t,&c,&r)==0);
            if(!(expected&32768))assert(t.delay==expected && blink.phase==phase);
            else {assert(t.delay==4 && blink.phase==(phase==2?3:2));if(phase==1)assert(cursor.tag==33 && cursor.tile==0x1d0 && cursor.y==28 && x==80 && display.zero_flags[8]==1);}
        }
    }
    s.phase=3;t.callback=cb3;t.delay=31;c.control=16;t.previous=&owner;t.next=&child;t.priority=1;owner.next=&t;owner.result=37;
    child.previous=&t;child.next=&end;child.priority=2;end.previous=&child;end.priority=0;tc.current=&t;tc.free_head=0;
    assert(ntm_round_record_tick(&s,&t,&c,&r)==1 && t.delay==31 && owner.result==37 && tc.free_head==&t && t.next==&child);
    assert(ntm_round_record_tick(&s,&t,&c,&r)==1);
    for(n=0;n<6;n++){c.game_mode=(uint8_t)(n==0);c.alternate=(uint8_t)(n==1);c.record_mode=(uint8_t)(n==2);c.record_kind=(uint8_t)(n==3);c.score=(uint8_t)(n==4?0:255);c.golfer=(uint8_t)(n==5?6:0);assert(ntm_round_record_tick(&s,&t,&c,&r)==-1);}
    puts("round record: 262144 timer states, direct stamps, scheduled children, ordered preparation, cleanup and unsupported guards OK");return 0;
}
