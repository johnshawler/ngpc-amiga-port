/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_swing_exit.h"
static int valid(const ntm_shot_swing_exit *s,const ntm_task *t,const ntm_shot_scene_object_context *c)
{
    return s && t && t->next && t->previous && c && c->aim && c->shared &&
        c->shared->tasks && c->shared->tasks->current==t && c->shared->intro &&
        c->shared->intro->display && c->shared->objects && c->shared->intro->viewport;
}
static void reset(ntm_shot_scene_object_context *c)
{
    (void)ntm_tee_objects_reset(c->shared->intro->display,c->shared->objects);
    /* Separate native screen-word mirror aliases these original reset bytes. */
    if(c->shared->intro->screen_words){c->shared->intro->screen_words[0]&=0xff00u;c->shared->intro->screen_words[1]&=0xff00u;}
}
static int close_and_finish(ntm_shot_swing_exit *s,ntm_task *t,ntm_shot_scene_object_context *c)
{
    uint8_t *v=c->shared->intro->viewport;int r;
    v[0]=v[1]=v[2]=0;v[3]=152;
    if(ntm_task_remove_descendants(c->shared->tasks,t)<0)return -1;
    r=ntm_task_finish(c->shared->tasks,t);if(r>=0)s->phase=3;return r;
}
int ntm_shot_swing_exit_hit(ntm_shot_swing_exit *s,ntm_task *t,ntm_shot_scene_object_context *c)
{
    if(!valid(s,t,c) || s->phase)return -1;
    if(c->aim->reset_d3==1 && (!s->perfect_entry || !s->perfect_wait || !c->shared->fallback))return -1;
    reset(c);
    if(c->aim->reset_d3!=1)return close_and_finish(s,t,c);
    if(ntm_task_remove_descendants(c->shared->tasks,t)<0)return -1;
    (void)ntm_task_allocate(c->shared->tasks,t,s->perfect_entry,c->shared->fallback);
    t->callback=s->perfect_wait;s->phase=1;return 0;
}
int ntm_shot_swing_exit_miss(ntm_shot_swing_exit *s,ntm_task *t,ntm_shot_scene_object_context *c)
{
    if(!valid(s,t,c) || s->phase || !s->miss_entry || !s->miss_wait || !c->shared->fallback)return -1;
    if(ntm_task_remove_descendants(c->shared->tasks,t)<0)return -1;
    (void)ntm_task_allocate(c->shared->tasks,t,s->miss_entry,c->shared->fallback);
    t->callback=s->miss_wait;s->phase=2;return 0;
}
int ntm_shot_swing_exit_tick(ntm_shot_swing_exit *s,ntm_task *t,ntm_shot_scene_object_context *c)
{
    if(!valid(s,t,c) || (s->phase!=1 && s->phase!=2))return -1;
    if(s->phase==1){if(t->callback!=s->perfect_wait)return -1;if(t->priority<t->next->priority)return 0;}
    else {if(t->callback!=s->miss_wait)return -1;if(!c->aim->reset_e6)return 0;reset(c);}
    return close_and_finish(s,t,c);
}
