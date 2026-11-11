/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_aim_application.h"
static uint32_t aim_long(const uint8_t *p)
{return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void aim_word(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static int aim_setup_boundary(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_aim_application *a=(ntm_shot_aim_application *)c;(void)t;
    a->base.boundary=NTM_SHOT_BOUNDARY_AIM_SETUP;
    a->base.base.base.selector.driver.running=0;
    a->base.complete_update=a->base.base.base.selector.driver.updates+1;
    return 0;
}
static int aim_scene_continue(ntm_task_context *c,ntm_task *t)
{
    ntm_shot_aim_application *a=(ntm_shot_aim_application *)c;
    ntm_shot_application *b=&a->base;ntm_tee_setup_application *tee=&b->base;
    ntm_course_application *selector=&tee->base.selector;
    ntm_course_intro_context *i=tee->context.intro;
    ntm_hole_prepare_state *h=&tee->hole;ntm_shot_hud_context hud;
    const ntm_shot_aim_application_resources *r=a->resources;unsigned course=h->hole[4],hole=h->hole[5],index=18u*course+hole;int result;
    if(!r || a->phase || course>=3 || hole>=18 || index>=r->world_count)return -1;
    result=ntm_shot_aim_prepare(&a->aim,h,&b->scene.player,r->aim,&b->scene.player_status,i->screen_words);
    if(result<0)return -1;
    if(result){b->boundary=NTM_SHOT_BOUNDARY_UNSUPPORTED;selector->driver.running=0;return 0;}
    aim_word(a->world.bytes,a->aim.origin_x);aim_word(a->world.bytes+2,a->aim.origin_y);
    if(ntm_aim_world_load(&a->world,r->worlds+index,16,selector->tiles,selector->maps,
        selector->palette.palette,0,&selector->palette.background)<0)return -1;
    *i->flags|=8;
    if(ntm_course_palette_enqueue(i->palette,&r->palette)<0)return -1;
    hud.screen=i->screen;hud.maps=i->maps;hud.tiles=selector->tiles;hud.text_base=&tee->context.text_base;
    hud.scratch=i->display->reset_words;hud.saved=a->saved_hud;
    if(ntm_shot_hud_initialize(&hud,r->hud,aim_long(h->hole+0x68),h->hole[0x6e],h->hole[0x6f],h->hole[0x76])<0 ||
       ntm_shot_hud_clear(i->maps)<0 || ntm_tee_objects_reset(i->display,tee->objects)<0)return -1;
    *i->flags=(uint8_t)((*i->flags&63)|16);i->display->zero_flags[0]=34;i->display->capacity=30;
    if(ntm_shot_aim_panorama(&tee->context,r->setup,(uint8_t)course,(uint8_t)hole)<0)return -1;
    tee->object_resources[4]=r->objects[index];t->callback=a->scene_continuation?a->scene_continuation:aim_setup_boundary;a->phase=1;
    if(a->scene_continuation)return 0;
    /* This is an explicit boundary after the completed callback, not a fake
     * call to209830. The real tail still executes within this scheduler tick. */
    b->boundary=NTM_SHOT_BOUNDARY_AIM_SETUP;selector->driver.running=0;
    b->complete_update=selector->driver.updates+1;return 0;
}
int ntm_shot_aim_application_enable(ntm_shot_aim_application *a,const ntm_shot_aim_application_resources *r)
{
    unsigned j;
    if(!a || !r || !r->aim || !r->worlds || r->world_count<54 || !r->hud || !r->setup ||
       a->base.started || a->base.aim_continuation || a->phase ||
       !a->base.base.base.selector.driver.running || a->base.base.base.selector.driver.updates)return -1;
    for(j=0;j<54;j++)if(!r->objects[j])return -1;
    if(!ntm_course_palette_job_valid(&a->base.base.base.selector.palette,&r->palette))return -1;
    a->resources=r;a->base.aim_continuation=aim_scene_continue;
    aim_word(a->world.bytes+44,0x9800);aim_word(a->world.bytes+46,0x8110);aim_word(a->world.bytes+48,0x8300);
    return 0;
}
