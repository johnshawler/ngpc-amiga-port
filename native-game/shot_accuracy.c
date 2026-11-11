/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_accuracy.h"
static int sb(uint8_t v){return v<128?(int)v:(int)v-256;}
static int lie_accepts(uint8_t lie,const uint8_t *p)
{
    if(lie==0 || lie==4)return 1;
    if(p[0x33])return lie==1 || lie==2;
    return p[1]!=1 && (lie==1 || (p[1]==4 && lie==2));
}
int ntm_shot_accuracy_initialize(ntm_shot_accuracy *s,ntm_task *t,ntm_shot_scene_object_context *c)
{
    const uint8_t *h;
    if(!s || !t || !c || !c->hole || !c->aim || s->phase || !s->wait)return -1;
    h=c->hole->hole;s->position=0x1300;
    s->velocity=c->aim->club==13?0:(uint16_t)(h[0x79]+256u*h[0x7a]);
    s->lower=h[0x7b];s->upper=(uint8_t)(38u-h[0x7b]);
    t->callback=s->wait;s->phase=1;return 0;
}
int ntm_shot_accuracy_tick(ntm_shot_accuracy *s,ntm_shot_scene_object_context *c,
    uint8_t lifecycle,uint8_t label,uint8_t held,ntm_task_context *tasks,ntm_task *t)
{
    uint8_t high,*h;uint16_t candidate;
    if(!s || !c || !c->hole || s->phase!=1)return -1;
    h=c->hole->hole;
    if(lifecycle==4) {
        if(!c->aim || !c->player || c->player->active_player>1)return -1;
        high=(uint8_t)(s->position>>8);h[0x78]=high;
        if(sb(high)>sb(s->upper))c->aim->reset_d2=1;
        else if(sb(high)<sb(s->lower))c->aim->reset_d2=2;
        else if(c->aim->club!=13) {
            if(sb(label)>9 && lie_accepts(h[0x70],c->hole->players[c->player->active_player]))h[0x7c]=held&3u;
            if((uint8_t)(high-18u)<=2 && sb(h[0x7d])>=sb((uint8_t)(h[0x7e]-4u)) && sb(label)<=9)
                c->aim->reset_d3=1;
        }
        return ntm_task_finish(tasks,t);
    }
    if(!c->shared || !c->shared->intro || !c->shared->intro->screen_words || !c->shared->objects)return -1;
    candidate=(uint16_t)(s->position+s->velocity);high=(uint8_t)(candidate>>8);
    if(sb(high)>=37 || sb(high)<=1) {
        high=(uint8_t)(sb(high)>=37?37:1);
        candidate=(uint16_t)(((uint16_t)high<<8)|(candidate&255u));
        s->position=(uint16_t)(candidate+s->velocity);
        s->velocity=(uint16_t)(0u-s->velocity);
    }
    s->position=(uint16_t)(s->position+s->velocity);
    c->shared->objects[7].y=(uint16_t)((uint8_t)(151u-(s->position>>8))+c->shared->intro->screen_words[1]);
    return 0;
}
