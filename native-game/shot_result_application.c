/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_result_application.h"
static ntm_shot_open_application *result_open(ntm_shot_result_application *a){return &a->base.base.base.base.base;}
static int result_stop(ntm_shot_result_application *a,unsigned boundary)
{a->boundary=(uint8_t)boundary;result_open(a)->base.base.boundary=NTM_SHOT_BOUNDARY_UNSUPPORTED;return NTM_TASK_BOUNDARY;}
static int result_flight_entry(ntm_task_context *c,ntm_task *t)
{(void)t;return result_stop((ntm_shot_result_application *)c,NTM_RESULT_FLIGHT_ENTRY);}
static int result_flight_wait(ntm_task_context *c,ntm_task *t)
{if(!t->next)return -1;return t->priority<t->next->priority?0:result_stop((ntm_shot_result_application *)c,NTM_RESULT_NEXT_STAGE);}
static int result_owner_wait(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_result_application *a=(ntm_shot_result_application *)c;
    if(!t->next || !result_open(a)->scene_context.shared->fallback)return -1;
    if(t->priority<t->next->priority)return 0;
    (void)ntm_task_allocate(c,t,result_flight_entry,result_open(a)->scene_context.shared->fallback);
    t->callback=result_flight_wait;return 0;
}
static int result_after_exit(ntm_shot_result_application *a,ntm_task *owner,int r)
{if(r>=0 && a->exit.phase==3 && owner)owner->callback=result_owner_wait;return r;}
static int result_parent_wait(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_result_application *a=(ntm_shot_result_application *)c;ntm_task *owner=t->owner;int r;
    a->base.context.suppress_sound=a->base.suppress_sound;
    r=ntm_shot_swing_tick(&a->base.swing,t,&a->base.context);if(r!=2)return r;
    r=a->base.swing.boundary==NTM_SHOT_SWING_HIT?
        ntm_shot_swing_exit_hit(&a->exit,t,&result_open(a)->scene_context):
        ntm_shot_swing_exit_miss(&a->exit,t,&result_open(a)->scene_context);
    return result_after_exit(a,owner,r);
}
static int result_parent_delay(ntm_task_context *c,ntm_task *t){return result_parent_wait(c,t);}
static int result_hit_reset(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_result_application *a=(ntm_shot_result_application *)c;ntm_task *owner=t->owner;
    return result_after_exit(a,owner,ntm_shot_swing_exit_hit(&a->exit,t,&result_open(a)->scene_context));
}
static int result_perfect_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_result_application *a=(ntm_shot_result_application *)c;
    return ntm_shot_perfect_initialize(&a->perfect,t,&result_open(a)->scene_context,a->resources->perfect);
}
static int result_perfect_first(ntm_task_context *c,ntm_task *t)
{ntm_shot_result_application *a=(ntm_shot_result_application *)c;return ntm_shot_perfect_tick(&a->perfect,t,&result_open(a)->scene_context);}
static int result_perfect_visible(ntm_task_context *c,ntm_task *t){return result_perfect_first(c,t);}
static int result_perfect_finish(ntm_task_context *c,ntm_task *t){return ntm_task_finish(c,t);}
static int result_exit_wait(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_result_application *a=(ntm_shot_result_application *)c;ntm_task *owner=t->owner;
    return result_after_exit(a,owner,ntm_shot_swing_exit_tick(&a->exit,t,&result_open(a)->scene_context));
}
static int result_perfect_wait(ntm_task_context *c,ntm_task *t){return result_exit_wait(c,t);}
static int result_miss_wait(ntm_task_context *c,ntm_task *t){return result_exit_wait(c,t);}
static int result_miss_delay(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_result_application *a=(ntm_shot_result_application *)c;int r=ntm_shot_miss_tick(&a->miss,t);
    return r==2?result_stop(a,NTM_RESULT_POST_SHOT):r;
}
static int result_miss_continue(ntm_task_context *c,ntm_task *t)
{(void)t;return result_stop((ntm_shot_result_application *)c,NTM_RESULT_POST_SHOT);}
static int result_miss_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_result_application *a=(ntm_shot_result_application *)c;ntm_shot_open_application *o=result_open(a);
    return ntm_shot_miss_initialize(&a->miss,t,&o->scene_context,&o->input_dispatch,a->resources->miss);
}
int ntm_shot_result_application_enable(ntm_shot_result_application *a,const ntm_shot_result_application_resources *r)
{
    if(!a || a->enabled || !r || !r->swing || !r->miss || !r->perfect)return -1;
    if(ntm_shot_swing_application_enable(&a->base,r->swing)<0)return -1;
    a->resources=r;a->base.swing.waiting=result_parent_wait;a->base.swing.hit_delay=result_parent_delay;a->base.swing.hit_reset=result_hit_reset;
    a->exit.perfect_entry=result_perfect_entry;a->exit.perfect_wait=result_perfect_wait;
    a->exit.miss_entry=result_miss_entry;a->exit.miss_wait=result_miss_wait;
    a->perfect.first_delay=result_perfect_first;a->perfect.visible_delay=result_perfect_visible;a->perfect.finish=result_perfect_finish;
    a->miss.delay=result_miss_delay;a->miss.continuation=result_miss_continue;a->enabled=1;return 0;
}
int ntm_shot_result_application_update(ntm_shot_result_application *a,uint8_t buttons)
{return !a || !a->enabled?-1:ntm_shot_swing_application_update(&a->base,buttons);}
