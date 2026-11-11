/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_golfer_animation.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static int signed_less(uint16_t a,uint16_t b){return (a^32768u)<(b^32768u);}
int ntm_shot_golfer_animation_tick(ntm_shot_golfer *s,ntm_course_screen *screen,
    const ntm_shot_golfer_animation_resources *r)
{
    unsigned layer,row[2]={0,4},frame[2]={0,0};uint8_t next[8];int animate[2]={0,0};
    if(!s || !screen || s->phase!=1 || screen->stamp_bytes>256 || (screen->stamp_bytes&7))return -1;
    for(layer=0;layer<2;layer++) {
        unsigned at=4*layer;uint8_t kind=s->task_bytes[at+3];
        next[at]=s->task_bytes[at];next[at+1]=s->task_bytes[at+1];
        next[at+2]=s->task_bytes[at+2];next[at+3]=kind;
        if(kind==252)continue;
        if((layer && kind) || (!layer && (kind>12 || (kind&3))))return NTM_TASK_BOUNDARY;
        if(!layer)row[layer]=kind/4;
        next[at+1]=(uint8_t)(next[at+1]-1u);
        if(next[at+1])continue;
        if(next[at]&1)return NTM_TASK_BOUNDARY;
        next[at]=(uint8_t)((next[at]+2u)&7u);next[at+1]=next[at+2];
        frame[layer]=next[at]/2;animate[layer]=1;
        if(!r)return -1;
    }
    for(layer=0;layer<8;layer++)s->task_bytes[layer]=next[layer];
    for(layer=0;layer<2;layer++)if(animate[layer])
        (void)ntm_course_screen_enqueue_stamp(screen,&r->frames[row[layer]][frame[layer]],
            (uint16_t)(layer?0xb44:0xa44),(uint16_t)(layer?0x1b6:0x190));
    return 0;
}
int ntm_shot_golfer_pin_adjust(const ntm_shot_golfer *s,ntm_shot_scene_object_context *c,
    const ntm_shot_golfer_green *green)
{
    ntm_course_cursor *pin;uint16_t x,left,width;
    if(!s || !c || !c->hole || !c->shared || !c->shared->intro ||
       !c->shared->intro->screen_words || !c->shared->objects || !c->shared->x || !green)return -1;
    pin=c->shared->objects+5;
    if(signed_less(word(c->hole->hole+0x6a),192)) {
        pin->byte15=3;x=(uint16_t)(c->shared->x[5]-c->shared->intro->screen_words[0]);
        if(green->status!=255) {
            left=(uint16_t)(((uint16_t)(green->center_x+72u)&65528u)-green->half_width);
            width=(uint16_t)(green->half_width+green->half_width);
            if((uint16_t)(x-left)<=width)pin->byte15=2;
        }
        if((uint16_t)(x-18u)>108)pin->byte15=2;
    }
    if((s->surface==6 || s->surface==7) && !signed_less(pin->velocity,113)) {
        pin->velocity=112;pin->flags|=128;
    }
    return 0;
}
