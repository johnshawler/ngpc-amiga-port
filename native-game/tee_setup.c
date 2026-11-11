/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "tee_setup.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static int valid(const ntm_tee_setup_context *c,const ntm_tee_setup_resources *r)
{
    ntm_course_intro_context *i;unsigned k;
    if(!c || !r || !(i=c->intro) || !i->screen || !i->display || !i->palette || !i->maps ||
       !i->scroll_x || !i->scroll_y || !i->flags || !i->viewport || !i->screen_words ||
       !c->tasks || !c->fallback || !c->objects || !c->x || !c->tiles ||
       c->game_mode || c->course>=3 || c->round_index>=18 || c->hole_index>=18 ||
       !c->saved_hole_scores || !c->handicaps || !r->objects ||
       i->screen->stamp_bytes>256 || (i->screen->stamp_bytes&7))return 0;
    for(k=0;k<4;k++)if(!r->metadata[k] || r->metadata_size[k]<5)return 0;
    return 1;
}
static int active_hole(const ntm_tee_setup_context *c)
{
    uint8_t badge;
    if(c->course>=3 || c->round_index>=18 || c->hole_index>=18 || c->par<3 || c->par>5)return 0;
    badge=ntm_tee_badge_offset(c->saved_hole_scores[18u*c->course+c->round_index],c->par);
    return badge==255 || (badge<=12 && !(badge&3));
}
uint8_t ntm_tee_badge_offset(uint8_t saved,uint8_t par)
{
    uint8_t diff;if(saved==1)return 0;
    diff=(uint8_t)(saved-par);
    return !(diff&128)?255:(uint8_t)((diff+4u)*4u);
}
static void reset(ntm_tee_setup_context *c)
{
    unsigned i;(void)ntm_display_reset(c->intro->display);
    c->intro->screen_words[0]&=0xff00;c->intro->screen_words[1]&=0xff00;
    for(i=0;i<32;i++){c->objects[i].id=(uint8_t)i;c->objects[i].tag=65535;}
}
static int finish(ntm_task *t,ntm_tee_setup_context *c,uint8_t *outcome)
{
    int result;if(!t->previous || !t->next || c->tasks->current!=t)return -1;
    if(ntm_task_remove_descendants(c->tasks,t)<0)return -1;
    result=ntm_task_finish(c->tasks,t);if(result<0)return -1;
    if(outcome)*outcome=(uint8_t)result;
    return 0;
}
static int initialize_object(ntm_tee_setup_context *c,const ntm_tee_setup_resources *r,
    unsigned id,uint16_t tag,uint16_t x,uint16_t y,uint16_t tile)
{
    ntm_course_cursor *o=c->objects+id;
    o->id=c->intro->display->object_ids[id];o->tag=tag;c->intro->display->object_tags[id]=tag;
    o->width=word(r->metadata[id]);o->velocity=word(r->metadata[id]+2);o->byte15=r->metadata[id][4];
    c->x[id]=x;o->y=y;o->second=0;o->byte14=0;o->tile=tile;o->flags=128;
    return ntm_tee_object_register(c->intro->display,o)<0?-1:0;
}
int ntm_tee_setup_initialize(ntm_tee_setup *s,ntm_task *t,ntm_tee_setup_context *c,
    const ntm_tee_setup_resources *r)
{
    ntm_course_intro_context *i;ntm_screen_stamp stamp;unsigned x,y;
    if(!s || !t || !valid(c,r) || s->phase || !s->notice_entry || !s->notice_wait ||
       !s->setup_delay || !s->open_delay || !s->hold || !s->handicap_entry || !s->handicap_wait ||
       !c->prepare || !t->next || !ntm_course_palette_job_valid(c->intro->palette,r->palettes) ||
       !ntm_course_palette_job_valid(c->intro->palette,r->palettes+1))return -1;
    i=c->intro;
    if(c->sound_request){c->sound_request(c->sound_context,1,65535,8);c->sound_request(c->sound_context,1,65534,8);}
    i->screen_words[0]=i->screen_words[1]=0;
    i->scroll_x[0]=i->scroll_x[1]=i->scroll_y[0]=i->scroll_y[1]=0;
    for(y=0;y<19;y++)for(x=0;x<40;x++)i->maps[64*y+x]=0;
    c->text_base=0;
    if(ntm_tee_copy_tiles(&r->font,0x100,c->tiles)<0)return -1;
    (void)ntm_course_palette_enqueue(i->palette,r->palettes);
    (void)ntm_course_palette_enqueue(i->palette,r->palettes+1);
    if(c->prepare(c->prepare_context,c)<0)return -1;
    if(!active_hole(c))return NTM_TEE_UNSUPPORTED;
    stamp.asset=r->header;stamp.offset=0x800;stamp.tile=0x50;
    if(ntm_course_screen_stamp(&stamp,c->tiles,i->maps)<0)return -1;
    (void)ntm_course_screen_enqueue_stamp(i->screen,r->hole_numbers+c->hole_index,0x52,0x640);
    if(ntm_tee_draw_number(c->distance,3,0x28,c->text_base,0x902,&i->display->reset_words[0],i->maps)<0)return -1;
    (void)ntm_course_screen_enqueue_stamp(i->screen,r->pars+c->par-3,0x94a,0x2846);
    (void)ntm_task_allocate(c->tasks,t,s->notice_entry,c->fallback);
    t->callback=s->notice_wait;s->phase=NTM_TEE_NOTICE_WAIT;return 0;
}
int ntm_tee_notice(ntm_task *t,ntm_tee_setup_context *c)
{
    /* Original20df6a immediately clears carry for game_mode<2. The normal
     * child therefore removes itself; it is not a missing match-play screen. */
    if(!t || !c || !c->tasks || c->game_mode)return -1;
    return finish(t,c,0);
}
int ntm_tee_setup_tick(ntm_tee_setup *s,ntm_task *t,ntm_tee_setup_context *c,
    const ntm_tee_setup_resources *r)
{
    ntm_course_intro_context *i;unsigned badge;
    if(!s || !t || !valid(c,r))return -1;
    if(s->phase==NTM_TEE_DONE)return NTM_TEE_COMPLETE;
    i=c->intro;
    if(s->phase==NTM_TEE_NOTICE_WAIT && t->callback==s->notice_wait) {
        const ntm_tee_hole_art *art;
        if(!t->next)return -1;
        if(t->priority<t->next->priority)return 0;
        if(!active_hole(c))return NTM_TEE_UNSUPPORTED;
        art=&r->holes[c->course][c->round_index];
        s->asset_group=(uint16_t)(4u*(c->course+36u));s->asset_index=(uint16_t)(4u*c->round_index);
        (void)ntm_course_screen_enqueue_stamp(i->screen,art->views,0xa46,0x70);
        (void)ntm_course_screen_enqueue_stamp(i->screen,art->views+1,0x246,0x2e0);
        (void)ntm_course_screen_enqueue_stamp(i->screen,art->views+2,0x858,0x550);
        reset(c);*i->flags=(uint8_t)((*i->flags&63)|16);
        if(initialize_object(c,r,0,30,96,8,400)<0 || initialize_object(c,r,1,29,0,0,432)<0 ||
           initialize_object(c,r,2,31,0,0,448)<0)return -1;
        badge=ntm_tee_badge_offset(c->saved_hole_scores[18u*c->course+c->round_index],c->par);
        if(badge!=255) {
            if(initialize_object(c,r,3,32,56,24,464)<0)return -1;
            c->objects[3].velocity=(uint16_t)(c->objects[3].velocity+badge);
        }
        c->x[1]=(uint16_t)(c->pin_x-c->origin_x+96u);c->objects[1].y=(uint16_t)(c->pin_y-c->origin_y+8u);
        c->x[2]=art->marker_x;c->objects[2].y=art->marker_y;
        t->delay=1;t->callback=s->setup_delay;s->phase=NTM_TEE_SETUP_DELAY;return 0;
    }
    if((s->phase==NTM_TEE_SETUP_DELAY && t->callback==s->setup_delay) ||
       (s->phase==NTM_TEE_OPEN_DELAY && t->callback==s->open_delay) ||
       (s->phase==NTM_TEE_HOLD && t->callback==s->hold)) {
        if(s->phase!=NTM_TEE_HOLD || !ntm_course_title_skip(c->control,c->input_mode,c->linked_controls)) {
            t->delay=(uint16_t)(t->delay-1u);if(!(t->delay&32768))return 0;
        }
        if(s->phase==NTM_TEE_SETUP_DELAY) {
            i->viewport[0]=i->viewport[1]=0;i->viewport[2]=160;i->viewport[3]=152;
            t->delay=3;t->callback=s->open_delay;s->phase=NTM_TEE_OPEN_DELAY;return 0;
        }
        if(s->phase==NTM_TEE_OPEN_DELAY) {
            t->delay=179;t->callback=s->hold;s->phase=NTM_TEE_HOLD;return 0;
        }
        if(!t->next)return -1;
        (void)ntm_task_allocate(c->tasks,t,s->handicap_entry,c->fallback);
        t->callback=s->handicap_wait;s->phase=NTM_TEE_HANDICAP_WAIT;return 0;
    }
    if(s->phase==NTM_TEE_HANDICAP_WAIT && t->callback==s->handicap_wait) {
        if(!t->next)return -1;
        if(t->priority<t->next->priority)return 0;
        if(!t->previous || c->tasks->current!=t)return -1;
        reset(c);i->viewport[0]=i->viewport[1]=i->viewport[2]=0;i->viewport[3]=152;
        if(finish(t,c,&s->finish_outcome)<0)return -1;
        s->phase=NTM_TEE_DONE;return NTM_TEE_COMPLETE;
    }
    return -1;
}
int ntm_tee_handicap_initialize(ntm_tee_handicap *s,ntm_task *t,
    ntm_tee_setup_context *c,const ntm_tee_setup_resources *r)
{
    if(!s || !t || !valid(c,r) || s->phase || !s->wait || !s->cleanup)return -1;
    /* Original20cf21 branches directly to removal unless game_mode==2.
     * Mode2 is outside this normal path, not an omitted handicap display. */
    if(finish(t,c,&s->finish_outcome)<0)return -1;
    s->phase=2;return NTM_TEE_COMPLETE;
}
int ntm_tee_handicap_tick(ntm_tee_handicap *s,ntm_task *t,ntm_tee_setup_context *c)
{
    if(!s || !t || !c || !c->tasks || c->game_mode)return -1;
    if(s->phase==2)return NTM_TEE_COMPLETE;
    if(s->phase!=1 || t->callback!=s->wait)return -1;
    t->delay=(uint16_t)(t->delay-1u);if(!(t->delay&32768))return 0;
    t->callback=s->cleanup;if(finish(t,c,&s->finish_outcome)<0)return -1;
    s->phase=2;return NTM_TEE_COMPLETE;
}
int ntm_tee_owner_tick(ntm_tee_owner *s,ntm_task *t,ntm_task_context *c,ntm_task *fallback)
{
    if(!s || !t || !c || !fallback || !s->overview_entry || !s->overview_wait || s->course>=3)return -1;
    if(s->phase==2)return 1;
    if(!t->next)return -1;
    if(!s->phase) {
        if(!s->skip_overview) {
            (void)ntm_task_allocate(c,t,s->overview_entry,fallback);
            t->callback=s->overview_wait;s->phase=1;return 0;
        }
    }else if(s->phase==1 && t->callback==s->overview_wait) {
        if(t->priority<t->next->priority)return 0;
        s->player_status=0;
    }else return -1;
    s->skip_overview=0;
    if(s->current_music!=s->course && s->sound_request) {
        s->sound_request(s->sound_context,0,65535,s->course);s->sound_request(s->sound_context,0,65534,s->course);
    }
    s->phase=2;return 1;
}
