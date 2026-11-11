/* SPDX-License-Identifier: GPL-2.0-or-later. Test-only original-state adapter. */
#include <string.h>
#include "course_driver.c"
#include "course_palette.h"
static ntm_course_driver d;static unsigned sounds[3];static const ntm_palette_job *completion;
static uint32_t address(ntm_task *n){return n?0x7000u+64u*(unsigned)(n-d.nodes):0;}
static uint32_t callback(ntm_task_callback c){
 if(c==driver_render)return 0x2095e3;
 if(c==driver_wait)return 0x2093b8;
 if(c==driver_cancel)return 0x2093ed;
 if(c==driver_accept)return 0x2094cb;
 if(c==driver_random)return 0x209485;
 if(c==driver_cycle)return 0x209496;
 if(c==driver_fade)return 0x209416;
 if(c==driver_setup)return 0x2094dc;
 if(c==driver_cancel_exit)return 0x20942d;
 if(c==driver_accept_exit)return 0x209508;
 if(c==driver_initialize)return 0x209395;
 if(c==driver_intro_reset)return 0x209366;
 if(c==driver_intro_fade)return 0x20937e;
 if(c==ntm_course_task)return 0x209536;
 return 0;
}
static int head(void *v){return ntm_course_palette_head(v);}static int tail(void *v){return ntm_course_palette_tail(v);}
static void sound(void *v,uint8_t n){(void)v;
 if(n<3)sounds[n]++;}
static int submit(void *v,uint16_t command)
{return command==84?ntm_course_palette_enqueue(v,completion):-1;}
int bridge_start(ntm_course_screen *s,ntm_display_reset_state *disp,ntm_course_palette *pal,const ntm_screen_asset *labels,const uint8_t *fi,const uint8_t *fo,const uint8_t *random,uint8_t *tiles,uint8_t *maps,uint8_t *controls,uint8_t *random_cursor,unsigned selection,unsigned gate,const ntm_palette_job *job){
 memset(&d,0,sizeof d);memset(sounds,0,sizeof sounds);completion=job;
 d.context.controls=controls;d.context.selection=selection;d.context.gate=gate;d.context.sound_request=sound;
 d.screen=s;d.display=disp;d.labels=labels;d.fade_window=fo;d.random_window=random;d.random_cursor=random_cursor;d.tiles=tiles;d.maps=maps;
 d.service_pump=head;d.service_tail=tail;d.completion_submit=submit;d.service_context=pal;
 d.parent.global_word=0;d.parent.global_flags=128;d.parent.viewport[0]=80;d.parent.viewport[1]=0;d.parent.viewport[2]=0;d.parent.viewport[3]=152;
 return ntm_course_driver_start_intro(&d,fi);
}
int bridge_update(unsigned input){return ntm_course_driver_update(&d,(uint8_t)input);}
void bridge_export(uint32_t *v){unsigned i;for(i=0;i<5;i++){
 ntm_task *t=d.nodes+i;*v++=callback(t->callback);*v++=address(t->previous);*v++=address(t->next);*v++=address(t->owner);*v++=t->priority;*v++=t->result;*v++=t->flags;*v++=t->substate;*v++=t->delay;}
 *v++=d.context.free_head?address(d.context.free_head):0xffffffff;*v++=address(d.context.current);*v++=address(d.context.published);
 *v++=d.context.selection;*v++=d.parent.global_word;*v++=d.parent.global_flags;*v++=d.parent.random_remaining;*v++=d.parent.mapped_course;
 for(i=0;i<4;i++)*v++=d.parent.viewport[i];
 *v++=d.parent.course_index;
 for(i=0;i<3;i++)*v++=d.parent.course_order[i];
 for(i=0;i<3;i++)*v++=sounds[i];
 *v++=d.running;
}
