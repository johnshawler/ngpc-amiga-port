/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_fire_application.h"
#include "input_state.h"
static ntm_shot_open_application *opening(ntm_shot_fire_application *a){return &a->base.base;}
static ntm_course_driver *driver(ntm_shot_fire_application *a){return &opening(a)->base.base.base.base.selector.driver;}
static int fire_stop(ntm_shot_fire_application *a,unsigned boundary)
{a->boundary=(uint8_t)boundary;opening(a)->base.base.boundary=NTM_SHOT_BOUNDARY_UNSUPPORTED;return NTM_TASK_BOUNDARY;}
static int fire_result(ntm_shot_fire_application *a,int result)
{return result==NTM_TASK_BOUNDARY?fire_stop(a,NTM_FIRE_APPLICATION_CHILD):result;}
static int fire_wait(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_fire_application *a=(ntm_shot_fire_application *)c;
    int r=ntm_shot_fire_tick(&a->fire,t,&a->fire_context,c->controls[1]);
    return r==2?fire_stop(a,a->fire.boundary):r;
}
static int fire_closed(ntm_task_context *c,ntm_task *t){return fire_wait(c,t);}
static int fire_open(ntm_task_context *c,ntm_task *t){return fire_wait(c,t);}
static int fire_ready(ntm_task_context *c,ntm_task *t){return fire_wait(c,t);}
static int fire_pre_ready(ntm_task_context *,ntm_task *);
static int fire_pre_delay(ntm_task_context *,ntm_task *);
static int fire_gate(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_fire_application *a=(ntm_shot_fire_application *)c;ntm_shot_open_application *o=opening(a);int r;
    r=ntm_shot_ready_tick(t,fire_pre_delay,fire_pre_ready,o->base.aim.reset_4416,c->controls[1]);
    if(t->callback==fire_pre_ready && !a->base.ready){a->base.ready=1;a->base.ready_update=driver(a)->updates+1;}
    if(r!=2)return r;
    r=ntm_shot_fire_initialize(&a->fire,t,&a->fire_context,a->resources->parent);if(r)return r;
    a->fire_update=driver(a)->updates+1;
    return 0; /* Initializer already includes the genuine same-call decrement. */
}
static int fire_pre_ready(ntm_task_context *c,ntm_task *t){return fire_gate(c,t);}
static int fire_pre_delay(ntm_task_context *c,ntm_task *t){return fire_gate(c,t);}
static int fire_marker(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_fire_application *a=(ntm_shot_fire_application *)c;ntm_shot_open_application *o=opening(a);
    o->marker_context.held=c->controls[0];o->marker_context.player_status=o->base.base.scene.player_status;
    return fire_result(a,ntm_shot_fire_markers_tick(&o->markers,t,c,&o->marker_context,o->resources->markers));
}
static int fire_marker_wait(ntm_task_context *c,ntm_task *t){return fire_marker(c,t);}
static int fire_spin_tick(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_fire_application *a=(ntm_shot_fire_application *)c;ntm_shot_open_application *o=opening(a);
    return fire_result(a,ntm_shot_spin_control(&a->spin,&o->scene_context,a->resources->spin_draw,
        o->base.aim.reset_d1,o->club.restore_delay,c->controls[1],c,t));
}
static int fire_spin_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_fire_application *a=(ntm_shot_fire_application *)c;ntm_shot_scene_object_context *s=&opening(a)->scene_context;
    return ntm_shot_spin_initialize(&a->spin,t,s->hole,s->player,a->resources->spin);
}
static int fire_trajectory_tick(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_fire_application *a=(ntm_shot_fire_application *)c;ntm_shot_open_application *o=opening(a);(void)t;
    int r=ntm_shot_trajectory_tick(&a->trajectory,&o->scene_context,a->resources->trajectory,o->club.label);
    return r==2?fire_stop(a,NTM_FIRE_APPLICATION_CHILD):fire_result(a,r);
}
static int fire_trajectory_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_fire_application *a=(ntm_shot_fire_application *)c;ntm_shot_open_application *o=opening(a);int r;
    r=ntm_shot_trajectory_initialize(&a->trajectory,t,&o->scene_context,a->resources->trajectory,o->club.label);
    if(!r)o->base.base.base.object_resources[16]=a->resources->trajectory_objects;
    return fire_result(a,r);
}
static int fire_golfer_animation(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_fire_application *a=(ntm_shot_fire_application *)c;ntm_shot_open_application *o=opening(a);unsigned index;(void)t;
    index=18u*o->scene_context.hole->hole[4]+o->scene_context.hole->hole[5];if(index>=a->resources->hole_count)return -1;
    return fire_result(a,ntm_shot_golfer_animation_tick(&a->golfer,o->scene_context.shared->intro->screen,a->resources->drawing[index].animation));
}
static int fire_golfer_drawing(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_fire_application *a=(ntm_shot_fire_application *)c;ntm_shot_open_application *o=opening(a);unsigned index;int r;
    index=18u*o->scene_context.hole->hole[4]+o->scene_context.hole->hole[5];if(index>=a->resources->hole_count)return -1;
    r=ntm_shot_golfer_draw_initialize(&a->drawing,&a->golfer,t,&o->scene_context,a->resources->drawing+index,fire_golfer_animation);
    if(!r){o->base.base.base.object_resources[18]=a->resources->green_objects;a->drawing_update=driver(a)->updates+1;}
    return fire_result(a,r);
}
static int fire_golfer_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_fire_application *a=(ntm_shot_fire_application *)c;ntm_shot_open_application *o=opening(a);unsigned index;int r;
    index=18u*o->scene_context.hole->hole[4]+o->scene_context.hole->hole[5];if(index>=a->resources->hole_count)return -1;
    a->golfer_context.terrain=a->resources->terrain+index;a->golfer_context.player_status=o->base.base.scene.player_status;
    r=ntm_shot_golfer_initialize(&a->golfer,t,&a->golfer_context,a->resources->golfer);
    if(!r)o->base.base.base.object_resources[5]=a->resources->pin_objects;
    return fire_result(a,r);
}
int ntm_shot_fire_application_enable(ntm_shot_fire_application *a,const ntm_shot_fire_application_resources *r)
{
    ntm_shot_open_application *o;ntm_course_driver *d;ntm_task *tail;unsigned i;
    if(!a || a->enabled || !r || !r->parent || !r->spin || !r->spin_draw || !r->trajectory || !r->golfer ||
       !r->drawing || !r->terrain || !r->hole_count || !r->trajectory_objects || !r->pin_objects || !r->green_objects)return -1;
    o=opening(a);d=driver(a);tail=d->context.free_head;if(!tail)return -1;
    /* The existing enable checks all live application phase/driver contracts. */
    if(ntm_shot_control_application_enable(&a->base)<0)return -1;
    while(tail->next)tail=tail->next;
    tail->next=a->extra_tasks;
    for(i=0;i<2;i++)a->extra_tasks[i].next=a->extra_tasks+i+1;
    a->extra_tasks[2].next=0;
    a->resources=r;o->visible_continuation=fire_pre_delay;
    o->markers.tick=fire_marker;o->markers.waiting=fire_marker_wait;
    a->fire.spin_entry=fire_spin_entry;a->fire.trajectory_entry=fire_trajectory_entry;a->fire.golfer_entry=fire_golfer_entry;
    a->fire.closed_delay=fire_closed;a->fire.open_delay=fire_open;a->fire.ready=fire_ready;
    a->spin.wait=fire_spin_tick;a->trajectory.wait=fire_trajectory_tick;a->golfer.drawing=fire_golfer_drawing;
    a->fire_context.shared=o->scene_context.shared;a->fire_context.aim=o->scene_context.aim;a->fire_context.overlay=&o->input_dispatch;
    a->golfer_context.scene=&o->scene_context;a->golfer_context.terrain_detail=&o->base.base.scene.terrain_detail;
    a->enabled=1;return 0;
}
int ntm_shot_fire_application_update(ntm_shot_fire_application *a,uint8_t buttons)
{
    ntm_course_driver *d;unsigned budget=11;int outcome;
    if(!a || !a->enabled)return -1;
    d=driver(a);if(!d->running)return -1;
    if(ntm_input_update(d->context.controls,buttons)<0 || d->service_pump(d->service_context)<0 ||
       ntm_course_screen_fill_queue(d->screen,d->maps)<0 || ntm_course_screen_stamp_queue(d->screen,d->tiles,d->maps)<0)return -1;
    if(d->drawing_head && d->drawing_head(d->service_context)<0)return -1;
    if(d->pre_task){outcome=d->pre_task(d->pre_task_context);if(outcome==NTM_TASK_BOUNDARY){d->running=0;return 1;}if(outcome<0)return -1;}
    d->context.current=d->nodes[0].next;
    while(d->context.current!=d->nodes){
        if(!budget--)return -1;
        outcome=ntm_task_dispatch_one(&d->context);
        if(outcome==NTM_TASK_BOUNDARY){d->running=0;return 1;}if(outcome<0)return -1;
    }
    if(d->service_tail(d->service_context)<0)return -1;
    d->updates++;
    if(!d->running){d->running=0;return 1;}return 0;
}
