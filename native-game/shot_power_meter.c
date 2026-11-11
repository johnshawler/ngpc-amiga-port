/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_power_meter.h"
#include "tee_graphics.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static int signed_byte(uint8_t x){return x<128?(int)x:(int)x-256;}
static void put(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static int scale_valid(const ntm_shot_power_meter_resources *r)
{return r && r->scale.power_scale && r->scale.power_scale_size>=256;}
int ntm_shot_power_meter_initialize(ntm_shot_power_meter *s,ntm_task *t,
    ntm_shot_scene_object_context *c,const ntm_shot_power_meter_resources *r)
{
    const uint8_t *p;unsigned i,a,b;uint16_t speed;uint8_t upper;
    if(!s || !t || !c || !c->hole || !c->player || c->player->active_player>1 ||
       !c->aim || !c->shared || !c->shared->tiles || !s->wait || s->phase || !scale_valid(r) ||
       !r->speed_player || !r->speed_lie || r->speed_player_size<257 || r->speed_lie_size<257 ||
       !r->scale.remainders || r->scale.remainders_size<10)return -1;
    /* Preflight every borrowed glyph before any visible mutation. */
    for(i=0;i<9;i++) {
        const ntm_screen_asset *g=r->glyphs+i;unsigned n,j;
        if(!g->data || !g->tiles || g->size<6)return -1;
        n=word(g->data);if(!n || n>512 || g->size<6u+2u*n || 0xe40u+32*i+16*n>8192)return -1;
        for(j=0;j<n;j++)if((size_t)word(g->data+6+2*j)+16>g->tiles_size)return -1;
    }
    p=c->hole->players[c->player->active_player];
    a=((uint8_t)((uint8_t)(p[1]+p[0x33])*2u))^128u;
    b=((uint8_t)((uint8_t)(c->hole->hole[0x70]+p[3])*2u))^128u;
    speed=(uint16_t)(word(r->speed_player+a)+word(r->speed_lie+b));
    if(c->hole->hole[0x70]==6 && !p[0x33])speed=(uint16_t)(speed+speed);
    upper=(uint8_t)(p[0x23]+62u);if(c->aim->club==13)upper=59;
    c->hole->hole[0x7c]=c->hole->hole[0x7d]=0;
    s->hold=1;s->position=256;s->lower=1;s->velocity=speed;s->upper=upper;
    if(ntm_shot_scene_power_limit(c->hole,c->player,c->aim->club,&r->scale)<0)return -1;
    for(i=0;i<9;i++)if(ntm_tee_copy_tiles(r->glyphs+i,(uint16_t)(0xe40+32*i),c->shared->tiles)<0)return -1;
    t->callback=s->wait;s->phase=1;return 0;
}
int ntm_shot_power_meter_advance(ntm_shot_power_meter *s,ntm_hole_prepare_state *h,
    const ntm_shot_player_state *p,const ntm_shot_power_meter_resources *r)
{
    uint8_t high;uint16_t v;
    if(!s || !h || !p || p->active_player>1 || !scale_valid(r))return -1;
    v=(uint16_t)(s->position+s->velocity);high=(uint8_t)(v>>8);
    if(signed_byte(high)>=signed_byte(s->upper)) {
        s->position=(uint16_t)(((unsigned)s->upper<<8)+s->velocity);
        s->velocity=(uint16_t)(0u-s->velocity);
        if(h->players[p->active_player][1]==2) {
            s->hold=5;s->position=(uint16_t)(s->position-256u);
            h->hole[0x7d]=r->scale.power_scale[s->upper^128u];
        }
    } else if(signed_byte(high)<=signed_byte(s->lower)) {
        s->position=(uint16_t)(((unsigned)s->lower<<8)+s->velocity);
        s->velocity=(uint16_t)(0u-s->velocity);
    }
    s->hold=(uint8_t)(s->hold-1);
    if(!s->hold) {
        s->hold=1;s->position=(uint16_t)(s->position+s->velocity);
        h->hole[0x7d]=r->scale.power_scale[(s->position>>8)^128u];
    }
    return 0;
}
int ntm_shot_power_meter_tick(ntm_shot_power_meter *s,ntm_shot_scene_object_context *c,
    const ntm_shot_power_meter_resources *r,uint8_t lifecycle,ntm_task_context *tasks,ntm_task *t)
{
    ntm_shot_power_meter next;ntm_hole_prepare_state h;unsigned full,rem,row=0;uint8_t *maps;
    if(!s || !c || !c->hole || s->phase!=1)return -1;
    if(lifecycle!=2)return ntm_task_finish(tasks,t);
    if(!c->shared || !c->shared->intro || !c->shared->intro->maps ||
       !c->shared->intro->screen_words || !c->shared->objects || !r ||
       !r->scale.remainders || r->scale.remainders_size<10)return -1;
    next=*s;h=*c->hole;
    if(ntm_shot_power_meter_advance(&next,&h,c->player,r)<0)return -1;
    if(h.hole[0x7d]>120)return NTM_TASK_BOUNDARY;
    *s=next;c->hole->hole[0x7d]=h.hole[0x7d];maps=c->shared->intro->maps;
    full=h.hole[0x7d]/10;rem=h.hole[0x7d]%10;
    for(;row<full;row++){put(maps+0x300-64*row,0x26f4);put(maps+0x302-64*row,0x26f5);}
    if(row<12){uint16_t tile=(uint16_t)(0x26e4+2u*r->scale.remainders[rem]);
        put(maps+0x300-64*row,tile);put(maps+0x302-64*row,(uint16_t)(tile+1));row++;}
    for(;row<12;row++){put(maps+0x300-64*row,0x26e4);put(maps+0x302-64*row,0x26e5);}
    return ntm_shot_scene_power_position(c);
}
