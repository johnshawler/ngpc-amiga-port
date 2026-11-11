/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_scene.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
int ntm_shot_scene_initialize(ntm_shot_scene *s,ntm_task *t,ntm_shot_scene_context *c)
{
    ntm_tee_setup_context *q;ntm_course_intro_context *i;ntm_terrain_result terrain;unsigned x,y;
    if(!s || !t || !c || !(q=c->shared) || !(i=q->intro) || !i->screen_words || !i->maps ||
       !i->display || !q->tasks || !q->fallback || !c->hole || !c->preparation || !c->terrain ||
       !c->random_cursor || !c->random_window || !s->banner_entry || !s->banner_wait || s->phase || !t->next)return -1;
    if(q->game_mode)return NTM_TEE_UNSUPPORTED;
    i->screen_words[0]=i->screen_words[1]=0;
    for(y=0;y<19;y++)for(x=0;x<40;x++)i->maps[64*y+x]=0;
    if(ntm_shot_prepare_geometry(&s->player,c->hole,c->preparation)<0)return -1;
    terrain.x=(uint16_t)i->display->reset_words[0];terrain.y=(uint16_t)(i->display->reset_words[0]>>16);
    terrain.detail=s->terrain_detail;terrain.terrain=0;
    if(ntm_shot_terrain(c->hole->hole,s->player_status,word(c->hole->hole+0x46),word(c->hole->hole+0x4a),c->terrain,&terrain)<0)return -1;
    i->display->reset_words[0]=(uint32_t)terrain.x|((uint32_t)terrain.y<<16);s->terrain_detail=terrain.detail;
    c->hole->hole[0x70]=c->hole->hole[0x73]=terrain.terrain;c->hole->hole[0x76]=4;
    if(ntm_shot_advance_stroke(&s->player,c->hole)<0 || ntm_shot_update_wind(c->hole,c->preparation,c->random_cursor,c->random_window)<0)return -1;
    (void)ntm_task_allocate(q->tasks,t,s->banner_entry,q->fallback);
    t->result=0;t->callback=s->banner_wait;s->phase=1;return 0;
}
int ntm_shot_scene_wait(ntm_shot_scene *s,ntm_task *t)
{
    if(!s || !t)return -1;
    if(s->phase==2)return 1;
    if(s->phase!=1 || t->callback!=s->banner_wait)return -1;
    if(!t->result)return 0;
    s->phase=2;return 1;
}
