/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Authored scheduler contracts; exact live meter/children use the original
 * application oracle. No test callback stands in for a successful child. */
#include "shot_fire_application.c"
#include "shot_power_application.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static ntm_shot_power_application a;
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
    memset(&a,0,sizeof a);o=power_opening(&a);d=power_driver(&a);tee=&o->base.base.base;s=&tee->base.selector;
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
    unsigned i,cases=0;ntm_course_driver *d;ntm_task *t;ntm_shot_power_application_resources bad;
    assert(ntm_shot_power_application_enable(0,&resources)==-1);
    assert(ntm_shot_power_application_update(0,0)==-1);
    for(i=0;i<3;i++){setup();bad=resources;if(i==0)bad.fire=0;else if(i==1)bad.meter=0;else power_driver(&a)->updates=1;assert(ntm_shot_power_application_enable(&a,&bad)==-1);assert(!a.enabled);cases++;}
    setup();assert(!ntm_shot_power_application_enable(&a,&resources));assert(a.enabled && a.base.enabled);
    assert(a.base.fire.ready==power_fire_ready && a.power.fire_closed==power_fire_closed && a.meter.wait==power_meter_tick);
    assert(ntm_shot_power_application_enable(&a,&resources)==-1);cases++;
    /* Exhausted pool is the actual allocation outcome, not a child-success
     * stub. Check full scheduler head/tail and the same-call parent delay. */
    setup();assert(!ntm_shot_power_application_enable(&a,&resources));t=start(power_fire_ready);d=power_driver(&a);a.base.fire.phase=3;
    assert(!ntm_shot_power_application_update(&a,16));assert(a.power.phase==1 && t->delay==3 && t->callback==power_delay_tick);
    assert(event_count==3 && events[0]==1 && events[1]==2 && events[2]==4);cases++;
    for(i=0;i<4;i++)assert(!ntm_shot_power_application_update(&a,0));
    assert(a.power.phase==2 && t->callback==power_ready_tick);
    assert(!ntm_shot_power_application_update(&a,48));assert(a.power.phase==6 && t->callback==power_cancel_restore);
    assert(!ntm_shot_power_application_update(&a,0));assert(a.power.phase==7);
    assert(!ntm_shot_power_application_update(&a,0));assert(!a.power.phase && a.base.fire.phase==1 && t->delay==3);
    for(i=0;i<8;i++)assert(!ntm_shot_power_application_update(&a,0));
    assert(a.base.fire.phase==3 && t->callback==power_fire_ready);cases++;
    /* First-fire cancel removes real descendant task and delays rebuilding. */
    t->next=d->nodes+2;d->nodes[2].previous=t;d->nodes[2].next=d->nodes;d->nodes[0].previous=d->nodes+2;
    d->nodes[2].priority=2;d->nodes[2].callback=observed;
    event_count=0;assert(!ntm_shot_power_application_update(&a,32));assert(a.power.phase==5 && t->callback==power_cancel_rebuild);
    assert(t->next==d->nodes && d->context.free_head==d->nodes+2 && event_count==3);
    event_count=0;assert(ntm_shot_power_application_update(&a,0)==1);assert(a.boundary==NTM_POWER_APPLICATION_WORLD);
    assert(event_count==2 && d->context.current==t && d->context.published==t && !d->running);
    assert(ntm_shot_power_application_update(&a,0)==-1);cases++;
    /* Accuracy allocation and dispatch are real. Its oscillator runs while
     * parent delays, then the third edge stops before swing and frame tail. */
    setup();assert(!ntm_shot_power_application_enable(&a,&resources));t=start(power_ready_tick);d=power_driver(&a);
    a.power.phase=2;d->context.free_head=d->nodes+2;d->nodes[2].next=0;d->nodes[2].result=87;
    assert(!ntm_shot_power_application_update(&a,16));
    assert(t->callback==power_accuracy_delay && t->delay==3 && t->next==d->nodes+2);
    assert(d->nodes[2].callback==power_accuracy_tick && d->nodes[2].owner==t && d->nodes[2].result==87);
    assert(a.accuracy.phase==1 && a.accuracy.position==0x1300 && event_count==3);
    for(i=0;i<4;i++)assert(!ntm_shot_power_application_update(&a,0));
    assert(t->callback==power_accuracy_ready && a.power.phase==4);
    event_count=0;assert(ntm_shot_power_application_update(&a,16)==1 && a.boundary==NTM_POWER_APPLICATION_SWING);
    assert(d->context.current==t && d->context.published==t && event_count==2);
    assert(ntm_shot_power_application_update(&a,0)==-1);cases++;
    printf("shot power application: %u enable, scheduler, cancellation and boundary cases PASS\n",cases);return 0;
}
