/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_miss.h"
static int context_valid(const ntm_shot_scene_object_context *c)
{
 return c && c->shared && c->shared->objects && c->shared->x && c->shared->intro && c->shared->intro->screen_words;
}
int ntm_shot_miss_object_gate(ntm_shot_scene_object_context *c,uint16_t left,uint16_t width)
{
 uint16_t offset;
 if(!context_valid(c))return -1;
 offset=(uint16_t)(c->shared->x[5]-c->shared->intro->screen_words[0]-left);
 if(c->shared->objects[5].byte15==3 && offset<=width)c->shared->objects[5].byte15=2;
 return 0;
}
int ntm_shot_miss_tick(ntm_shot_miss *s,ntm_task *t)
{
 if(!s || !t || s->phase!=1 || t->callback!=s->delay || !s->continuation)return -1;
 t->delay=(uint16_t)(t->delay-1u);
 if(!(t->delay&32768u))return 0;
 t->callback=s->continuation;s->phase=2;return 2;
}
int ntm_shot_miss_initialize(ntm_shot_miss *s,ntm_task *t,ntm_shot_scene_object_context *c,
 uint8_t *miss_flag,const ntm_shot_miss_resources *r)
{
 if(!s || !t || s->phase || !s->delay || !s->continuation || !context_valid(c) || !c->hole ||
    !c->shared->intro->screen || !miss_flag || !r)return -1;
 *miss_flag=255;c->hole->hole[0x71]=c->hole->hole[0x70];
 (void)ntm_shot_miss_object_gate(c,38,68);
 (void)ntm_course_screen_enqueue_stamp(c->shared->intro->screen,&r->notice,0x1ca,0x22f8);
 t->delay=30;t->callback=s->delay;s->phase=1;return ntm_shot_miss_tick(s,t);
}
