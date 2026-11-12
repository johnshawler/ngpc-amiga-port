/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_swing_application.h"
static ntm_shot_open_application *swing_open(ntm_shot_swing_application *a)
{return &a->base.base.base.base;}
static int swing_stop(ntm_shot_swing_application *a,unsigned boundary)
{a->boundary=(uint8_t)boundary;swing_open(a)->base.base.boundary=NTM_SHOT_BOUNDARY_UNSUPPORTED;return NTM_TASK_BOUNDARY;}
static int swing_parent_wait(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_swing_application *a=(ntm_shot_swing_application *)c;int r;
    a->context.suppress_sound=a->suppress_sound;
    r=ntm_shot_swing_tick(&a->swing,t,&a->context);
    return r==2?swing_stop(a,a->swing.boundary==NTM_SHOT_SWING_HIT?
        NTM_SWING_APPLICATION_HIT_RESET:NTM_SWING_APPLICATION_MISS_CHILD):r;
}
static int swing_parent_delay(ntm_task_context *c,ntm_task *t){return swing_parent_wait(c,t);}
static int swing_hit_reset(ntm_task_context *c,ntm_task *t)
{(void)t;return swing_stop((ntm_shot_swing_application *)c,NTM_SWING_APPLICATION_HIT_RESET);}
static int swing_power_ready(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_swing_application *a=(ntm_shot_swing_application *)c;int r;
    r=ntm_shot_power_tick(&a->base.power,t,&a->base.context,c->controls[1]);
    if(r!=2)return r;
    if(a->base.power.boundary!=NTM_SHOT_POWER_SWING)return swing_stop(a,a->base.power.boundary);
    a->context.suppress_sound=a->suppress_sound;
    return ntm_shot_swing_initialize(&a->swing,t,&a->context);
}
static int swing_power_delay(ntm_task_context *c,ntm_task *t){return swing_power_ready(c,t);}
static int swing_trajectory_animate(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_swing_application *a=(ntm_shot_swing_application *)c;ntm_shot_open_application *o=swing_open(a);
    return ntm_shot_trajectory_swing_tick(&a->trajectory,&a->base.base.trajectory,c,t,
        &o->scene_context,a->resources->trajectory,a->suppress_sound,o->club.label);
}
static int swing_trajectory_wait(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_swing_application *a=(ntm_shot_swing_application *)c;ntm_shot_open_application *o=swing_open(a);int r;
    r=ntm_shot_trajectory_tick(&a->base.base.trajectory,&o->scene_context,
        a->base.base.resources->trajectory,o->club.label);
    if(r!=2)return r;
    return ntm_shot_trajectory_swing_enter(&a->trajectory,&a->base.base.trajectory,c,t,
        &o->scene_context,a->resources->trajectory,a->suppress_sound,o->club.label);
}
int ntm_shot_swing_application_enable(ntm_shot_swing_application *a,const ntm_shot_swing_application_resources *r)
{
    ntm_shot_open_application *o;
    if(!a || a->enabled || !r || !r->power || !r->trajectory)return -1;
    if(ntm_shot_power_application_enable(&a->base,r->power)<0)return -1;
    o=swing_open(a);a->resources=r;
    a->base.power.accuracy_delay=swing_power_delay;a->base.power.accuracy_ready=swing_power_ready;
    a->base.base.trajectory.wait=swing_trajectory_wait;
    a->swing.waiting=swing_parent_wait;a->swing.hit_delay=swing_parent_delay;a->swing.hit_reset=swing_hit_reset;
    a->trajectory.animate=swing_trajectory_animate;
    a->context.scene=&o->scene_context;a->context.overlay=&o->input_dispatch;
    a->enabled=1;return 0;
}
int ntm_shot_swing_application_update(ntm_shot_swing_application *a,uint8_t buttons)
{return !a || !a->enabled?-1:ntm_shot_power_application_update(&a->base,buttons);}
