/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_power_application.h"
static ntm_shot_open_application *power_opening(ntm_shot_power_application *a){return &a->base.base.base;}
static ntm_course_driver *power_driver(ntm_shot_power_application *a){return &power_opening(a)->base.base.base.base.selector.driver;}
static int power_stop(ntm_shot_power_application *a,unsigned boundary)
{a->boundary=(uint8_t)boundary;power_opening(a)->base.base.boundary=NTM_SHOT_BOUNDARY_UNSUPPORTED;return NTM_TASK_BOUNDARY;}
static int power_fire_wait(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_power_application *a=(ntm_shot_power_application *)c;int r;
    r=ntm_shot_fire_tick(&a->base.fire,t,&a->base.fire_context,c->controls[1]);
    if(r!=2)return r;
    if(a->base.fire.boundary==NTM_SHOT_FIRE_CANCEL)return ntm_shot_power_cancel(&a->power,t,&a->context);
    r=ntm_shot_power_initialize(&a->power,t,&a->context);
    if(!r)a->power_update=power_driver(a)->updates+1;
    return r;
}
static int power_fire_closed(ntm_task_context *c,ntm_task *t){return power_fire_wait(c,t);}
static int power_fire_open(ntm_task_context *c,ntm_task *t){return power_fire_wait(c,t);}
static int power_fire_ready(ntm_task_context *c,ntm_task *t){return power_fire_wait(c,t);}
static int power_parent_tick(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_power_application *a=(ntm_shot_power_application *)c;int r;
    r=ntm_shot_power_tick(&a->power,t,&a->context,c->controls[1]);
    if(r==2){
        if(a->power.boundary==NTM_SHOT_POWER_WORLD){
            ntm_shot_aim_application *aim=&power_opening(a)->base;
            aim->world.bytes[0]=(uint8_t)aim->aim.origin_x;aim->world.bytes[1]=(uint8_t)(aim->aim.origin_x>>8);
            aim->world.bytes[2]=(uint8_t)aim->aim.origin_y;aim->world.bytes[3]=(uint8_t)(aim->aim.origin_y>>8);
        }
        return power_stop(a,a->power.boundary);
    }
    if(!r && a->power.phase==8){a->base.fire.phase=1;a->base.fire.boundary=0;a->power.phase=0;}
    return r;
}
static int power_delay_tick(ntm_task_context *c,ntm_task *t){return power_parent_tick(c,t);}
static int power_ready_tick(ntm_task_context *c,ntm_task *t){return power_parent_tick(c,t);}
static int power_accuracy_delay(ntm_task_context *c,ntm_task *t){return power_parent_tick(c,t);}
static int power_accuracy_ready(ntm_task_context *c,ntm_task *t){return power_parent_tick(c,t);}
static int power_cancel_rebuild(ntm_task_context *c,ntm_task *t){return power_parent_tick(c,t);}
static int power_cancel_restore(ntm_task_context *c,ntm_task *t){return power_parent_tick(c,t);}
static int power_cancel_return(ntm_task_context *c,ntm_task *t){return power_parent_tick(c,t);}
static int power_accuracy_tick(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_power_application *a=(ntm_shot_power_application *)c;ntm_shot_open_application *o=power_opening(a);
    return ntm_shot_accuracy_tick(&a->accuracy,&o->scene_context,o->base.aim.reset_d1,o->club.label,c->controls[0],c,t);
}
static int power_accuracy_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_power_application *a=(ntm_shot_power_application *)c;
    a->accuracy.phase=0;
    return ntm_shot_accuracy_initialize(&a->accuracy,t,&power_opening(a)->scene_context);
}
static int power_meter_tick(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_power_application *a=(ntm_shot_power_application *)c;ntm_shot_open_application *o=power_opening(a);int r;
    r=ntm_shot_power_meter_tick(&a->meter,&o->scene_context,a->resources->meter,o->base.aim.reset_d1,c,t);
    return r==NTM_TASK_BOUNDARY?power_stop(a,NTM_POWER_APPLICATION_METER):r;
}
static int power_meter_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_power_application *a=(ntm_shot_power_application *)c;
    a->meter.phase=0;
    return ntm_shot_power_meter_initialize(&a->meter,t,&power_opening(a)->scene_context,a->resources->meter);
}
int ntm_shot_power_application_enable(ntm_shot_power_application *a,
    const ntm_shot_power_application_resources *r)
{
    ntm_shot_open_application *o;
    if(!a || a->enabled || !r || !r->fire || !r->meter)return -1;
    if(ntm_shot_fire_application_enable(&a->base,r->fire)<0)return -1;
    o=power_opening(a);a->resources=r;
    a->base.fire.closed_delay=power_fire_closed;a->base.fire.open_delay=power_fire_open;a->base.fire.ready=power_fire_ready;
    a->power.meter_entry=power_meter_entry;a->power.power_delay=power_delay_tick;a->power.power_ready=power_ready_tick;
    a->power.accuracy_entry=power_accuracy_entry;a->power.accuracy_delay=power_accuracy_delay;a->power.accuracy_ready=power_accuracy_ready;
    a->power.cancel_rebuild=power_cancel_rebuild;a->power.cancel_restore=power_cancel_restore;a->power.cancel_return=power_cancel_return;
    a->power.fire_closed=power_fire_closed;a->meter.wait=power_meter_tick;a->accuracy.wait=power_accuracy_tick;
    a->context.scene=&o->scene_context;a->context.overlay=&o->input_dispatch;a->context.saved_meter=o->base.saved_hud;
    a->enabled=1;return 0;
}
int ntm_shot_power_application_update(ntm_shot_power_application *a,uint8_t buttons)
{return !a || !a->enabled?-1:ntm_shot_fire_application_update(&a->base,buttons);}
