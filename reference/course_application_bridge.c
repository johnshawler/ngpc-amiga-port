/* SPDX-License-Identifier: GPL-2.0-or-later. Test-only original-state adapter. */
#include <string.h>
#include "course_driver.c"
#include "course_application.c"
static ntm_course_application app;
#define d (app.driver)
static unsigned sounds[3];static ntm_course_resources resources;
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
static void sound(void *p,uint8_t n){(void)p;if(n<3)sounds[n]++;}
int bridge_start(const ntm_course_initialization *init,const ntm_course_screen *s,const ntm_display_reset_state *disp,const ntm_course_palette *pal,const ntm_screen_asset *assets,const ntm_palette_job *jobs,const uint8_t *descriptor,const uint8_t *fi,const uint8_t *fo,const uint8_t *random,uint8_t *random_cursor,const ntm_cursor_assets *cursor,const uint8_t *tiles,const uint8_t *maps,const uint8_t *sprites,const uint8_t *spal,const uint8_t *controls,unsigned gate){
 memset(&app,0,sizeof app);memset(sounds,0,sizeof sounds);
 app.initialization=*init;app.screen=*s;app.display=*disp;app.palette=*pal;
 memcpy(app.tiles,tiles,8192);memcpy(app.maps,maps,4096);memcpy(app.sprites,sprites,256);memcpy(app.sprite_palettes,spal,64);memcpy(app.controls,controls,16);
 resources.assets=assets;resources.palette_jobs=jobs;resources.cursor_descriptor=descriptor;resources.fade_in=fi;resources.fade_out=fo;resources.random=random;resources.random_cursor=random_cursor;resources.cursor=cursor;
 d.context.sound_request=sound;d.context.gate=(uint8_t)gate;
 d.parent.viewport[0]=80;d.parent.viewport[3]=152;
 return ntm_course_application_start(&app,&resources);
}
int bridge_update(unsigned input){return ntm_course_application_update(&app,(uint8_t)input);}
void *bridge_pointer(unsigned n){switch(n){case 0:return &app.initialization;case 1:return &app.screen;case 2:return &app.display;case 3:return &app.palette;case 4:return app.controls;case 5:return app.tiles;case 6:return app.maps;case 7:return app.sprites;case 8:return app.sprite_palettes;case 9:return &app.cursor_engine;default:return 0;}}
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
