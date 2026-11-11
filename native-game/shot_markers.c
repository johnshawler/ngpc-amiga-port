/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_markers.h"
#include "shot_math.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static uint32_t lng(const uint8_t *p){return (uint32_t)word(p)|((uint32_t)word(p+2)<<16);}
static void putl(uint8_t *p,uint32_t v){unsigned i;for(i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static int signed_gt(uint16_t a,uint16_t b){return (a^32768u)>(b^32768u);}
uint32_t ntm_shot_marker_multiply(uint32_t a,uint32_t b)
{
    unsigned negative=(a^b)>>31;uint32_t out,low;
    if(a&0x80000000u)a=0u-a;
    if(b&0x80000000u)b=0u-b;
    out=ntm_shot_multiply(a,b);low=(a&65535u)*(b&65535u);
    if(negative && (a>=65536u || b>=65536u) && (low&65535u))out++;
    return negative?0u-out:out;
}
uint32_t ntm_shot_marker_direction(const uint8_t quarter[4100],uint16_t angle)
{
    unsigned negative=0;uint32_t v;angle&=4095u;
    if(angle>2048)angle=(uint16_t)((0u-angle)&2047u);
    if(angle>1024){negative=1;angle=(uint16_t)((0u-angle)&1023u);}
    v=lng(quarter+4u*(1024u-angle));return negative?v:0u-v;
}
static int valid(const ntm_shot_marker_context *c,const ntm_shot_marker_resources *r)
{return c && r && c->shared && c->shared->intro && c->shared->intro->display && c->shared->intro->screen_words && c->shared->objects && c->shared->x && c->hole && c->aim && c->dirty && r->quarter && r->quarter_size>=4100 && r->metadata && r->metadata_size>=10;}
int ntm_shot_markers_position(ntm_shot_marker_context *c,const ntm_shot_marker_resources *r)
{
    uint32_t dx,dy,distance;uint16_t length,x;ntm_tee_setup_context *q;uint8_t *h;
    if(!valid(c,r))return -1;
    if(c->player_status)return 2;
    q=c->shared;h=c->hole->hole;length=word(h+0x6a);distance=signed_gt(length,48)?40u<<16:20u<<16;
    dx=ntm_shot_marker_direction(r->quarter,(uint16_t)(c->aim->angle+1024u));
    dy=ntm_shot_marker_direction(r->quarter,c->aim->angle);putl(h+0xd4,dx);putl(h+0xd8,dy);
    q->x[2]=(uint16_t)((ntm_shot_marker_multiply(dx,distance)+lng(h+0x44))>>16);
    q->objects[2].y=(uint16_t)((ntm_shot_marker_multiply(dy,distance)+lng(h+0x48))>>16);
    distance=signed_gt(length,160)?30u<<16:(uint32_t)(length>>3)<<16;
    q->x[3]=(uint16_t)(q->x[1]+(ntm_shot_marker_multiply(dx,distance)>>16));
    q->objects[3].y=(uint16_t)(q->objects[1].y+(ntm_shot_marker_multiply(dy,distance)>>16));
    q->objects[3].flags&=(uint8_t)~32u;x=(uint16_t)(q->x[3]-q->intro->screen_words[0]);
    if(!signed_gt(x,128))q->objects[3].flags|=32;
    return 0;
}
int ntm_shot_markers_initialize(ntm_shot_markers *s,ntm_task *t,ntm_shot_marker_context *c,const ntm_shot_marker_resources *r)
{
    unsigned i;int result;
    if(!s || !t || !s->tick || !s->waiting || s->phase || !valid(c,r))return -1;
    if(c->player_status)return 2;
    *c->dirty=255;
    for(i=0;i<2;i++){
        ntm_course_cursor *o=c->shared->objects+i+2;const uint8_t *m=r->metadata+5*i;
        o->tag=(uint16_t)(3+i);c->shared->intro->display->object_tags[i+2]=o->tag;
        o->width=word(m);o->velocity=word(m+2);o->byte15=m[4];
        c->shared->x[i+2]=0;o->y=0;o->second=16;o->byte14=0;o->tile=(uint16_t)(60+i);o->flags=128;
        if(ntm_tee_object_register(c->shared->intro->display,o)<0)return -1;
    }
    s->step=4;s->limit=512;result=ntm_shot_markers_position(c,r);if(result)return result;
    t->callback=s->tick;s->phase=1;return 0;
}
int ntm_shot_markers_tick(ntm_shot_markers *s,ntm_task *t,ntm_shot_marker_context *c,const ntm_shot_marker_resources *r)
{
    uint16_t delta;unsigned buttons;
    if(!s || !t || !valid(c,r))return -1;
    if(s->phase==1 && t->callback==s->tick){
        if(c->aim->reset_d1==1){t->callback=s->waiting;s->phase=2;return 2;}
        if(!c->aim->reset_4416){
            buttons=c->held&12u;
            if(buttons==4 || buttons==8){
                delta=(uint16_t)(c->aim->angle_delta+(buttons==4?0u-s->step:s->step));
                if(!signed_gt(delta,s->limit) && !signed_gt((uint16_t)(0u-s->limit),delta)){
                    c->aim->angle_delta=delta;*c->dirty=255;
                }
            }
        }
        if(*c->dirty){*c->dirty=0;c->aim->angle=(uint16_t)((c->aim->initial_angle+c->aim->angle_delta)&4095u);return ntm_shot_markers_position(c,r);}
        return 0;
    }
    if(s->phase==2 && t->callback==s->waiting)return 2;
    return -1;
}
int ntm_shot_hint_initialize(ntm_shot_hint_delay *s,ntm_task *t,ntm_task_context *c,uint8_t mode)
{
    if(!s || !t || !c || !s->delay || !s->active || s->phase)return -1;
    if(!mode)return ntm_task_finish(c,t)<0?-1:0;
    s->repeat=1;s->last=255;s->pending=s->changed=0;t->delay=30;t->callback=s->delay;s->phase=1;
    return ntm_shot_hint_tick(s,t);
}
int ntm_shot_hint_tick(ntm_shot_hint_delay *s,ntm_task *t)
{
    if(!s || !t || s->phase!=1 || t->callback!=s->delay)return -1;
    t->delay=(uint16_t)(t->delay-1u);if(!(t->delay&32768u))return 0;
    t->callback=s->active;s->phase=2;return 2;
}
int ntm_shot_camera_initialize(ntm_shot_camera_delay *s,ntm_task *t)
{
    if(!s || !t || !s->delay || !s->active || s->phase)return -1;
    t->delay=8;t->callback=s->delay;s->phase=1;return ntm_shot_camera_tick(s,t);
}
int ntm_shot_camera_tick(ntm_shot_camera_delay *s,ntm_task *t)
{
    if(!s || !t || s->phase!=1 || t->callback!=s->delay)return -1;
    t->delay=(uint16_t)(t->delay-1u);if(!(t->delay&32768u))return 0;
    t->callback=s->active;s->phase=2;return 2;
}
