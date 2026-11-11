/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_power.h"
static int valid(const ntm_shot_power *s,const ntm_task *t,const ntm_shot_power_context *c)
{
    return s && t && c && c->scene && c->scene->shared &&
        c->scene->shared->intro && c->scene->aim && c->overlay;
}
static int allocation_valid(const ntm_shot_power_context *c,const ntm_task *t)
{
    return c->scene->shared->tasks && c->scene->shared->fallback && t->next;
}
int ntm_shot_power_initialize(ntm_shot_power *s,ntm_task *t,ntm_shot_power_context *c)
{
    unsigned y,x;ntm_tee_setup_context *q;
    if(!valid(s,t,c) || s->phase || !s->meter_entry || !s->power_delay ||
       !s->power_ready || !allocation_valid(c,t) || !c->scene->saved_map ||
       !c->scene->shared->intro->maps)return -1;
    q=c->scene->shared;c->scene->aim->reset_d1=2;
    for(y=0;y<4;y++)for(x=0;x<8;x++)q->intro->maps[0x10e +64*y+x]=c->scene->saved_map[8*y+x];
    (void)ntm_task_allocate(q->tasks,t,s->meter_entry,q->fallback);
    t->delay=3;t->callback=s->power_delay;s->phase=1;s->boundary=0;return 0;
}
int ntm_shot_power_cancel(ntm_shot_power *s,ntm_task *t,ntm_shot_power_context *c)
{
    uint8_t *v;
    if(!valid(s,t,c) || s->phase || !s->cancel_rebuild ||
       !c->scene->shared->intro->viewport || !c->scene->shared->tasks || !t->next)return -1;
    *c->overlay=255;v=c->scene->shared->intro->viewport;
    v[0]=v[1]=v[2]=0;v[3]=152;
    if(ntm_task_remove_descendants(c->scene->shared->tasks,t)<0)return -1;
    t->callback=s->cancel_rebuild;s->phase=5;s->boundary=0;return 0;
}
int ntm_shot_power_tick(ntm_shot_power *s,ntm_task *t,ntm_shot_power_context *c,uint8_t pressed)
{
    ntm_tee_setup_context *q;unsigned x,y;
    if(!valid(s,t,c) || !s->phase || s->phase>=8)return -1;
    q=c->scene->shared;
    if(s->phase==5){
        if(t->callback!=s->cancel_rebuild || !q->intro->maps || !q->intro->screen_words || !c->scene->hole)return -1;
        for(y=0;y<15;y++)for(x=0;x<28;x++)q->intro->maps[0x84+64*y+x]=0;
        c->scene->aim->reset_d1=0;q->intro->screen_words[0]=q->intro->screen_words[1]=0;
        (void)ntm_shot_aim_camera(c->scene->aim,c->scene->hole->hole);
        s->boundary=NTM_SHOT_POWER_WORLD;s->phase=9;return 2;
    }
    if(s->phase==6){
        if(t->callback!=s->cancel_restore || !s->cancel_return || !q->intro->maps || !c->saved_meter)return -1;
        for(y=0;y<12;y++)for(x=0;x<4;x++)q->intro->maps[0x40+64*y+x]=c->saved_meter[x];
        c->scene->aim->reset_d1=1;t->callback=s->cancel_return;s->phase=7;return 0;
    }
    if(s->phase==7){
        if(t->callback!=s->cancel_return || !s->fire_closed)return -1;
        t->delay=3;t->callback=s->fire_closed;s->phase=8;return 0;
    }
    if(s->phase==1){
        if(t->callback!=s->power_delay || !s->power_ready)return -1;
        t->delay=(uint16_t)(t->delay-1u);if(!(t->delay&32768u))return 0;
        t->callback=s->power_ready;s->phase=2;
    }
    if(s->phase==2){
        if(t->callback!=s->power_ready)return -1;
        if(pressed&32u){
            if(!s->cancel_restore)return -1;
            *c->overlay=255;t->callback=s->cancel_restore;s->phase=6;return 0;
        }
        if(!(pressed&16u))return 0;
        if(!s->accuracy_entry || !s->accuracy_delay || !s->accuracy_ready || !allocation_valid(c,t))return -1;
        c->scene->aim->reset_d1=3;
        (void)ntm_task_allocate(q->tasks,t,s->accuracy_entry,q->fallback);
        t->delay=3;t->callback=s->accuracy_delay;s->phase=3;return 0;
    }
    if(s->phase==3){
        if(t->callback!=s->accuracy_delay || !s->accuracy_ready)return -1;
        t->delay=(uint16_t)(t->delay-1u);if(!(t->delay&32768u))return 0;
        t->callback=s->accuracy_ready;s->phase=4;
    }
    if(s->phase!=4 || t->callback!=s->accuracy_ready)return -1;
    if(c->scene->aim->club!=13 && !(pressed&16u))return 0;
    s->boundary=NTM_SHOT_POWER_SWING;s->phase=9;return 2;
}
