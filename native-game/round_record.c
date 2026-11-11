/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "round_record.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static int valid(const ntm_round_record_context *c,const ntm_round_record_resources *r)
{
    const ntm_course_intro_context *i;
    return c && r && (i=c->intro) && i->screen && i->display && i->palette &&
        i->maps && i->viewport && i->scroll_x && i->scroll_y && i->flags && i->screen_words &&
        c->tasks && c->fallback && c->cursor && c->cursor_x && c->tiles &&
        !c->game_mode && !c->alternate && !c->record_mode && !c->record_kind && c->score==255 &&
        c->golfer<6 && c->course<3 && r->metadata && r->metadata_size>=5 &&
        r->missing_score && r->missing_score_size && r->object &&
        i->screen->stamp_bytes<=256 && !(i->screen->stamp_bytes&7);
}
static void reset(ntm_round_record_context *c)
{
    (void)ntm_display_reset(c->intro->display);
    c->intro->screen_words[0]&=0xff00;c->intro->screen_words[1]&=0xff00;
    c->cursor->id=0;c->cursor->tag=65535;
}
static void sound(ntm_round_record_context *c,uint16_t command)
{if(c->sound_request)c->sound_request(c->sound_context,command);}
int ntm_round_record_initialize(ntm_round_record *s,ntm_task *t,
    ntm_round_record_context *c,const ntm_round_record_resources *r)
{
    ntm_course_intro_context *i;
    if(!s || !t || !valid(c,r) || s->phase || !s->drawing_entry || !s->blink_entry ||
       !s->allocate_blink || !s->open || !s->wait || !s->cleanup || !t->next ||
       !ntm_course_palette_job_valid(c->intro->palette,r->palettes) ||
       !ntm_course_palette_job_valid(c->intro->palette,r->palettes+1))return -1;
    i=c->intro;sound(c,65533);
    if(c->prepare && c->prepare(c->prepare_context)<0)return -1;
    c->player_markers[0]=1;c->player_markers[1]=0;
    reset(c);*i->flags=(uint8_t)((*i->flags&63)|16);
    i->screen_words[0]=i->screen_words[1]=0;
    i->scroll_x[0]=i->scroll_y[0]=i->scroll_x[1]=i->scroll_y[1]=0;
    (void)ntm_course_palette_enqueue(i->palette,r->palettes);
    (void)ntm_course_palette_enqueue(i->palette,r->palettes+1);
    c->text_mode=1;c->text_base=0x4c0;
    {ntm_screen_stamp stamp={r->font,0x4c0,0xe0};if(ntm_course_screen_stamp(&stamp,c->tiles,i->maps)<0)return -1;}
    (void)ntm_task_allocate(c->tasks,t,s->drawing_entry,c->fallback);
    t->callback=s->allocate_blink;s->phase=NTM_ROUND_ALLOCATE_BLINK;return 0;
}
int ntm_round_record_tick(ntm_round_record *s,ntm_task *t,
    ntm_round_record_context *c,const ntm_round_record_resources *r)
{
    if(!s || !t || !valid(c,r))return -1;
    if(s->phase==NTM_ROUND_DONE)return NTM_ROUND_COMPLETE;
    if(s->phase==NTM_ROUND_ALLOCATE_BLINK && t->callback==s->allocate_blink) {
        if(!t->next)return -1;
        (void)ntm_task_allocate(c->tasks,t,s->blink_entry,c->fallback);
        t->callback=s->open;s->phase=NTM_ROUND_OPEN;return 0;
    }
    if(s->phase==NTM_ROUND_OPEN && t->callback==s->open) {
        sound(c,65535);sound(c,65534);
        c->intro->viewport[0]=c->intro->viewport[1]=0;c->intro->viewport[2]=160;c->intro->viewport[3]=152;
        t->delay=239;t->callback=s->wait;s->phase=NTM_ROUND_WAIT;return 0;
    }
    if(s->phase!=NTM_ROUND_WAIT || t->callback!=s->wait)return -1;
    if(!ntm_course_title_skip(c->control,c->input_mode,c->linked_controls)) {
        t->delay=(uint16_t)(t->delay-1u);if(!(t->delay&32768))return 0;
    }
    if(!t->previous || !t->next || c->tasks->current!=t)return -1;
    t->callback=s->cleanup;
    c->intro->viewport[0]=c->intro->viewport[1]=c->intro->viewport[2]=0;c->intro->viewport[3]=152;
    reset(c);
    if(ntm_task_remove_descendants(c->tasks,t)<0)return -1;
    {int outcome=ntm_task_finish(c->tasks,t);if(outcome<0)return -1;s->finish_outcome=(uint8_t)outcome;}
    s->phase=NTM_ROUND_DONE;return NTM_ROUND_COMPLETE;
}
int ntm_round_record_drawing_initialize(ntm_round_record_drawing *s,ntm_task *t,
    ntm_round_record_context *c,const ntm_round_record_resources *r)
{
    unsigned x,y;ntm_course_screen *q;
    if(!s || !t || !valid(c,r) || s->phase || !s->finish)return -1;
    q=c->intro->screen;
    for(y=0;y<19;y++)for(x=0;x<40;x++)c->intro->maps[64*y+x]=0;
    (void)ntm_course_screen_enqueue_stamp(q,&r->header,0,0x180);
    {ntm_screen_stamp stamp={r->footer,0x3c0,0x190};if(ntm_course_screen_stamp(&stamp,c->tiles,c->intro->maps)<0)return -1;}
    s->asset_group=(uint16_t)(4u*(c->course+12u));
    (void)ntm_course_screen_enqueue_stamp(q,&r->course[c->course][0],0x8c0,0x120);
    (void)ntm_course_screen_enqueue_stamp(q,&r->course[c->course][1],0x218,0x1ae);
    t->callback=s->finish;s->phase=1;return 0;
}
int ntm_round_record_drawing_finish(ntm_round_record_drawing *s,ntm_task *t,
    ntm_round_record_context *c,const ntm_round_record_resources *r)
{
    ntm_course_screen *q;
    if(!s || !t || !valid(c,r))return -1;
    if(s->phase==2)return 1;
    if(s->phase!=1 || t->callback!=s->finish || !t->previous || !t->next || c->tasks->current!=t)return -1;
    q=c->intro->screen;
    (void)ntm_course_screen_enqueue_stamp(q,&r->golfer[c->golfer][0],0x18c,0x40);
    (void)ntm_course_screen_enqueue_stamp(q,&r->golfer[c->golfer][1],0xb0c,0x80);
    (void)ntm_course_screen_enqueue_stamp(q,&r->labels[0],0x240,0x90);
    (void)ntm_course_screen_enqueue_stamp(q,&r->labels[1],0xb00,0xd0);
    if(ntm_task_finish(c->tasks,t)<0)return -1;
    s->phase=2;return 1;
}
/* Original201cbd mode1 uses glyphs in the live font map, NOT source tiles.
 * Reads happen now, after the preceding drawing head, not when stamp queued. */
