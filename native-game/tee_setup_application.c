/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "tee_setup_application.h"
static uint16_t tee_word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static void tee_sound(void *opaque,uint8_t service,uint16_t command,uint16_t parameter)
{
    ntm_tee_setup_application *a=opaque;
    /* Preserve the query byte written before the unconverted audio work.
     * All other sound activity remains explicitly a request, not synthesis. */
    if(command==65535)a->owner.current_music=(uint8_t)(parameter+(service?16u:0u));
    if(a->context.sound_request)a->context.sound_request(a->context.sound_context,service,command,parameter);
}
static int tee_prepare(void *opaque,ntm_tee_setup_context *c)
{
    ntm_tee_setup_application *a=opaque;ntm_hole_prepare_state *p=&a->hole;unsigned i,bank;
    if(ntm_hole_prepare(p,a->hole_resources,a->base.selector.resources->random_cursor,a->base.selector.resources->random)<0)return -1;
    c->course=p->hole[4];c->round_index=p->hole[5];c->hole_index=p->hole[6];c->par=p->hole[0x15];
    c->distance=tee_word(p->hole+0x16);c->pin_x=tee_word(p->hole+0x1a);c->pin_y=tee_word(p->hole+0x1e);
    c->origin_x=tee_word(p->hole+0x3c);c->origin_y=tee_word(p->hole+0x3e);
    a->owner.course=c->course;
    for(bank=0;bank<2;bank++)for(i=0;i<64;i++)a->base.round_prepare.players[bank][i]=p->players[bank][i];
    a->base.round_prepare.mapped_course=c->course;a->base.round_prepare.round_index=c->round_index;
    return 0;
}
static int tee_problem(ntm_tee_setup_application *a,int result)
{
    if(result<0)return -1;
    if(result==NTM_TEE_UNSUPPORTED){a->boundary=NTM_TEE_BOUNDARY_UNSUPPORTED;a->base.selector.driver.running=0;}
    return 0;
}
static int tee_tick(ntm_task_context *c,ntm_task *t)
{
    ntm_tee_setup_application *a=(ntm_tee_setup_application *)c;int result;
    a->context.control=a->base.selector.controls[1];
    result=ntm_tee_setup_tick(&a->overview,t,&a->context,a->resources);
    if(!a->open_update && a->overview.phase==NTM_TEE_OPEN_DELAY)a->open_update=a->base.selector.driver.updates+1;
    if(!a->hold_update && a->overview.phase==NTM_TEE_HOLD)a->hold_update=a->base.selector.driver.updates+1;
    if(!a->complete_update && a->overview.phase==NTM_TEE_DONE)a->complete_update=a->base.selector.driver.updates+1;
    return tee_problem(a,result);
}
static int tee_delay(ntm_task_context *c,ntm_task *t){return tee_tick(c,t);}
static int tee_open_delay(ntm_task_context *c,ntm_task *t){return tee_tick(c,t);}
static int tee_hold(ntm_task_context *c,ntm_task *t){return tee_tick(c,t);}
static int tee_wait_handicap(ntm_task_context *c,ntm_task *t){return tee_tick(c,t);}
static int tee_notice_entry(ntm_task_context *c,ntm_task *t)
{return ntm_tee_notice(t,&((ntm_tee_setup_application *)c)->context);}
static int tee_handicap_wait(ntm_task_context *c,ntm_task *t)
{
    ntm_tee_setup_application *a=(ntm_tee_setup_application *)c;
    return ntm_tee_handicap_tick(&a->handicap,t,&a->context)<0?-1:0;
}
static int tee_handicap_cleanup(ntm_task_context *c,ntm_task *t){return tee_handicap_wait(c,t);}
static int tee_handicap_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_tee_setup_application *a=(ntm_tee_setup_application *)c;
    return tee_problem(a,ntm_tee_handicap_initialize(&a->handicap,t,&a->context,a->resources));
}
static int tee_overview_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_tee_setup_application *a=(ntm_tee_setup_application *)c;
    /* Initialize from the real preceding record, preserving unowned hole data. */
    ntm_round_prepare_state *p=&a->base.round_prepare;unsigned i,bank;int result;
    for(bank=0;bank<2;bank++)for(i=0;i<64;i++)a->hole.players[bank][i]=p->players[bank][i];
    for(i=0;i<36;i++)a->hole.order[i]=p->course_order[i];
    a->hole.hole[3]=p->reset_kind;a->hole.hole[4]=p->mapped_course;a->hole.hole[5]=p->round_index;
    a->hole.hole[6]=p->hole_index;a->hole.hole[9]=p->course_order_kind;
    a->hole.hole[11]=a->base.selector.driver.parent.course_index;
    for(i=0;i<3;i++)a->hole.hole[12+i]=a->base.selector.driver.parent.course_order[i];
    a->context.course=p->mapped_course;a->context.round_index=p->round_index;a->context.hole_index=p->hole_index;
    a->context.reset_kind=p->reset_kind;
    a->overview_update=a->base.selector.driver.updates+1;
    /* Use the local sound adapter without replacing the user's observer. */
    {ntm_tee_setup_context context=a->context;
     context.sound_request=tee_sound;context.sound_context=a;
     result=ntm_tee_setup_initialize(&a->overview,t,&context,a->resources);
     a->context.course=context.course;a->context.round_index=context.round_index;a->context.hole_index=context.hole_index;
     a->context.par=context.par;a->context.distance=context.distance;a->context.pin_x=context.pin_x;a->context.pin_y=context.pin_y;
     a->context.origin_x=context.origin_x;a->context.origin_y=context.origin_y;a->context.text_base=context.text_base;}
    return tee_problem(a,result);
}
static int tee_owner_tick(ntm_task_context *c,ntm_task *t)
{
    ntm_tee_setup_application *a=(ntm_tee_setup_application *)c;int result;
    result=ntm_tee_owner_tick(&a->owner,t,c,&a->base.fallback);
    if(result<0)return -1;
    if(result){a->boundary=NTM_TEE_BOUNDARY_SHOT_SCENE;a->base.selector.driver.running=0;}
    return 0;
}
static int tee_owner_wait(ntm_task_context *c,ntm_task *t){return tee_owner_tick(c,t);}
static int tee_outer_wait(ntm_task_context *c,ntm_task *t)
{
    (void)c;if(!t->next)return -1;
    return t->priority<t->next->priority?0:-1; /* Later outer path unconverted. */
}
static int tee_allocate(ntm_task_context *c,ntm_task *t)
{
    ntm_tee_setup_application *a=(ntm_tee_setup_application *)c;ntm_course_application *base=&a->base.selector;
    ntm_course_cursor_engine *old=&base->cursor_engine;unsigned i,j,k;
    if(!t->next || base->display.zero_flags[10])return -1;
    a->objects[0]=a->base.round_cursor;a->x[0]=a->base.round_x;
    a->engine.frame[0]=old->frame;a->engine.delay[0]=old->delay;
    for(i=0;i<4;i++)a->engine.bounds[0][i]=old->bounds[i];
    for(i=0;i<2;i++)for(j=0;j<256;j++)a->engine.bank1[i][j]=old->bank1[i][j];
    for(i=0;i<2;i++)for(j=0;j<2;j++)for(k=0;k<64;k++)a->engine.palettes[i][j][k]=old->palettes[i][j][k];
    a->engine.list_dirty_high=old->list_dirty_high;a->engine.uploaded_bank=old->uploaded_bank;
    for(i=0;i<32;i++){a->objects[i].id=base->display.object_ids[i];a->objects[i].tag=base->display.object_tags[i];}
    a->owner.course=a->base.round_prepare.mapped_course;
    (void)ntm_task_allocate(c,t,tee_owner_tick,&a->base.fallback);t->callback=tee_outer_wait;
    a->started=1;a->start_update=base->driver.updates+1;return 0;
}
static int tee_drawing_head(void *opaque)
{
    ntm_tee_setup_application *a=opaque;ntm_course_application *b=&a->base.selector;
    if(!a->started)return a->previous_head(opaque);
    if((b->initialization.flags&80)==80 && ntm_tee_objects_head(&a->engine,&b->display,b->tiles,b->sprites,b->sprite_palettes)<0)return -1;
    b->initialization.flags&=63;return 0;
}
static int tee_tail(void *opaque)
{
    ntm_tee_setup_application *a=opaque;ntm_course_application *b=&a->base.selector;
    if(!a->started)return a->previous_tail(opaque);
    if(ntm_course_palette_tail(&b->palette)<0)return -1;
    if((b->initialization.flags&144)==16) {
        if(ntm_tee_objects_tail(&a->engine,a->objects,&b->display,a->x,b->initialization.screen_words,
            &b->initialization.flags,a->object_resources)<0)return -1;
        b->initialization.flags|=192;
    }
    return 0;
}
int ntm_tee_setup_application_start(ntm_tee_setup_application *a,const ntm_course_resources *selector,
    const ntm_course_intro_resources courses[3],const ntm_course_title_resources titles[3],
    const ntm_cursor_assets title_objects[3],const ntm_round_record_resources *round,
    const ntm_tee_setup_resources *resources,const ntm_hole_prepare_resources *holes)
{
    unsigned i;int result;ntm_tee_setup_context *c;
    if(!a || !resources || !holes || a->started || a->overview.phase || a->owner.phase)return -1;
    result=ntm_course_intro_application_start_round_record(&a->base,selector,courses,titles,title_objects,round);if(result)return result;
    a->resources=resources;a->hole_resources=holes;
    a->hole.difficulty=2;a->hole.selected_difficulty=0;
    a->base.tee_continuation=tee_allocate;
    a->previous_head=a->base.selector.driver.drawing_head;a->previous_tail=a->base.selector.driver.service_tail;
    a->base.selector.driver.drawing_head=tee_drawing_head;a->base.selector.driver.service_tail=tee_tail;
    a->owner.overview_entry=tee_overview_entry;a->owner.overview_wait=tee_owner_wait;
    a->owner.sound_request=tee_sound;a->owner.sound_context=a;
    a->overview.notice_entry=tee_notice_entry;a->overview.notice_wait=tee_tick;
    a->overview.setup_delay=tee_delay;a->overview.open_delay=tee_open_delay;a->overview.hold=tee_hold;
    a->overview.handicap_entry=tee_handicap_entry;a->overview.handicap_wait=tee_wait_handicap;
    a->handicap.wait=tee_handicap_wait;a->handicap.cleanup=tee_handicap_cleanup;
    for(i=0;i<32;i++)a->object_resources[i]=resources->objects;
    c=&a->context;c->intro=&a->base.intro_context;c->tasks=&a->base.selector.driver.context;c->fallback=&a->base.fallback;
    c->objects=a->objects;c->x=a->x;c->tiles=a->base.selector.tiles;c->prepare=tee_prepare;c->prepare_context=a;
    if(!c->saved_hole_scores){for(i=0;i<54;i++)a->saved_hole_scores[i]=10;c->saved_hole_scores=a->saved_hole_scores;}
    if(!c->handicaps)c->handicaps=a->handicaps;
    c->language=1;return 0;
}
int ntm_tee_setup_application_update(ntm_tee_setup_application *a,uint8_t buttons)
{return a?ntm_course_intro_application_update(&a->base,buttons):-1;}
