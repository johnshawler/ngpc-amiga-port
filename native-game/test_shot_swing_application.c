/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Authored scheduler contracts; exact live meter/children use the original
 * application oracle. No test callback stands in for a successful child. */
#include "shot_fire_application.c"
#include "shot_power_application.c"
#include "shot_swing_application.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static ntm_shot_swing_application whole;
#define a (whole.base)
static ntm_shot_open_resources open_resources;
static ntm_shot_fire_resources parent;
static ntm_shot_spin_resources spin;
static ntm_shot_spin_draw_resources spin_draw;
static ntm_shot_trajectory_resources trajectory;
static ntm_shot_golfer_resources golfer;
static ntm_shot_golfer_drawing_resources drawing;
static ntm_terrain_resources terrain;
static ntm_tee_object_assets objects;
static ntm_shot_fire_application_resources fire_resources={&parent,&spin,&spin_draw,&trajectory,&golfer,&drawing,&terrain,1,&objects,&objects,&objects};
static ntm_shot_power_meter_resources meter;
static ntm_shot_power_application_resources resources={&fire_resources,&meter};
static unsigned events[64],event_count;
static void event(unsigned n){assert(event_count<64);events[event_count++]=n;}
static int pump(void *p){(void)p;event(1);return 0;}
static int head(void *p){(void)p;event(2);return 0;}
static int tail(void *p){(void)p;event(4);return 0;}
static int observed(ntm_task_context *c,ntm_task *t){(void)c;(void)t;event(3);return 0;}
static void setup(void)
{
    ntm_shot_open_application *o;ntm_course_driver *d;ntm_course_application *s;ntm_tee_setup_application *tee;unsigned i;
    memset(&whole,0,sizeof whole);o=power_opening(&a);d=power_driver(&a);tee=&o->base.base.base;s=&tee->base.selector;
    o->resources=&open_resources;o->scene_context.shared=&tee->context;o->scene_context.hole=&tee->hole;
    o->scene_context.aim=&o->base.aim;o->scene_context.saved_map=o->saved_map;tee->context.intro=&tee->base.intro_context;
    tee->context.tasks=&d->context;tee->context.fallback=&tee->base.fallback;tee->context.objects=tee->objects;
    tee->base.intro_context.viewport=d->parent.viewport;tee->base.intro_context.display=&s->display;
    tee->base.intro_context.palette=&s->palette;tee->base.intro_context.maps=s->maps;tee->base.intro_context.flags=&s->initialization.flags;
    tee->base.intro_context.screen_words=s->initialization.screen_words;
    for(i=0;i<32;i++)tee->objects[i].id=s->display.object_ids[i]=(uint8_t)i;
    for(i=0;i<8;i++){s->palette.slots[i]=(uint16_t)(65280+4*i);s->screen.fill_status[i]=255;}
    parent.palette.kind=2;parent.palette.control_offset=0x118;
    d->running=1;d->context.free_head=d->nodes+2;d->nodes[2].next=d->nodes+3;
    d->context.controls=s->controls;d->screen=&s->screen;d->tiles=s->tiles;d->maps=s->maps;
    d->service_pump=pump;d->drawing_head=head;d->service_tail=tail;event_count=0;
}
static ntm_task *start(ntm_task_callback callback)
{
    ntm_course_driver *d=power_driver(&a);ntm_task *t=d->nodes+1;
    d->pre_task=0;d->context.free_head=0;d->nodes[0].next=d->nodes[0].previous=t;
    t->next=t->previous=d->nodes;t->callback=callback;t->priority=1;return t;
}
int main(void)
{
    ntm_shot_trajectory_swing_resources trajectory_swing;
    ntm_shot_swing_application_resources r={&resources,&trajectory_swing};
    ntm_task *t;ntm_course_driver *d;unsigned i;
    memset(&trajectory_swing,0,sizeof trajectory_swing);
    assert(ntm_shot_swing_application_enable(0,&r)==-1);
    assert(ntm_shot_swing_application_update(0,0)==-1);
    setup();assert(!ntm_shot_swing_application_enable(&whole,&r));
    assert(whole.enabled && a.enabled && a.base.enabled);
    assert(a.power.accuracy_delay==swing_power_delay && a.power.accuracy_ready==swing_power_ready);
    assert(a.base.trajectory.wait==swing_trajectory_wait);
    assert(ntm_shot_swing_application_enable(&whole,&r)==-1);
    t=start(swing_power_ready);d=power_driver(&a);a.power.phase=4;
    assert(!ntm_shot_swing_application_update(&whole,16));
    assert(whole.swing.phase==1 && t->callback==swing_parent_wait && t->result==0);
    assert(swing_open(&whole)->base.aim.reset_d1==4 && swing_open(&whole)->input_dispatch==255);
    for(i=0;i<5;i++)assert(!ntm_shot_swing_application_update(&whole,0));
    assert(t->result==0 && whole.swing.phase==1 && d->running);
    /* Boundary contracts only: injected result is NOT gameplay evidence.
     * Genuine child publication is checked by the original application oracle. */
    t->result=1;assert(!ntm_shot_swing_application_update(&whole,0));
    assert(t->delay==3 && whole.swing.phase==2);
    for(i=0;i<3;i++)assert(!ntm_shot_swing_application_update(&whole,0));
    event_count=0;assert(ntm_shot_swing_application_update(&whole,0)==1);
    assert(whole.boundary==NTM_SWING_APPLICATION_HIT_RESET && !d->running);
    assert(event_count==2 && events[0]==1 && events[1]==2);
    assert(ntm_shot_swing_application_update(&whole,0)==-1);
    setup();assert(!ntm_shot_swing_application_enable(&whole,&r));
    t=start(swing_parent_wait);whole.swing.phase=1;t->result=255;
    assert(ntm_shot_swing_application_update(&whole,0)==1);
    assert(whole.boundary==NTM_SWING_APPLICATION_MISS_CHILD);
    /* Ensure inherited observer remains available for ordinary live tasks. */
    setup();assert(!ntm_shot_swing_application_enable(&whole,&r));start(observed);
    assert(!ntm_shot_swing_application_update(&whole,0));
    puts("swing application authored scheduler/boundary contracts PASS");return 0;
}
