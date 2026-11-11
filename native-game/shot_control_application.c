/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_control_application.h"
static int stop(ntm_shot_control_application *a,uint8_t kind)
{a->partial_boundary=kind;a->base.base.base.boundary=NTM_SHOT_BOUNDARY_UNSUPPORTED;return NTM_TASK_BOUNDARY;}
static int control_ready(ntm_task_context *,ntm_task *);
static int control_delay(ntm_task_context *,ntm_task *);
static int control_gate(ntm_shot_control_application *a,ntm_task *t)
{
    ntm_course_application *s=&a->base.base.base.base.base.selector;int r;
    r=ntm_shot_ready_tick(t,control_delay,control_ready,a->base.base.aim.reset_4416,s->controls[1]);
    if(t->callback==control_ready && !a->ready){a->ready=1;a->ready_update=s->driver.updates+1;}
    return r==2?stop(a,1):r;
}
static int control_delay(ntm_task_context *c,ntm_task *t)
{return control_gate((ntm_shot_control_application *)c,t);}
static int control_ready(ntm_task_context *c,ntm_task *t)
{return control_gate((ntm_shot_control_application *)c,t);}
static void sync_origins(ntm_shot_aim_application *a)
{
    a->world.bytes[0]=(uint8_t)a->aim.origin_x;a->world.bytes[1]=(uint8_t)(a->aim.origin_x>>8);
    a->world.bytes[2]=(uint8_t)a->aim.origin_y;a->world.bytes[3]=(uint8_t)(a->aim.origin_y>>8);
}
static int control_camera(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_control_application *a=(ntm_shot_control_application *)c;int r;
    a->base.marker_context.held=a->base.base.base.base.base.selector.controls[0];
    a->base.marker_context.player_status=a->base.base.base.scene.player_status;
    r=ntm_shot_camera_active(&a->camera,t,c,&a->base.marker_context,a->base.resources->markers,
        a->base.base.base.base.base.selector.controls[1]);
    if(!r)sync_origins(&a->base.base);
    return r==2?stop(a,2):r;
}
static int control_club(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_control_application *a=(ntm_shot_control_application *)c;int r;(void)t;
    r=ntm_shot_club_control(&a->base.club,&a->base.scene_context,a->base.resources->club,
        a->base.base.base.base.base.selector.controls[0],a->base.base.base.base.base.selector.controls[1]);
    return r==2?stop(a,3):r;
}
static int control_pre_task(void *opaque)
{
    ntm_shot_control_application *a=opaque;ntm_shot_aim_application *aim=&a->base.base;
    ntm_course_application *s=&aim->base.base.base.selector;unsigned index;uint8_t scroll[2];int r;
    if(aim->phase && (s->initialization.flags&8u)){
        index=18u*aim->base.base.hole.hole[4]+aim->base.base.hole.hole[5];
        if(index>=aim->resources->world_count)return -1;
        sync_origins(aim);
        r=ntm_shot_camera_scroll(&aim->world,aim->resources->worlds+index,s->maps,scroll);if(r)return r;
        s->scroll_x[1]=scroll[0];s->scroll_y[1]=scroll[1];
    }
    if(a->base.handlers_installed && ntm_shot_ready_input(s->controls[0],s->controls[1],a->base.input_dispatch))return stop(a,4);
    return 0;
}
int ntm_shot_control_application_enable(ntm_shot_control_application *a)
{
    ntm_course_driver *d;
    if(!a || !a->base.resources || a->enabled || a->base.phase || a->base.visible_continuation || a->base.camera_continuation || a->base.club_continuation)return -1;
    d=&a->base.base.base.base.base.selector.driver;
    if(!d->running || d->updates || d->pre_task || a->base.base.base.base.context.input_mode)return -1;
    if(ntm_shot_ready_bind_players(a->base.base.base.base.base.round_prepare.players)<0)return -1;
    a->base.visible_continuation=control_delay;a->base.camera_continuation=control_camera;a->base.club_continuation=control_club;
    d->pre_task=control_pre_task;d->pre_task_context=a;a->enabled=1;return 0;
}
