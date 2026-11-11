/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_fire.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static void putl(uint8_t *p,uint32_t v){unsigned i;for(i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static int gt(uint16_t a,uint16_t b){return (a^32768u)>(b^32768u);}
static int valid(const ntm_shot_fire *s,const ntm_task *t,const ntm_shot_fire_context *c)
{
    return s && t && c && c->shared && c->shared->intro && c->aim && c->overlay &&
        c->shared->intro->viewport && s->closed_delay && s->open_delay && s->ready;
}
int ntm_shot_fire_initialize(ntm_shot_fire *s,ntm_task *t,ntm_shot_fire_context *c,
    const ntm_shot_fire_resources *r)
{
    static const unsigned slots[4]={0,2,5,17};
    unsigned i,x,y;ntm_tee_setup_context *q;ntm_course_intro_context *g;
    ntm_display_reset_state check;ntm_course_cursor removed[4];
    if(!valid(s,t,c) || s->phase || !r || !s->spin_entry || !s->trajectory_entry ||
       !s->golfer_entry)return -1;
    q=c->shared;g=q->intro;
    if(!q->tasks || !q->fallback || !t->next || !q->objects || !g->display || !g->maps ||
       !g->flags || !ntm_course_palette_job_valid(g->palette,&r->palette))return -1;
    /* Validate all removals before changing live output. Original missing
     * registrations are harmless, including saturation and prior removals. */
    check=*g->display;
    for(i=0;i<4;i++){
        removed[i]=q->objects[slots[i]];
        if(ntm_tee_object_remove(&check,removed+i)<0)return -1;
    }
    *c->overlay=255;c->aim->reset_d1=1;
    g->viewport[0]=g->viewport[1]=g->viewport[2]=0;g->viewport[3]=152;
    *g->flags&=(uint8_t)~8u;
    for(i=0;i<4;i++)(void)ntm_tee_object_remove(g->display,q->objects+slots[i]);
    for(y=0;y<4;y++)for(x=0;x<8;x++)g->maps[0x10e + 64*y+x]=0;
    (void)ntm_course_palette_enqueue(g->palette,&r->palette);
    (void)ntm_task_allocate(q->tasks,t,s->spin_entry,q->fallback);
    (void)ntm_task_allocate(q->tasks,t,s->trajectory_entry,q->fallback);
    (void)ntm_task_allocate(q->tasks,t,s->golfer_entry,q->fallback);
    t->delay=3;t->callback=s->closed_delay;s->phase=1;s->boundary=0;return 0;
}
int ntm_shot_fire_tick(ntm_shot_fire *s,ntm_task *t,ntm_shot_fire_context *c,uint8_t pressed)
{
    uint8_t *v;
    if(!valid(s,t,c))return -1;
    if(s->phase==4)return s->boundary?2:-1;
    if(s->phase==1){
        if(t->callback!=s->closed_delay)return -1;
        t->delay=(uint16_t)(t->delay-1u);if(!(t->delay&32768u))return 0;
        v=c->shared->intro->viewport;v[0]=v[1]=0;v[2]=160;v[3]=152;
        *c->overlay=0;t->delay=4;t->callback=s->open_delay;s->phase=2;
    }
    if(s->phase==2){
        if(t->callback!=s->open_delay)return -1;
        t->delay=(uint16_t)(t->delay-1u);if(!(t->delay&32768u))return 0;
        t->callback=s->ready;s->phase=3;
    }
    if(s->phase!=3 || t->callback!=s->ready)return -1;
    if(pressed&32u)s->boundary=NTM_SHOT_FIRE_CANCEL;
    else if(pressed&16u)s->boundary=NTM_SHOT_FIRE_POWER;
    else return 0;
    s->phase=4;return 2;
}
int ntm_shot_fire_markers_tick(ntm_shot_markers *s,ntm_task *t,ntm_task_context *tasks,
    ntm_shot_marker_context *c,const ntm_shot_marker_resources *r)
{
    ntm_tee_setup_context *q;uint8_t *h;uint32_t dx,dy,distance;uint16_t length,x;int result;
    if(!s || !t || !tasks || !c || !c->aim)return -1;
    if(s->phase==1){
        result=ntm_shot_markers_tick(s,t,c,r);if(result!=2 || s->phase!=2)return result;
    }
    if(s->phase!=2 || t->callback!=s->waiting)return -1;
    if(c->aim->reset_d1==2)return ntm_task_finish(tasks,t)<0?-1:0;
    if(!c->dirty)return -1;
    if(!*c->dirty)return 0;
    if(!c->shared || !c->shared->intro || !c->shared->intro->screen_words ||
       !c->shared->objects || !c->shared->x || !c->hole || !r || !r->quarter ||
       r->quarter_size<4100)return -1;
    q=c->shared;h=c->hole->hole;*c->dirty=0;
    dx=ntm_shot_marker_direction(r->quarter,(uint16_t)(c->aim->angle+1024u));
    dy=ntm_shot_marker_direction(r->quarter,c->aim->angle);putl(h+0xd4,dx);putl(h+0xd8,dy);
    length=word(h+0x6a);distance=gt(length,160)?30u<<16:(uint32_t)(length>>3)<<16;
    q->x[3]=(uint16_t)(q->x[1]+(ntm_shot_marker_multiply(dx,distance)>>16));
    q->objects[3].y=(uint16_t)(q->objects[1].y+(ntm_shot_marker_multiply(dy,distance)>>16));
    q->objects[3].flags&=(uint8_t)~32u;x=(uint16_t)(q->x[3]-q->intro->screen_words[0]);
    if(!gt(x,128))q->objects[3].flags|=32;
    return 0;
}
