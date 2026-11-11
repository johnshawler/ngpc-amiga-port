/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_application.h"
static int shot_result(ntm_shot_application *a,int result)
{
    if(result<0)return -1;
    if(result==NTM_TEE_UNSUPPORTED){a->boundary=NTM_SHOT_BOUNDARY_UNSUPPORTED;a->base.base.selector.driver.running=0;}
    return 0;
}
static int shot_palette_tail(void *p)
{ntm_shot_application *a=p;return ntm_shot_palette_tail(&a->base.base.selector.palette,&a->palette);}
static int shot_palette_request(void *p,uint16_t command)
{
    ntm_shot_application *a=p;
    if(command!=0x6c && command!=0x70)return -1;
    return ntm_shot_palette_enqueue(&a->base.base.selector.palette,&a->palette,a->resources->exit_palettes+(command==0x70));
}
static int shot_banner_tick(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_application *a=(ntm_shot_application *)c;int result;
    a->base.context.control=a->base.base.selector.controls[1];
    result=ntm_shot_banner_tick(&a->banner,t,&a->banner_context,a->resources->banner);
    return shot_result(a,result);
}
static int shot_banner_delay(ntm_task_context *c,ntm_task *t){return shot_banner_tick(c,t);}
static int shot_banner_hold(ntm_task_context *c,ntm_task *t){return shot_banner_tick(c,t);}
static int shot_banner_exit_wait(ntm_task_context *c,ntm_task *t){return shot_banner_tick(c,t);}
static int shot_banner_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_application *a=(ntm_shot_application *)c;
    a->banner_update=a->base.base.selector.driver.updates+1;
    return shot_result(a,ntm_shot_banner_initialize(&a->banner,t,&a->banner_context,a->resources->banner));
}
static int shot_notice_entry(ntm_task_context *c,ntm_task *t)
{return ntm_shot_banner_notice(t,&((ntm_shot_application *)c)->banner_context);}
static int shot_animation_tick(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_application *a=(ntm_shot_application *)c;
    return shot_result(a,ntm_shot_banner_animation_tick(&a->animation,t,&a->banner_context,a->resources->banner));
}
static int shot_animation_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_application *a=(ntm_shot_application *)c;
    return shot_result(a,ntm_shot_banner_animation_initialize(&a->animation,t,&a->banner_context,a->resources->banner));
}
static int shot_exit_tick(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_application *a=(ntm_shot_application *)c;
    return shot_result(a,ntm_shot_banner_exit_tick(&a->exit,t,&a->banner_context));
}
static int shot_exit_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_application *a=(ntm_shot_application *)c;
    return shot_result(a,ntm_shot_banner_exit_initialize(&a->exit,t,&a->banner_context,a->resources->banner));
}
static int shot_scene_wait(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_application *a=(ntm_shot_application *)c;int result=ntm_shot_scene_wait(&a->scene,t);
    if(result<0)return -1;
    if(result){a->boundary=NTM_SHOT_BOUNDARY_SCENE_SETUP;a->base.base.selector.driver.running=0;a->complete_update=a->base.base.selector.driver.updates+1;}
    return 0;
}
static int shot_scene_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_application *a=(ntm_shot_application *)c;
    return shot_result(a,ntm_shot_scene_initialize(&a->scene,t,&a->scene_context));
}
static int shot_tee_wait(ntm_task_context *c,ntm_task *t)
{(void)c;if(!t->next)return -1;return t->priority<t->next->priority?0:-1;}
static int shot_allocate(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_application *a=(ntm_shot_application *)c;
    if(a->base.overview.phase!=NTM_TEE_DONE)return shot_result(a,NTM_TEE_UNSUPPORTED);
    if(!t->next)return -1;
    a->scene.player_status=a->base.owner.player_status;
    a->base.object_resources[11]=a->resources->objects;
    (void)ntm_task_allocate(c,t,shot_scene_entry,&a->base.base.fallback);
    t->callback=shot_tee_wait;a->started=1;a->start_update=a->base.base.selector.driver.updates+1;return 0;
}
int ntm_shot_application_start(ntm_shot_application *a,const ntm_course_resources *selector,
    const ntm_course_intro_resources courses[3],const ntm_course_title_resources titles[3],
    const ntm_cursor_assets title_objects[3],const ntm_round_record_resources *round,
    const ntm_tee_setup_resources *tee,const ntm_hole_prepare_resources *holes,const ntm_shot_resources *r)
{
    ntm_task *tail;unsigned i;int result;
    if(!a || !r || !r->preparation || !r->terrain || !r->banner || !r->objects || !r->exit_palettes || a->started)return -1;
    result=ntm_tee_setup_application_start(&a->base,selector,courses,titles,title_objects,round,tee,holes);if(result)return result;
    a->resources=r;a->base.shot_continuation=shot_allocate;a->base.palette_tail=shot_palette_tail;a->base.palette_context=a;
    tail=a->base.base.selector.driver.context.free_head;
    if(!tail)return -1;
    while(tail->next)tail=tail->next;
    tail->next=a->extra_tasks;
    for(i=0;i<3;i++)a->extra_tasks[i].next=a->extra_tasks+i+1;
    a->extra_tasks[3].next=0;
    a->scene.banner_entry=shot_banner_entry;a->scene.banner_wait=shot_scene_wait;
    a->scene_context.shared=&a->base.context;a->scene_context.hole=&a->base.hole;
    a->scene_context.preparation=r->preparation;a->scene_context.terrain=r->terrain;
    a->scene_context.random_cursor=selector->random_cursor;a->scene_context.random_window=selector->random;
    a->banner.delay=shot_banner_delay;a->banner.animation_wait=shot_banner_tick;a->banner.hold=shot_banner_hold;
    a->banner.exit_wait=shot_banner_exit_wait;a->banner.notice_entry=shot_notice_entry;
    a->banner.animation_entry=shot_animation_entry;a->banner.exit_entry=shot_exit_entry;
    a->animation.wait=shot_animation_tick;a->exit.tick=shot_exit_tick;
    a->banner_context.shared=&a->base.context;a->banner_context.hole=&a->base.hole;a->banner_context.player=&a->scene.player;
    a->banner_context.controlling_player=a->base.base.records.active_player;
    a->banner_context.exit_palette_request=shot_palette_request;a->banner_context.exit_palette_context=a;a->banner_context.raster=&a->raster;
    return 0;
}
int ntm_shot_application_update(ntm_shot_application *a,uint8_t buttons)
{return a?ntm_tee_setup_application_update(&a->base,buttons):-1;}
