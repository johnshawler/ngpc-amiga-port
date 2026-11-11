/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_title.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
int ntm_course_title_skip(uint8_t control,uint8_t mode,const uint8_t linked[2])
{
    if(mode){if(!linked)return -1;control=(uint8_t)(linked[0]|linked[1]);}
    return (control&16)!=0;
}
static int valid(const ntm_course_title *s,const ntm_task *t,
    const ntm_course_title_context *c,const ntm_course_title_resources *r)
{
    const ntm_course_intro_context *i;
    if(!s || !t || !c || !r || !(i=c->intro))return 0;
    return i->screen && i->display && i->maps && i->viewport && i->screen_words &&
        c->cursor && c->cursor_x && c->tasks && r->descriptor && r->descriptor_size>=12 &&
        r->metadata && r->metadata_size>=5 && s->title_wait && s->tournament_delay &&
        s->tournament_wait && s->cleanup && i->screen->stamp_bytes<=256 && !(i->screen->stamp_bytes&7);
}
int ntm_course_title_initialize(ntm_course_title *s,ntm_task *t,
    ntm_course_title_context *c,const ntm_course_title_resources *r,const ntm_course_intro *intro)
{
    ntm_course_cursor *o;ntm_display_reset_state *d;unsigned offset;
    if(!valid(s,t,c,r) || !intro || s->phase!=NTM_TITLE_NEW ||
       intro->phase!=NTM_INTRO_TITLE_PENDING || t->callback!=intro->title ||
       intro->descriptor_next!=r->descriptor+4)return -1;
    d=c->intro->display;offset=d->zero_words[0];
    if(d->zero_flags[8]<32 && ((offset&1) || offset>=64))return -1;
    s->asset_group=intro->asset_group;s->descriptor_next=r->descriptor+10;
    (void)ntm_course_screen_enqueue_stamp(c->intro->screen,&r->title,0x188,word(r->descriptor+4));
    o=c->cursor;o->id=d->object_ids[0];o->tag=38;d->object_tags[0]=38;
    o->width=word(r->metadata);o->velocity=word(r->metadata+2);o->byte15=r->metadata[4];
    *c->cursor_x=0;o->y=0;o->second=255;o->byte14=0;o->tile=word(r->descriptor+6);o->flags=128;
    /* Original2007ca silently rejects a full list, including its final object
     * writes. The caller still applies position and animation index afterward. */
    if(d->zero_flags[8]<32) {
        d->list[offset/2]=(uint16_t)(o->id+256u*o->second);
        d->zero_words[0]=(uint16_t)(offset+2);d->zero_flags[8]++;
        d->zero_flags[9]|=1;o->flags|=128;o->byte30=0;o->link=65535;
    }
    *c->cursor_x=(uint16_t)(40u+c->intro->screen_words[0]);
    o->y=(uint16_t)(48u+c->intro->screen_words[1]);o->width=word(r->descriptor+8);
    t->delay=59;t->callback=s->title_wait;s->phase=NTM_TITLE_WAIT;return NTM_TITLE_NONE;
}
static void reset(ntm_course_title_context *c)
{
    (void)ntm_display_reset(c->intro->display);
    /* Original reset clears only low bytes of the two shared scroll words. */
    c->intro->screen_words[0]&=0xff00;c->intro->screen_words[1]&=0xff00;
    c->cursor->id=0;c->cursor->tag=65535;
}
int ntm_course_title_tick(ntm_course_title *s,ntm_task *t,
    ntm_course_title_context *c,const ntm_course_title_resources *r)
{
    unsigned x,y;
    if(!valid(s,t,c,r))return -1;
    if(s->phase==NTM_TITLE_DONE)return NTM_TITLE_COMPLETE;
    if((s->phase==NTM_TITLE_WAIT && t->callback!=s->title_wait) ||
       (s->phase==NTM_TITLE_TOURNAMENT_DELAY && t->callback!=s->tournament_delay) ||
       (s->phase==NTM_TITLE_TOURNAMENT_WAIT && t->callback!=s->tournament_wait) ||
       s->phase<NTM_TITLE_WAIT || s->phase>NTM_TITLE_TOURNAMENT_WAIT)return -1;
    if(s->phase==NTM_TITLE_TOURNAMENT_DELAY || !ntm_course_title_skip(c->control,c->input_mode,c->linked_controls)) {
        t->delay=(uint16_t)(t->delay-1u);
        if(!(t->delay&32768))return NTM_TITLE_NONE;
    }
    if(s->phase==NTM_TITLE_WAIT) {
        reset(c);
        for(y=0;y<8;y++)for(x=0;x<24;x++)c->intro->maps[0x188+64*y+x]=0;
        (void)ntm_course_screen_enqueue_stamp(c->intro->screen,r->tournament+(c->game_mode==2),0x180,word(r->descriptor+10));
        s->descriptor_next=r->descriptor+12;t->delay=1;
        t->callback=s->tournament_delay;s->phase=NTM_TITLE_TOURNAMENT_DELAY;return NTM_TITLE_NONE;
    }
    if(s->phase==NTM_TITLE_TOURNAMENT_DELAY) {
        c->intro->viewport[0]=c->intro->viewport[1]=0;c->intro->viewport[2]=160;c->intro->viewport[3]=152;
        t->delay=59;t->callback=s->tournament_wait;s->phase=NTM_TITLE_TOURNAMENT_WAIT;return NTM_TITLE_NONE;
    }
    if(!t->previous || !t->next || c->tasks->current!=t)return -1;
    t->callback=s->cleanup;
    c->intro->viewport[0]=c->intro->viewport[1]=c->intro->viewport[2]=0;c->intro->viewport[3]=152;
    reset(c);
    if(ntm_task_remove_descendants(c->tasks,t)<0)return -1;
    {int outcome=ntm_task_finish(c->tasks,t);if(outcome<0)return -1;s->finish_outcome=(uint8_t)outcome;}
    s->phase=NTM_TITLE_DONE;return NTM_TITLE_COMPLETE;
}
