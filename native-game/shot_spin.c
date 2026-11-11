/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_spin.h"
int ntm_shot_spin_initialize(ntm_shot_spin *s,ntm_task *t,
    ntm_hole_prepare_state *h,const ntm_shot_player_state *p,
    const ntm_shot_spin_resources *r)
{
    unsigned index;const uint8_t *player;
    if(!s || !t || !h || !p || !r || !r->bounds || !s->wait ||
       s->phase || p->active_player>=2)return -1;
    player=h->players[p->active_player];
    index=(uint8_t)((uint8_t)(player[1]+player[0x33])*2u);
    if(index+1u>=r->bounds_size)return -1;
    s->minimum=r->bounds[index];s->maximum=r->bounds[index+1];
    s->current=h->hole[0x76];h->hole[0x77]=0;
    t->callback=s->wait;s->phase=1;return 0;
}
int ntm_shot_spin_tick(const ntm_shot_spin *s,const ntm_hole_prepare_state *h,
    uint8_t lifecycle,uint8_t input_gate,uint8_t pressed,ntm_task_context *tasks,ntm_task *task)
{
    if(!s || !h || s->phase!=1)return -1;
    if(lifecycle==2)return NTM_TASK_IDLE;
    if(lifecycle==3)return ntm_task_finish(tasks,task);
    if((!input_gate && ((pressed&12u)==4 || (pressed&12u)==8)) || h->hole[0x77] ||
       h->hole[0x76]!=s->current)return NTM_TASK_BOUNDARY;
    return NTM_TASK_IDLE;
}

int ntm_shot_spin_control(ntm_shot_spin *s,ntm_shot_scene_object_context *c,
    const ntm_shot_spin_draw_resources *r,uint8_t lifecycle,uint8_t input_gate,
    uint8_t pressed,ntm_task_context *tasks,ntm_task *task)
{
    uint8_t spin,timer;unsigned edge,row,j;int changed=0;
    if(!s || !c || !c->hole || s->phase!=1)return -1;
    if(lifecycle==3)return ntm_task_finish(tasks,task);
    if(lifecycle==2)return 0;
    spin=c->hole->hole[0x76];timer=c->hole->hole[0x77];edge=pressed&12u;
    if(!input_gate && (edge==4 || edge==8)) {
        uint8_t next=(uint8_t)(spin+(edge==4?255:1));
        if((next^128u)>=(s->minimum^128u) && (next^128u)<=(s->maximum^128u)) {
            spin=next;timer=32;changed=1;
        }
    }
    if(spin!=s->current && spin>=9)return NTM_TASK_BOUNDARY;
    if(!c->shared || !c->shared->intro || !c->shared->intro->screen)return -1;
    if(timer==1 && (!c->saved_map || !c->shared->intro->maps))return -1;
    if(spin!=s->current && !r)return -1;
    c->hole->hole[0x76]=spin;
    if(changed && c->shared->sound_request)c->shared->sound_request(c->shared->sound_context,2,1,0);
    if(timer) {
        timer--;
        if(!timer)for(row=0;row<4;row++)for(j=0;j<8;j++)
            c->shared->intro->maps[0x10e +64*row+j]=c->saved_map[8*row+j];
    }
    c->hole->hole[0x77]=timer;
    if(spin!=s->current) {
        s->current=spin;
        if(ntm_course_screen_enqueue_stamp(c->shared->intro->screen,r->indicators+spin,0x448,0xd6)<0 ||
           ntm_course_screen_enqueue_stamp(c->shared->intro->screen,r->highlights+spin,0x10e,0x28e4)<0)return -1;
    }
    return 0;
}
