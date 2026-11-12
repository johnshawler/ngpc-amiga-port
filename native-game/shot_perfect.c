/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_perfect.h"
static int valid(const ntm_shot_perfect *s,const ntm_task *t,const ntm_shot_scene_object_context *c)
{
 return s && t && c && c->shared && c->shared->tasks && c->shared->intro &&
  c->shared->intro->viewport && s->first_delay && s->visible_delay && s->finish && t->next && t->previous;
}
int ntm_shot_perfect_tick(ntm_shot_perfect *s,ntm_task *t,ntm_shot_scene_object_context *c)
{
 uint8_t *v;int outcome;
 if(!valid(s,t,c) || (s->phase!=1 && s->phase!=2) || t->callback!=(s->phase==1?s->first_delay:s->visible_delay))return -1;
 t->delay=(uint16_t)(t->delay-1u);if(!(t->delay&32768u))return 0;
 if(s->phase==1){
  v=c->shared->intro->viewport;v[0]=v[1]=0;v[2]=160;v[3]=152;
  t->delay=(uint16_t)(s->duration-1u);t->callback=s->visible_delay;s->phase=2;
  if(!(t->delay&32768u))return 0;
 }
 t->callback=s->finish;outcome=ntm_task_finish(c->shared->tasks,t);if(outcome>=0)s->phase=3;return outcome;
}
int ntm_shot_perfect_initialize(ntm_shot_perfect *s,ntm_task *t,ntm_shot_scene_object_context *c,const ntm_shot_perfect_resources *r)
{
 ntm_course_intro_context *i;unsigned g,k;
 if(!valid(s,t,c) || s->phase || !r || !c->player || c->player->active_player>1 || !c->hole)return -1;
 i=c->shared->intro;g=c->hole->players[c->player->active_player][1];
 if(g>=6 || !i->screen || !i->palette || !i->scroll_x || !i->scroll_y)return -1;
 for(k=0;k<2;k++)if(!ntm_course_palette_job_valid(i->palette,r->palettes+k))return -1;
 s->kind=0;s->duration=30;i->viewport[0]=i->viewport[1]=i->viewport[2]=0;i->viewport[3]=152;
 i->scroll_x[0]=i->scroll_x[1]=i->scroll_y[0]=i->scroll_y[1]=0;
 for(k=0;k<2;k++)(void)ntm_course_palette_enqueue(i->palette,r->palettes+k);
 (void)ntm_course_screen_enqueue_stamp(i->screen,r->player_art+g,0,0x40);
 (void)ntm_course_screen_enqueue_stamp(i->screen,&r->overlay,0x800,0x132);
 t->delay=2;t->callback=s->first_delay;s->phase=1;return ntm_shot_perfect_tick(s,t,c);
}
