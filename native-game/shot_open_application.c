/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_open_application.h"
static int open_result(ntm_shot_open_application *a,int result)
{
    if(result<0)return -1;
    if(result==2){a->base.base.boundary=NTM_SHOT_BOUNDARY_UNSUPPORTED;a->base.base.base.base.selector.driver.running=0;}
    return 0;
}
static int open_hint_active(ntm_task_context *c,ntm_task *t)
{(void)t;return open_result((ntm_shot_open_application *)c,2);}
static int open_hint_tick(ntm_task_context *c,ntm_task *t)
{ntm_shot_open_application *a=(ntm_shot_open_application *)c;return open_result(a,ntm_shot_hint_tick(&a->hint,t));}
static int open_hint_entry(ntm_task_context *c,ntm_task *t)
{ntm_shot_open_application *a=(ntm_shot_open_application *)c;return open_result(a,ntm_shot_hint_initialize(&a->hint,t,c,a->base.base.base.context.input_mode));}
static int open_camera_active(ntm_task_context *c,ntm_task *t)
{ntm_shot_open_application *a=(ntm_shot_open_application *)c;return a->camera_continuation?a->camera_continuation(c,t):open_result(a,2);}
static int open_camera_tick(ntm_task_context *c,ntm_task *t)
{ntm_shot_open_application *a=(ntm_shot_open_application *)c;int r=ntm_shot_camera_tick(&a->camera,t);return r==2 && a->camera_continuation?a->camera_continuation(c,t):open_result(a,r);}
static int open_camera_entry(ntm_task_context *c,ntm_task *t)
{ntm_shot_open_application *a=(ntm_shot_open_application *)c;return open_result(a,ntm_shot_camera_initialize(&a->camera,t));}
static int open_club_tick(ntm_task_context *c,ntm_task *t)
{ntm_shot_open_application *a=(ntm_shot_open_application *)c;return a->club_continuation?a->club_continuation(c,t):open_result(a,ntm_shot_club_tick(&a->club,&a->scene_context));}
static int open_club_entry(ntm_task_context *c,ntm_task *t)
{ntm_shot_open_application *a=(ntm_shot_open_application *)c;int r=ntm_shot_club_initialize(&a->club,t,&a->scene_context,a->resources->club);return !r && a->club_continuation?a->club_continuation(c,t):open_result(a,r);}
static void marker_input(ntm_shot_open_application *a)
{a->marker_context.held=a->base.base.base.base.selector.controls[0];a->marker_context.player_status=a->base.base.scene.player_status;}
static int open_marker_tick(ntm_task_context *c,ntm_task *t)
{ntm_shot_open_application *a=(ntm_shot_open_application *)c;marker_input(a);return open_result(a,ntm_shot_markers_tick(&a->markers,t,&a->marker_context,a->resources->markers));}
static int open_marker_waiting(ntm_task_context *c,ntm_task *t){return open_marker_tick(c,t);}
static int open_marker_entry(ntm_task_context *c,ntm_task *t)
{ntm_shot_open_application *a=(ntm_shot_open_application *)c;marker_input(a);return open_result(a,ntm_shot_markers_initialize(&a->markers,t,&a->marker_context,a->resources->markers));}
static int open_visible_delay(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_open_application *a=(ntm_shot_open_application *)c;
    /*2098ee is intentionally beyond the declared first-visible boundary. */
    (void)t;return open_result(a,2);
}
static int open_wait(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_open_application *a=(ntm_shot_open_application *)c;
    ntm_course_driver *d=&a->base.base.base.base.selector.driver;uint8_t *v=a->scene_context.shared->intro->viewport;
    t->delay=(uint16_t)(t->delay-1u);if(!(t->delay&32768u))return 0;
    /*2098af and2098ee's same-call first decrement. No later input dispatch
     * occurs in this bounded application; handler identities are owned state. */
    v[0]=v[1]=0;v[2]=160;v[3]=152;a->handlers_installed=1;a->input_dispatch=0;
    t->delay=3;t->callback=a->visible_continuation?a->visible_continuation:open_visible_delay;a->phase=2;
    a->open_update=d->updates+1;a->base.base.complete_update=a->open_update;
    if(a->visible_continuation)return 0;
    a->base.base.boundary=NTM_SHOT_BOUNDARY_AIM_OPEN;d->running=0;return 0;
}
static int open_scene_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_open_application *a=(ntm_shot_open_application *)c;ntm_tee_setup_application *tee=&a->base.base.base;
    static const uint8_t slots[11]={0,1,2,3,5,6,7,8,9,10,17};unsigned j;
    if(a->phase || ntm_shot_scene_objects_initialize(&a->scene_context,a->resources->scene,a->base.world.bytes)<0)return -1;
    for(j=0;j<11;j++)tee->object_resources[slots[j]]=a->resources->objects;
    (void)ntm_task_allocate(c,t,open_hint_entry,&tee->base.fallback);
    (void)ntm_task_allocate(c,t,open_marker_entry,&tee->base.fallback);
    (void)ntm_task_allocate(c,t,open_club_entry,&tee->base.fallback);
    (void)ntm_task_allocate(c,t,open_camera_entry,&tee->base.fallback);
    t->delay=2;t->callback=open_wait;a->phase=1;return open_wait(c,t);
}
int ntm_shot_open_application_enable(ntm_shot_open_application *a,const ntm_shot_open_resources *r)
{
    ntm_tee_setup_application *tee;
    if(!a || !r || !r->scene || !r->club || !r->markers || !r->objects || !a->base.resources ||
       r->club->aim!=a->base.resources->aim || r->club->scene!=r->scene ||
       a->base.phase || a->base.scene_continuation || a->phase || !a->base.base.base.base.selector.driver.running ||
       a->base.base.base.base.selector.driver.updates)return -1;
    tee=&a->base.base.base;a->resources=r;a->base.scene_continuation=open_scene_entry;
    a->scene_context.shared=&tee->context;a->scene_context.hole=&tee->hole;a->scene_context.player=&a->base.base.scene.player;
    a->scene_context.aim=&a->base.aim;a->scene_context.saved_map=a->saved_map;
    a->marker_context.shared=&tee->context;a->marker_context.hole=&tee->hole;a->marker_context.aim=&a->base.aim;
    a->marker_context.dirty=&a->club.change_flag;
    a->club.wait=open_club_tick;a->markers.tick=open_marker_tick;a->markers.waiting=open_marker_waiting;
    a->hint.delay=open_hint_tick;a->hint.active=open_hint_active;a->camera.delay=open_camera_tick;a->camera.active=open_camera_active;
    return 0;
}
