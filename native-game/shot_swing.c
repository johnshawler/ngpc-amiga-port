/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_swing.h"
static int valid(const ntm_shot_swing *s,const ntm_task *t,const ntm_shot_swing_context *c)
{return s && t && c && c->scene && c->scene->aim && c->scene->shared && c->overlay;}
int ntm_shot_swing_initialize(ntm_shot_swing *s,ntm_task *t,ntm_shot_swing_context *c)
{
    ntm_tee_setup_context *q;
    if(!valid(s,t,c) || s->phase || !s->waiting || !s->hit_delay || !s->hit_reset)return -1;
    q=c->scene->shared;c->scene->aim->reset_d1=4;t->result=0;*c->overlay=255;
    if(!c->suppress_sound && q->sound_request)q->sound_request(q->sound_context,0,65532,0);
    t->callback=s->waiting;s->phase=1;s->boundary=0;return 0;
}
int ntm_shot_swing_tick(ntm_shot_swing *s,ntm_task *t,ntm_shot_swing_context *c)
{
    if(!valid(s,t,c) || !s->phase || s->phase>2)return -1;
    if(s->phase==1) {
        if(t->callback!=s->waiting)return -1;
        if(!t->result)return 0;
        if(t->result&128u){s->boundary=NTM_SHOT_SWING_MISS;s->phase=3;return 2;}
        if(!c->scene->shared->objects || !s->hit_delay)return -1;
        c->scene->shared->objects[16].flags|=64;t->delay=4;t->callback=s->hit_delay;s->phase=2;
    }
    if(t->callback!=s->hit_delay)return -1;
    t->delay=(uint16_t)(t->delay-1u);if(!(t->delay&32768u))return 0;
    /* Original installs209b05 at this exact prefix before executing its body. */
    t->callback=s->hit_reset;s->boundary=NTM_SHOT_SWING_HIT;s->phase=3;return 2;
}
