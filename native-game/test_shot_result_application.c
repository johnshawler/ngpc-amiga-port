/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Authored scheduler contracts; exact live meter/children use the original
 * application oracle. No test callback stands in for a successful child. */
#include "shot_fire_application.c"
#include "shot_power_application.c"
#include "shot_swing_application.c"
#include "shot_result_application.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static ntm_shot_result_application result_whole;
#define whole (result_whole.base)
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
    memset(&result_whole,0,sizeof result_whole);o=power_opening(&a);d=power_driver(&a);tee=&o->base.base.base;s=&tee->base.selector;
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
    ntm_shot_swing_application_resources swing={&resources,&trajectory_swing};
    ntm_shot_miss_resources miss;ntm_shot_perfect_resources perfect;
    ntm_shot_result_application_resources r={&swing,&miss,&perfect};
    ntm_task *t,*child;ntm_course_driver *d;ntm_shot_result_application_resources bad;unsigned j;
    memset(&trajectory_swing,0,sizeof trajectory_swing);memset(&miss,0,sizeof miss);memset(&perfect,0,sizeof perfect);
    assert(ntm_shot_result_application_enable(0,&r)==-1);
    assert(ntm_shot_result_application_update(0,0)==-1);
    for(j=0;j<3;j++){
        setup();bad=r;if(j==0)bad.swing=0;else if(j==1)bad.miss=0;else bad.perfect=0;
        assert(ntm_shot_result_application_enable(&result_whole,&bad)==-1 && !result_whole.enabled);
    }
    setup();assert(!ntm_shot_result_application_enable(&result_whole,&r));
    assert(result_whole.enabled && whole.enabled && a.enabled);
    assert(whole.swing.waiting==result_parent_wait && result_whole.perfect.finish==result_perfect_finish);
    assert(ntm_shot_result_application_enable(&result_whole,&r)==-1);
    /* Actual allocation and dispatch to an honest unconverted-child boundary.
     * No test callback stands in for completed flight or a successful child. */
    t=start(result_owner_wait);d=power_driver(&a);d->context.current=t;
    child=d->nodes+2;child->next=0;d->context.free_head=child;
    assert(!result_owner_wait(&d->context,t));
    assert(t->callback==result_flight_wait && t->next==child && child->owner==t);
    assert(child->callback==result_flight_entry && !d->context.free_head);
    assert(!result_flight_wait(&d->context,t));
    d->context.current=child;assert(ntm_task_dispatch_one(&d->context)==NTM_TASK_BOUNDARY);
    assert(result_whole.boundary==NTM_RESULT_FLIGHT_ENTRY && d->context.current==child);
    /* Exhausted pool keeps original fallback untouched; next-stage boundary
     * remains explicit instead of claiming the missing child ran. */
    setup();assert(!ntm_shot_result_application_enable(&result_whole,&r));
    t=start(result_owner_wait);d=power_driver(&a);d->context.current=t;
    assert(!result_owner_wait(&d->context,t));assert(t->next==d->nodes);
    assert(result_flight_wait(&d->context,t)==NTM_TASK_BOUNDARY);
    assert(result_whole.boundary==NTM_RESULT_NEXT_STAGE);
    setup();assert(!ntm_shot_result_application_enable(&result_whole,&r));start(observed);
    assert(!ntm_shot_result_application_update(&result_whole,0));
    puts("result application authored enable/allocation/boundary contracts PASS");return 0;
}
