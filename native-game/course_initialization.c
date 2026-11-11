/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_initialization.h"
int ntm_course_initialize(ntm_course_initialization *s,ntm_course_screen *screen,
    ntm_display_reset_state *display,ntm_course_palette *palette,
    uint8_t sprites[256],uint8_t sprite_palettes[64],const ntm_screen_asset assets[5],
    const uint8_t descriptor[5],const ntm_palette_job jobs[2])
{
    unsigned i;ntm_course_cursor *c;
    if(!s || !screen || !display || !palette || !sprites || !sprite_palettes || !assets || !descriptor || !jobs)return -1;
    if(s->guard)return 1;
    if(!ntm_course_palette_job_valid(palette,jobs) || !ntm_course_palette_job_valid(palette,jobs+1))return -1;
    if(screen->displayed_selection>2 || screen->stamp_bytes>256 || (screen->stamp_bytes&7))return -1;
    s->plane_swap=0;
    (void)ntm_course_screen_enqueue_fill(screen,0,20,19,0);
    (void)ntm_course_screen_enqueue_fill(screen,0x800,20,19,0);
    s->flags&=(uint8_t)~24u;
    if(ntm_display_reset(display)<0)return -1;
    for(i=0;i<256;i++)sprites[i]=0;
    for(i=0;i<64;i++)sprite_palettes[i]=0;
    s->flags&=(uint8_t)~16u;s->sprite_x=s->sprite_y=0;
    s->screen_words[0]=s->screen_words[1]=0;s->mapped_course=0;
    s->selection=screen->displayed_selection;screen->cached_selection=65535;
    if(ntm_course_palette_enqueue(palette,jobs)<0 || ntm_course_palette_enqueue(palette,jobs+1)<0)return -1;
    (void)ntm_course_screen_enqueue_stamp(screen,assets,0x800,1);
    (void)ntm_course_screen_enqueue_stamp(screen,assets+1,0,131);
    c=&s->cursor;c->id=display->object_ids[0];c->tag=46;display->object_tags[0]=46;
    c->width=(uint16_t)(descriptor[0]+256u*descriptor[1]);c->velocity=(uint16_t)(descriptor[2]+256u*descriptor[3]);c->byte15=descriptor[4];
    screen->cursor_x=8;c->y=28;c->second=255;c->byte14=0;c->tile=450;c->flags=128;
    /* Reset guarantees count0 and offset0 for this initializer's append. */
    display->list[0]=(uint16_t)(c->id+256u*c->second);display->zero_words[0]=2;display->zero_flags[8]=1;display->zero_flags[9]|=1;
    c->flags|=128;c->byte30=0;c->link=65535;c->velocity=0;
    s->flags=(uint8_t)((s->flags&63)|16);
    return ntm_course_screen_draw(screen,s->selection,assets+2);
}
