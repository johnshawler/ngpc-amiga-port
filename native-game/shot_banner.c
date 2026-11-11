/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_banner.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static int valid(const ntm_shot_banner_context *c,const ntm_shot_banner_resources *r)
{
    const ntm_tee_setup_context *t;const ntm_course_intro_context *i;unsigned k;
    if(!c || !r || !c->hole || !c->player || c->player->active_player>1 || !(t=c->shared) ||
       !(i=t->intro) || !i->screen || !i->display || !i->palette || !i->maps || !i->flags ||
       i->screen->stamp_bytes>256 || (i->screen->stamp_bytes&7) || !i->viewport || !i->scroll_x || !i->scroll_y || !i->screen_words || !t->tasks || !t->fallback ||
       !t->tiles || !t->objects || !t->x || !c->exit_palette_request || !c->raster || !r->metadata || r->metadata_size<5)return 0;
    for(k=0;k<2;k++)if(!ntm_course_palette_job_valid(i->palette,r->palettes+k))return 0;
    return 1;
}
static int initial_tee(const ntm_shot_banner_context *c)
{
    const uint8_t *p=c->hole->players[c->player->active_player];
    return !c->shared->game_mode && c->hole->hole[0x70]==4 && p[0]<=1 && p[1]<6 && p[2]==1 && p[3]==0;
}
static void clear(uint8_t *maps,unsigned at,unsigned columns,unsigned rows)
{unsigned x,y;for(y=0;y<rows;y++)for(x=0;x<2*columns;x++)maps[at+64*y+x]=0;}
static int finish(ntm_task *t,ntm_shot_banner_context *c,int descendants,uint8_t *outcome)
{
    int result;ntm_task_context *ctx=c->shared->tasks;
    if(!t || !t->next || !t->previous || ctx->current!=t)return -1;
    if(descendants && ntm_task_remove_descendants(ctx,t)<0)return -1;
    result=ntm_task_finish(ctx,t);if(result<0)return -1;
    if(outcome)*outcome=(uint8_t)result;
    return 0;
}
static int stamp(ntm_shot_banner_context *c,ntm_screen_asset asset,uint16_t offset,uint16_t tile)
{return ntm_course_screen_enqueue_stamp(c->shared->intro->screen,&asset,offset,tile)<0?-1:0;}
int ntm_shot_banner_initialize(ntm_shot_banner *s,ntm_task *t,ntm_shot_banner_context *c,const ntm_shot_banner_resources *r)
{
    ntm_tee_setup_context *q;ntm_course_intro_context *i;const uint8_t *p;
    if(!s || !t || !valid(c,r) || s->phase || !s->delay || !s->animation_wait || !s->hold || !s->exit_wait ||
       !s->notice_entry || !s->animation_entry || !s->exit_entry)return -1;
    if(!initial_tee(c))return NTM_TEE_UNSUPPORTED;
    q=c->shared;i=q->intro;p=c->hole->players[c->player->active_player];
    if(!c->suppress_sound && q->sound_request)q->sound_request(q->sound_context,0,65534,0);
    s->blink_delay=1;s->blink_state=0;i->screen_words[0]=i->screen_words[1]=0;
    if(ntm_tee_objects_reset(i->display,q->objects)<0)return -1;
    *i->flags=(uint8_t)((*i->flags&63)|16);
    i->scroll_x[0]=i->scroll_x[1]=i->scroll_y[0]=i->scroll_y[1]=0;
    clear(i->maps,0,32,19);clear(i->maps,0x800,20,19);
    (void)ntm_course_palette_enqueue(i->palette,r->palettes);(void)ntm_course_palette_enqueue(i->palette,r->palettes+1);
    if(stamp(c,r->terrain,0x800,0xf0)<0 || stamp(c,r->characters[p[1]],0x104,0x40)<0 ||
       stamp(c,r->players[p[0]],0x10c,p[0]?0x50:0x250)<0 ||
       stamp(c,r->tee_shot,0x18c,p[0]?0x70:0x270)<0)return -1;
    t->delay=1;t->callback=s->delay;s->phase=NTM_SHOT_BANNER_DELAY;return 0;
}
static int blink(ntm_shot_banner *s,ntm_shot_banner_context *c,const ntm_shot_banner_resources *r)
{
    s->blink_delay=(uint8_t)(s->blink_delay-1);if(s->blink_delay)return 0;
    s->blink_state^=1;
    if(s->blink_state){s->blink_delay=16;return stamp(c,r->hints[c->shared->language!=0],0x400,0xd0);}
    s->blink_delay=8;clear(c->shared->intro->maps,0x400,20,2);return 0;
}
int ntm_shot_banner_tick(ntm_shot_banner *s,ntm_task *t,ntm_shot_banner_context *c,const ntm_shot_banner_resources *r)
{
    ntm_tee_setup_context *q;ntm_course_intro_context *i;
    if(!s || !t || !valid(c,r))return -1;
    q=c->shared;i=q->intro;
    if(s->phase==NTM_SHOT_BANNER_DONE)return NTM_TEE_COMPLETE;
    if(s->phase==NTM_SHOT_BANNER_DELAY && t->callback==s->delay){
        t->delay=(uint16_t)(t->delay-1);if(!(t->delay&32768))return 0;
        i->viewport[0]=i->viewport[1]=0;i->viewport[2]=160;i->viewport[3]=152;
        (void)ntm_task_allocate(q->tasks,t,s->notice_entry,q->fallback);(void)ntm_task_allocate(q->tasks,t,s->animation_entry,q->fallback);
        t->result=255;t->callback=s->animation_wait;s->phase=NTM_SHOT_BANNER_ANIMATION_WAIT;return 0;
    }
    if(s->phase==NTM_SHOT_BANNER_ANIMATION_WAIT && t->callback==s->animation_wait){
        if(t->result==255)return 0;
        t->delay=149;t->callback=s->hold;s->phase=NTM_SHOT_BANNER_HOLD;return 0;
    }
    if(s->phase==NTM_SHOT_BANNER_HOLD && t->callback==s->hold){
        if(!ntm_course_title_skip(q->control,q->input_mode,q->linked_controls)){
            if(!q->game_mode){if(q->control&64)return NTM_TEE_UNSUPPORTED;if(blink(s,c,r)<0)return -1;}
            t->delay=(uint16_t)(t->delay-1);if(!(t->delay&32768))return 0;
        }
        s->blink_delay=8;clear(i->maps,0x400,20,2);
        (void)ntm_task_allocate(q->tasks,t,s->exit_entry,q->fallback);t->result=255;
        t->callback=s->exit_wait;s->phase=NTM_SHOT_BANNER_EXIT_WAIT;return 0;
    }
    if(s->phase==NTM_SHOT_BANNER_EXIT_WAIT && t->callback==s->exit_wait){
        if(t->result==255)return 0;
        if(!t->owner || !t->previous || !t->next || q->tasks->current!=t)return -1;
        if(ntm_shot_raster_remove(c->raster)<0)return -1;
        i->scroll_x[0]=i->scroll_y[0]=0;i->viewport[0]=i->viewport[1]=i->viewport[2]=0;i->viewport[3]=152;
        if(ntm_tee_objects_reset(i->display,q->objects)<0)return -1;
        i->screen_words[0]&=0xff00;i->screen_words[1]&=0xff00;
        t->owner->result=255;if(finish(t,c,1,&s->finish_outcome)<0)return -1;
        s->phase=NTM_SHOT_BANNER_DONE;return NTM_TEE_COMPLETE;
    }
    return -1;
}
int ntm_shot_banner_notice(ntm_task *t,ntm_shot_banner_context *c)
{
    if(!c || !c->shared || !c->shared->tasks || !c->hole || c->hole->hole[0x70]==6)return -1;
    return finish(t,c,0,0);
}
int ntm_shot_banner_animation_initialize(ntm_shot_banner_animation *s,ntm_task *t,
    ntm_shot_banner_context *c,const ntm_shot_banner_resources *r)
{
    ntm_course_cursor *o;ntm_tee_setup_context *q;const uint8_t *p;
    if(!s || !t || !valid(c,r) || !s->wait || s->phase)return -1;
    if(!initial_tee(c))return NTM_TEE_UNSUPPORTED;
    q=c->shared;p=c->hole->players[c->player->active_player];s->inactive=(uint8_t)(p[0]!=c->controlling_player?255:0);s->variant=88;
    o=q->objects+11;o->tag=24;q->intro->display->object_tags[11]=24;o->width=word(r->metadata);o->velocity=word(r->metadata+2);o->byte15=r->metadata[4];
    q->x[11]=64;o->y=80;o->second=255;o->byte14=0;o->tile=176;o->flags=128;
    if(ntm_tee_object_register(q->intro->display,o)<0)return -1;
    o->velocity=152;if(s->inactive)o->y=65456;
    t->callback=s->wait;s->phase=1;return 0;
}
int ntm_shot_banner_animation_tick(ntm_shot_banner_animation *s,ntm_task *t,
    ntm_shot_banner_context *c,const ntm_shot_banner_resources *r)
{
    ntm_course_cursor *o;
    if(!s || !t || !valid(c,r))return -1;
    if(s->phase==2)return NTM_TEE_COMPLETE;
    if(s->phase!=1 || t->callback!=s->wait)return -1;
    o=c->shared->objects+11;if(!(o->byte30&64))return 0;
    if(!t->owner)return -1;
    if(!s->inactive){if(stamp(c,r->portrait,0x290,0xa0)<0)return -1;o->velocity=(uint16_t)(s->variant+60);o->flags=128;}
    t->owner->result=0;if(finish(t,c,0,0)<0)return -1;s->phase=2;return NTM_TEE_COMPLETE;
}
int ntm_shot_banner_exit_initialize(ntm_shot_banner_exit *s,ntm_task *t,
    ntm_shot_banner_context *c,const ntm_shot_banner_resources *r)
{
    ntm_course_intro_context *i;ntm_course_cursor *o;
    if(!s || !t || !valid(c,r) || !s->tick || s->phase)return -1;
    i=c->shared->intro;o=c->shared->objects+11;c->raster->lower_scroll=c->raster->upper_scroll=0;
    o->velocity=156;o->flags=128;clear(i->maps,0x290,4,4);
    if(c->exit_palette_request(c->exit_palette_context,0x6c)<0 || c->exit_palette_request(c->exit_palette_context,0x70)<0)return -1;
    if(ntm_shot_raster_install(c->raster)<0)return -1;
    t->delay=20;t->callback=s->tick;s->phase=1;return 0;
}
int ntm_shot_banner_exit_tick(ntm_shot_banner_exit *s,ntm_task *t,ntm_shot_banner_context *c)
{
    unsigned at,y;uint8_t *m;
    if(!s || !t || !c || !c->shared || !c->shared->intro || !c->raster)return -1;
    if(s->phase==2)return NTM_TEE_COMPLETE;
    if(s->phase!=1 || t->callback!=s->tick)return -1;
    t->delay=(uint16_t)(t->delay-1);
    if(!t->delay){if(!t->owner)return -1;t->owner->result=0;if(finish(t,c,0,0)<0)return -1;s->phase=2;return NTM_TEE_COMPLETE;}
    c->raster->upper_scroll=(uint8_t)(c->raster->upper_scroll+12);at=0x100+2u*((uint8_t)(c->raster->upper_scroll-16)>>3);m=c->shared->intro->maps;
    for(y=0;y<4;y++){m[at+64*y]=m[at+64*y+1]=m[at+64*y+2]=m[at+64*y+3]=0;}
    return 0;
}