static int missing_score(ntm_round_record_context *c,const ntm_round_record_resources *r)
{
    size_t p;unsigned out=0x450;uint8_t *maps=c->intro->maps;
    if(c->text_mode!=1 || c->text_base>4095)return -1;
    for(p=0;p<r->missing_score_size && r->missing_score[p]!=255;p++) {
        unsigned v=(uint16_t)(r->missing_score[p]-48u),source=c->text_base+4u*(v&15u)+8u*(v&240u);
        if(source+67>4095 || out+67>4095)return -1;
        out+=4;
    }
    if(p==r->missing_score_size)return -1;
    out=0x450;
    for(p=0;r->missing_score[p]!=255;p++) {
        unsigned v=(uint16_t)(r->missing_score[p]-48u),source=c->text_base+4u*(v&15u)+8u*(v&240u),y,x;
        for(y=0;y<2;y++)for(x=0;x<2;x++) {
            unsigned n=word(maps+source+64*y+2*x);n=(n&511u)|0x3600u;
            maps[out+64*y+2*x]=(uint8_t)n;maps[out+64*y+2*x+1]=(uint8_t)(n>>8);
        }
        out+=4;
    }
    return 0;
}
int ntm_round_record_blink_initialize(ntm_round_record_blink *s,ntm_task *t,
    ntm_round_record_context *c,const ntm_round_record_resources *r)
{
    if(!s || !t || !valid(c,r) || s->phase || !s->initial_delay || !s->shown_delay || !s->hidden_delay)return -1;
    if(missing_score(c,r)<0)return -1;
    t->delay=30;t->callback=s->initial_delay;s->phase=1;
    return ntm_round_record_blink_tick(s,t,c,r);
}
int ntm_round_record_blink_tick(ntm_round_record_blink *s,ntm_task *t,
    ntm_round_record_context *c,const ntm_round_record_resources *r)
{
    ntm_course_cursor *o;ntm_display_reset_state *d;unsigned offset;
    if(!s || !t || !valid(c,r) || s->phase<1 || s->phase>3 ||
       t->callback!=(s->phase==1?s->initial_delay:s->phase==2?s->shown_delay:s->hidden_delay))return -1;
    if(s->phase==1 && ((uint16_t)(t->delay-1u)&32768)) {
        d=c->intro->display;offset=d->zero_words[0];
        if(d->zero_flags[8]<32 && ((offset&1)||offset>=64))return -1;
    }
    t->delay=(uint16_t)(t->delay-1u);if(!(t->delay&32768))return 0;
    if(s->phase==1) {
        d=c->intro->display;o=c->cursor;offset=d->zero_words[0];
        o->id=d->object_ids[0];o->tag=33;d->object_tags[0]=33;
        o->width=word(r->metadata);o->velocity=word(r->metadata+2);o->byte15=r->metadata[4];
        *c->cursor_x=80;o->y=28;o->second=255;o->byte14=0;o->tile=0x1d0;o->flags=128;
        if(d->zero_flags[8]<32) {
            d->list[offset/2]=(uint16_t)(o->id+256u*o->second);d->zero_words[0]=(uint16_t)(offset+2);
            d->zero_flags[8]++;d->zero_flags[9]|=1;o->flags|=128;o->byte30=0;o->link=65535;
        }
        s->phase=2;t->callback=s->shown_delay;
    }else if(s->phase==2) {c->cursor->flags|=32;s->phase=3;t->callback=s->hidden_delay;}
    else {c->cursor->flags&=(uint8_t)~32u;s->phase=2;t->callback=s->shown_delay;}
    t->delay=4;return 0; /* store5 and immediately decrement */
}
