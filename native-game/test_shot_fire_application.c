/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Authored contract/scheduler fixture. Asset-exact child execution belongs to
 * the unchanged-original application oracle, not these test callbacks. */
#include "shot_fire_application.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static ntm_shot_fire_application a;
static ntm_shot_open_resources open_resources;
static ntm_shot_fire_resources parent;
static ntm_shot_spin_resources spin;
static ntm_shot_spin_draw_resources spin_draw;
static ntm_shot_trajectory_resources trajectory;
static ntm_shot_golfer_resources golfer;
static ntm_shot_golfer_drawing_resources drawing;
static ntm_terrain_resources terrain;
static ntm_tee_object_assets objects;
static ntm_shot_fire_application_resources resources={&parent,&spin,&spin_draw,&trajectory,&golfer,&drawing,&terrain,1,&objects,&objects,&objects};
static unsigned calls,events[64],event_count;
static void event(unsigned n){assert(event_count<64);events[event_count++]=n;}
static int pump(void *p){(void)p;event(1);return 0;}
static int head(void *p){(void)p;event(2);return 0;}
static int tail(void *p){(void)p;event(4);return 0;}
static int callback(ntm_task_context *c,ntm_task *t){(void)c;(void)t;event(3);calls++;return 0;}
static void setup(void)
{
    ntm_shot_open_application *o;ntm_course_driver *d;ntm_course_application *s;ntm_tee_setup_application *tee;unsigned i;
    memset(&a,0,sizeof a);o=opening(&a);d=driver(&a);tee=&o->base.base.base;s=&tee->base.selector;
    o->resources=&open_resources;o->scene_context.shared=&tee->context;o->scene_context.hole=&tee->hole;
    o->scene_context.aim=&o->base.aim;tee->context.intro=&tee->base.intro_context;
    tee->context.tasks=&d->context;tee->context.fallback=&tee->base.fallback;tee->context.objects=tee->objects;
    tee->base.intro_context.viewport=d->parent.viewport;tee->base.intro_context.display=&s->display;
    tee->base.intro_context.palette=&s->palette;tee->base.intro_context.maps=s->maps;tee->base.intro_context.flags=&s->initialization.flags;
    for(i=0;i<32;i++)tee->objects[i].id=s->display.object_ids[i]=(uint8_t)i;
    for(i=0;i<8;i++){s->palette.slots[i]=(uint16_t)(65280+4*i);s->screen.fill_status[i]=255;}
    parent.palette.kind=2;parent.palette.control_offset=0x118;
    d->running=1;d->context.free_head=d->nodes+2;d->nodes[2].next=d->nodes+3;
    d->context.controls=s->controls;d->screen=&s->screen;d->tiles=s->tiles;d->maps=s->maps;
    d->service_pump=pump;d->drawing_head=head;d->service_tail=tail;
    calls=event_count=0;
}
int main(void)
{
    unsigned i,n,boundary,cases=0;ntm_course_driver *d;ntm_task nodes[13];ntm_shot_fire_application_resources bad;
    assert(ntm_shot_fire_application_enable(0,&resources)==-1);
    assert(ntm_shot_fire_application_update(0,0)==-1);
    for(i=0;i<12;i++){
        setup();bad=resources;
        switch(i){case 0:bad.parent=0;break;case 1:bad.spin=0;break;case 2:bad.spin_draw=0;break;
        case 3:bad.trajectory=0;break;case 4:bad.golfer=0;break;case 5:bad.drawing=0;break;
        case 6:bad.terrain=0;break;case 7:bad.hole_count=0;break;case 8:bad.trajectory_objects=0;break;
        case 9:bad.pin_objects=0;break;case 10:bad.green_objects=0;break;case 11:driver(&a)->updates=1;break;}
        assert(ntm_shot_fire_application_enable(&a,&bad)==-1);assert(!a.enabled && !a.base.enabled);cases++;
    }
    setup();d=driver(&a);assert(!ntm_shot_fire_application_enable(&a,&resources));
    assert(a.enabled && a.base.enabled && d->nodes[3].next==a.extra_tasks);
    assert(a.extra_tasks[0].next==a.extra_tasks+1 && a.extra_tasks[1].next==a.extra_tasks+2 && !a.extra_tasks[2].next);
    assert(ntm_shot_fire_application_enable(&a,&resources)==-1);
    /* Real parent allocations preserve exhaustion, fallback, allocation order,
     * caller scratch, and the initializer's already-performed decrement. */
    for(n=0;n<=3;n++){
        ntm_task *t;setup();d=driver(&a);assert(!ntm_shot_fire_application_enable(&a,&resources));
        memset(nodes,0,sizeof nodes);t=nodes;t->next=nodes+1;t->previous=nodes+1;nodes[1].previous=t;nodes[1].next=t;
        t->callback=fire_pre_ready;t->priority=2;d->context.controls[1]=16;
        d->context.free_head=n?nodes+2:0;
        for(i=0;i<n;i++){nodes[i+2].next=i+1<n?nodes+i+3:0;nodes[i+2].result=91;nodes[i+2].delay=0x1234;}
        assert(!fire_gate(&d->context,t));assert(a.fire.phase==1 && t->delay==3 && t->callback==fire_closed);
        assert(!d->context.free_head && a.fire_update==1 && opening(&a)->input_dispatch==255);
        if(n)assert(t->next==nodes+n+1);
        for(i=0;i<n;i++){assert(nodes[i+2].owner==t && nodes[i+2].priority==3 && nodes[i+2].result==91 && nodes[i+2].delay==0x1234);}
        if(n>0)assert(nodes[2].callback==fire_spin_entry);
        if(n>1)assert(nodes[3].callback==fire_trajectory_entry);
        if(n>2)assert(nodes[4].callback==fire_golfer_entry);
        cases++;
    }
    /* Entire update boundary contract, using genuine parent waits and an
     * explicitly exhausted pool. Never dispatch or fake successful children. */
    for(boundary=1;boundary<=3;boundary++){
        ntm_task *t;setup();d=driver(&a);assert(!ntm_shot_fire_application_enable(&a,&resources));d->pre_task=0;
        d->context.free_head=0;t=d->nodes+1;d->nodes[0].next=t;d->nodes[0].previous=t;t->next=d->nodes;t->previous=d->nodes;t->callback=fire_pre_ready;
        assert(!ntm_shot_fire_application_update(&a,16));assert(a.fire.phase==1 && t->delay==3 && d->updates==1);
        for(i=0;i<8;i++)assert(!ntm_shot_fire_application_update(&a,0));
        assert(a.fire.phase==3 && d->updates==9);
        event_count=0;assert(ntm_shot_fire_application_update(&a,(uint8_t)(16*boundary))==1);
        assert(a.boundary==(boundary==1?1:2) && d->updates==9 && !d->running);
        assert(d->context.current==t && d->context.published==t && event_count==2 && events[0]==1 && events[1]==2);
        assert(ntm_shot_fire_application_update(&a,0)==-1 && event_count==2);cases++;
    }
    /* Twelve caller-owned nodes means exactly eleven possible dispatches. */
    for(n=0;n<=12;n++){
        setup();d=driver(&a);assert(!ntm_shot_fire_application_enable(&a,&resources));d->pre_task=0;
        memset(nodes,0,sizeof nodes);d->nodes[0].next=n?nodes:d->nodes;
        for(i=0;i<n;i++){nodes[i].callback=callback;nodes[i].next=i+1<n?nodes+i+1:d->nodes;}
        assert(ntm_shot_fire_application_update(&a,0)==(n<=11?0:-1));
        assert(calls==(n<=11?n:11));assert(d->updates==(n<=11?1:0));
        assert(events[0]==1 && events[1]==2);if(n<=11)assert(events[event_count-1]==4);cases++;
    }
    printf("fire application: %u contract/allocation/partial-update/budget cases PASS; task=%lu, extra pool=%lu, total extension=%lu bytes\n",cases,(unsigned long)sizeof(ntm_task),(unsigned long)sizeof(a.extra_tasks),(unsigned long)(sizeof(a)-sizeof(a.base)));
    return 0;
}
