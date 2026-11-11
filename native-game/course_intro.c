/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_intro.h"
int ntm_course_records_normal(ntm_course_records *r)
{
    unsigned i,index;
    if(!r)return -1;
    index=r->golfer[r->active_player!=0];if(index>=6)return -1;
    for(i=0;i<6;i++)r->active[i]=0;
    r->count[index]=(uint16_t)(r->count[index]+1u);r->active[index]=255;
    return 0;
}
int ntm_course_records_prepare(ntm_course_records *r)
{
    if(!r)return -1;
    if(r->record_kind[r->active_player!=0]==6)return 1;
    return ntm_course_records_normal(r);
}
static int owner_accept(ntm_task_context *c,ntm_task *t,ntm_course_owner *s,
    ntm_course_records *r,ntm_task *fallback,int special_done)
{
    int result;
    if(!c || !t || !t->next || !s || !s->intro_entry || !s->wait_intro || !fallback)return -1;
    result=special_done?ntm_course_records_normal(r):ntm_course_records_prepare(r);
    if(result<0)return -1;
    if(result){s->phase=NTM_OWNER_SPECIAL_PENDING;return NTM_OWNER_SPECIAL_RECORD;}
    (void)ntm_task_allocate(c,t,s->intro_entry,fallback);
    t->callback=s->wait_intro;s->phase=NTM_OWNER_INTRO_WAIT;
    return NTM_OWNER_NONE;
}
int ntm_course_owner_tick(ntm_task_context *c,ntm_task *t,ntm_course_owner *s,
    ntm_course_records *r,ntm_task *fallback)
{
    if(!c || !t || !s)return -1;
    switch(s->phase) {
    case NTM_OWNER_SELECTOR_WAIT:
        if(t->result==249)return NTM_OWNER_NONE;
        if(t->result){s->phase=NTM_OWNER_PRIOR_PENDING;return NTM_OWNER_PRIOR_SELECTION;}
        return owner_accept(c,t,s,r,fallback,0);
    case NTM_OWNER_SPECIAL_PENDING:return NTM_OWNER_SPECIAL_RECORD;
    case NTM_OWNER_PRIOR_PENDING:return NTM_OWNER_PRIOR_SELECTION;
    case NTM_OWNER_INTRO_WAIT:
        if(!t->next || !s->wait_intro || t->callback!=s->wait_intro)return -1;
        if(t->priority<t->next->priority)return NTM_OWNER_NONE;
        s->phase=NTM_OWNER_ROUND_PENDING;return NTM_OWNER_ROUND_RECORD;
    case NTM_OWNER_ROUND_PENDING:return NTM_OWNER_ROUND_RECORD;
    default:return -1;
    }
}
int ntm_course_owner_special_done(ntm_task_context *c,ntm_task *t,
    ntm_course_owner *s,ntm_course_records *r,ntm_task *fallback)
{
    if(!s || s->phase!=NTM_OWNER_SPECIAL_PENDING)return -1;
    return owner_accept(c,t,s,r,fallback,1);
}
static int intro_context_valid(const ntm_course_intro *s,const ntm_task *t,
    const ntm_course_intro_context *c)
{
    return s && t && c && c->screen && c->display && c->palette && c->maps &&
        c->scroll_x && c->scroll_y && c->flags && c->viewport && c->screen_words &&
        s->scenery_delay && s->visible_delay && s->title;
}
static uint16_t intro_word(const uint8_t *p)
{return (uint16_t)(p[0]+256u*p[1]);}
int ntm_course_intro_initialize(ntm_course_intro *s,ntm_task *t,
    ntm_course_intro_context *c,const ntm_course_intro_resources *r)
{
    unsigned x,y;
    if(!intro_context_valid(s,t,c) || s->phase!=NTM_INTRO_NEW || !r ||
       !r->descriptor || r->descriptor_size<4 || c->screen->stamp_bytes>256 ||
       (c->screen->stamp_bytes&7) || !ntm_course_palette_job_valid(c->palette,r->palettes) ||
       !ntm_course_palette_job_valid(c->palette,r->palettes+1))return -1;
    c->scroll_x[0]=c->scroll_y[0]=c->scroll_x[1]=c->scroll_y[1]=0;
    for(y=0;y<19;y++)for(x=0;x<40;x++)c->maps[64*y+x]=0;
    (void)ntm_display_reset(c->display);
    *c->flags=(uint8_t)((*c->flags&63)|16);
    c->screen_words[0]=c->screen_words[1]=0;
    (void)ntm_course_palette_enqueue(c->palette,r->palettes);
    (void)ntm_course_palette_enqueue(c->palette,r->palettes+1);
    s->asset_group=intro_word(r->descriptor);s->descriptor_next=r->descriptor+4;
    (void)ntm_course_screen_enqueue_stamp(c->screen,&r->background,0x800,intro_word(r->descriptor+2));
    t->delay=2;t->callback=s->scenery_delay;s->phase=NTM_INTRO_SCENERY_DELAY;
    return ntm_course_intro_tick(s,t,c);
}
int ntm_course_intro_tick(ntm_course_intro *s,ntm_task *t,
    ntm_course_intro_context *c)
{
    if(!intro_context_valid(s,t,c))return -1;
    if(s->phase==NTM_INTRO_TITLE_PENDING)return NTM_INTRO_TITLE;
    if((s->phase==NTM_INTRO_SCENERY_DELAY && t->callback!=s->scenery_delay) ||
       (s->phase==NTM_INTRO_VISIBLE_DELAY && t->callback!=s->visible_delay) ||
       (s->phase!=NTM_INTRO_SCENERY_DELAY && s->phase!=NTM_INTRO_VISIBLE_DELAY))return -1;
    t->delay=(uint16_t)(t->delay-1u);
    if(!(t->delay&0x8000))return NTM_INTRO_NONE;
    if(s->phase==NTM_INTRO_SCENERY_DELAY) {
        c->viewport[0]=c->viewport[1]=0;c->viewport[2]=160;c->viewport[3]=152;
        t->delay=19;t->callback=s->visible_delay;s->phase=NTM_INTRO_VISIBLE_DELAY;
        return NTM_INTRO_NONE;
    }
    t->callback=s->title;s->phase=NTM_INTRO_TITLE_PENDING;return NTM_INTRO_TITLE;
}
