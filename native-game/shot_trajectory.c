/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_trajectory.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static int valid(const ntm_shot_trajectory *s,const ntm_shot_scene_object_context *c,
 const ntm_shot_trajectory_resources *r)
{
 return s && c && c->shared && c->shared->intro && c->shared->objects &&
 c->shared->x && c->shared->intro->screen && c->shared->intro->display &&
 c->shared->intro->screen_words && c->hole && c->player && c->player->active_player<2 &&
 c->hole->players[c->player->active_player][1]<6 && c->aim && r;
}
int ntm_shot_trajectory_tick(ntm_shot_trajectory *s,ntm_shot_scene_object_context *c,
 const ntm_shot_trajectory_resources *r,uint8_t label)
{
 unsigned category,golfer;ntm_course_cursor *o;
 if(!valid(s,c,r) || s->phase!=1)return -1;
 if(c->aim->reset_d1==4)return 2;
 if(label==s->last_label)return 0;
 category=(label^128u)<=137u?0:(label^128u)<=148u?1:2;
 if(!r->hud_bases || r->hud_bases_size<6)return -1;
 for(golfer=0;golfer<6;golfer++)if(s->hud_base==4u*r->hud_bases[golfer])break;
 if(golfer==6)return -1;
 s->last_label=label;s->hud_delta=(uint16_t)(60*category);
 o=c->shared->objects+16;o->velocity=(uint16_t)(s->sprite_base+8*category);o->flags|=128;
 return ntm_course_screen_enqueue_stamp(c->shared->intro->screen,&r->hud[golfer][category],0x2ca,0x2be0)<0?-1:0;
}
int ntm_shot_trajectory_initialize(ntm_shot_trajectory *s,ntm_task *t,
 ntm_shot_scene_object_context *c,const ntm_shot_trajectory_resources *r,uint8_t label)
{
 unsigned g;ntm_course_cursor o;ntm_display_reset_state d;ntm_tee_setup_context *q;
 if(!valid(s,c,r) || !t || !s->wait || s->phase || !r->metadata || r->metadata_size<5 ||
 !r->sprite_bases || r->sprite_bases_size<6 || !r->hud_bases || r->hud_bases_size<6)return -1;
 q=c->shared;g=c->hole->players[c->player->active_player][1];
 if(!ntm_course_palette_job_valid(q->intro->palette,&r->palettes[g]))return -1;
 o=q->objects[16];o.tag=27;o.width=word(r->metadata);o.velocity=word(r->metadata+2);
 o.byte15=r->metadata[4];o.y=0;o.second=o.byte14=0;o.tile=84;o.flags=128;
 d=*q->intro->display;if(ntm_tee_object_register(&d,&o)<0)return -1;
 /* object_tags mirrors the original record word, even when registration
  * is dropped because the list is full. */
 d.object_tags[16]=o.tag;
 s->counter=0;s->last_label=255;s->sprite_base=(uint16_t)(4*r->sprite_bases[g]);
 s->hud_base=(uint16_t)(4*r->hud_bases[g]);
 (void)ntm_course_palette_enqueue(q->intro->palette,&r->palettes[g]);
 *q->intro->display=d;q->objects[16]=o;
 q->x[16]=(uint16_t)(40+q->intro->screen_words[0]);
 q->objects[16].y=(uint16_t)(88+q->intro->screen_words[1]);q->objects[16].flags|=64;
 t->callback=s->wait;s->phase=1;
 return ntm_shot_trajectory_tick(s,c,r,label);
}
