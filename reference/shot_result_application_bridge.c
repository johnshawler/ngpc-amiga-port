/* SPDX-License-Identifier: GPL-2.0-or-later. Test-only original-state adapter. */
#define NTM_SHOT_BRIDGE_AIM
#define NTM_SHOT_BRIDGE_OPEN
#define NTM_SHOT_BRIDGE_CONTROL
#include <string.h>
#include "course_driver.c"
#include "course_application.c"
#include "course_intro_application.c"
#include "tee_setup_application.c"
#include "shot_application.c"
#ifdef NTM_SHOT_BRIDGE_AIM
#include "shot_aim_application.c"
#ifdef NTM_SHOT_BRIDGE_OPEN
#include "shot_open_application.c"
#ifdef NTM_SHOT_BRIDGE_CONTROL
#include "shot_control_application.c"
static int bridge_dispatch_one(ntm_task_context *);
#define ntm_task_dispatch_one bridge_dispatch_one
#include "shot_fire_application.c"
#undef ntm_task_dispatch_one
#include "shot_power_application.c"
#include "shot_swing_application.c"
#include "shot_result_application.c"
static ntm_shot_result_application result_application;
#define swing_application (result_application.base)
#define power_application (swing_application.base)
#define fire_application (power_application.base)
#define control_application (fire_application.base)
#define open_application (control_application.base)
#else
static ntm_shot_open_application open_application;
#endif
#define aim_application (open_application.base)
#else
static ntm_shot_aim_application aim_application;
#endif
#define shot_application (aim_application.base)
#else
static ntm_shot_application shot_application;
#endif
#define tee_application (shot_application.base)
#define continuation (tee_application.base)
#define app (continuation.selector)
#define d (app.driver)
static unsigned sounds[3],round_sounds[4];static ntm_course_resources resources;
static void round_sound(void *p,uint16_t n){(void)p;if(round_sounds[0]<3)round_sounds[1+round_sounds[0]++]=n;}
static const ntm_course_title_resources *title_resources;
static const ntm_round_record_resources *round_resources;
void bridge_round(const ntm_round_record_resources *r){round_resources=r;}
static const ntm_cursor_assets *title_objects;
void bridge_titles(const ntm_course_title_resources *r,const ntm_cursor_assets *o){title_resources=r;title_objects=o;}
static uint32_t address(ntm_task *n){
 unsigned i;if(!n)return 0;if(n==&continuation.fallback)return 0x7140;
 for(i=0;i<5;i++)if(n==d.nodes+i)return 0x7000+64*i;
 for(i=0;i<4;i++)if(n==shot_application.extra_tasks+i)return 0x7180+64*i;
 for(i=0;i<3;i++)if(n==fire_application.extra_tasks+i)return 0x7280+64*i;
 return 0xdeadbeef;
}
static unsigned tee_sound_count,tee_sounds[64][3];
static const ntm_tee_setup_resources *tee_resources;
static const ntm_hole_prepare_resources *hole_resources;
static const ntm_shot_resources *shot_resources;
void bridge_shot(const ntm_shot_resources *r){shot_resources=r;}
void bridge_tee(const ntm_tee_setup_resources *r,const ntm_hole_prepare_resources *h){tee_resources=r;hole_resources=h;}
static void observe_tee_sound(void *p,uint8_t service,uint16_t command,uint16_t parameter){
 (void)p;if(tee_sound_count<64){tee_sounds[tee_sound_count][0]=service;tee_sounds[tee_sound_count][1]=command;tee_sounds[tee_sound_count++][2]=parameter;}}
static uint32_t callback(ntm_task_callback c){
 if(c==result_flight_entry)return 0x20e065;
 if(c==result_flight_wait)return 0x2087b3;
 if(c==result_perfect_first)return 0x20c6f6;
 if(c==result_perfect_visible)return 0x20c72f;
 if(c==result_perfect_finish)return 0x20c740;
 if(c==result_parent_wait)return 0x209ad3;
 if(c==result_parent_delay)return 0x209af4;
 if(c==result_hit_reset)return 0x209b05;
 if(c==result_perfect_entry)return 0x20c64e;
 if(c==result_perfect_wait)return 0x209b29;
 if(c==result_miss_entry)return 0x2109d5;
 if(c==result_miss_wait)return 0x209b63;
 if(c==result_miss_delay)return 0x210a27;
 if(c==result_miss_continue)return 0x210a38;
 if(c==result_owner_wait)return 0x208799;
 if(c==swing_power_delay)return 0x209a77;
 if(c==swing_power_ready)return 0x209a88;
 if(c==swing_parent_wait)return 0x209ad3;
 if(c==swing_parent_delay)return 0x209af4;
 if(c==swing_hit_reset)return 0x209b05;
 if(c==swing_trajectory_wait)return 0x20b432;
 if(c==swing_trajectory_animate)return 0x20b47f;
 if(c==power_fire_closed)return 0x2099a5;
 if(c==power_fire_open)return 0x2099e1;
 if(c==power_fire_ready)return 0x2099f2;
 if(c==power_delay_tick)return 0x209a2e;
 if(c==power_ready_tick)return 0x209a3f;
 if(c==power_accuracy_delay)return 0x209a77;
 if(c==power_accuracy_ready)return 0x209a88;
 if(c==power_cancel_rebuild)return 0x209b9a;
 if(c==power_cancel_restore)return 0x209bc1;
 if(c==power_cancel_return)return 0x209bd6;
 if(c==power_accuracy_entry)return 0x20bc19;
 if(c==power_accuracy_tick)return 0x20bc5b;
 if(c==power_meter_entry)return 0x20b70b;
 if(c==power_meter_tick)return 0x20b74b;
 if(c==fire_pre_delay)return 0x2098ee;
 if(c==fire_pre_ready)return 0x2098ff;
 if(c==fire_closed)return 0x2099a5;
 if(c==fire_open)return 0x2099e1;
 if(c==fire_ready)return 0x2099f2;
 if(c==fire_marker)return 0x20af8a;
 if(c==fire_marker_wait)return 0x20afa5;
 if(c==fire_spin_entry)return 0x20b604;
 if(c==fire_spin_tick)return 0x20b62d;
 if(c==fire_trajectory_entry)return 0x20b3aa;
 if(c==fire_trajectory_tick)return 0x20b432;
 if(c==fire_golfer_entry)return 0x20a58d;
 if(c==fire_golfer_drawing)return 0x20a5aa;
 if(c==fire_golfer_animation)return 0x20a5c0;

#ifdef NTM_SHOT_BRIDGE_AIM
 if(c==aim_setup_boundary)return 0x209830;
#ifdef NTM_SHOT_BRIDGE_OPEN
#ifdef NTM_SHOT_BRIDGE_CONTROL
 if(c==control_delay)return 0x2098ee;
 if(c==control_ready)return 0x2098ff;
#endif
 if(c==open_scene_entry)return 0x209830;
 if(c==open_wait)return 0x20989e;
 if(c==open_visible_delay)return 0x2098ee;
 if(c==open_hint_entry)return 0x20c744;
 if(c==open_hint_tick)return 0x20c775;
 if(c==open_hint_active)return 0x20c786;
 if(c==open_marker_entry)return 0x20aee1;
 if(c==open_marker_tick)return 0x20af8a;
 if(c==open_marker_waiting)return 0x20afa5;
 if(c==open_club_entry)return 0x20ace7;
 if(c==open_club_tick)return 0x20ad1a;
 if(c==open_camera_entry)return 0x20c883;
 if(c==open_camera_tick)return 0x20c894;
 if(c==open_camera_active)return 0x20c8a5;
#endif
#endif
 if(c==shot_scene_entry)return 0x2096a4;
 if(c==shot_scene_wait)return 0x209749;
 if(c==shot_tee_wait)return 0x208799;
 if(c==shot_banner_entry)return 0x20bf8e;
 if(c==shot_banner_delay)return 0x20c0f7;
 if(c==shot_banner_tick)return 0x20c13e;
 if(c==shot_banner_hold)return 0x20c158;
 if(c==shot_banner_exit_wait)return 0x20c199;
 if(c==shot_notice_entry)return 0x20c3eb;
 if(c==shot_animation_entry)return 0x20c45b;
 if(c==shot_animation_tick)return 0x20c503;
 if(c==shot_exit_entry)return 0x20c564;
 if(c==shot_exit_tick)return 0x20c5b9;
 if(c==tee_owner_tick)return 0x208753;
 if(c==tee_owner_wait)return 0x20876f;
 if(c==tee_outer_wait)return 0x203f8d;
 if(c==tee_overview_entry)return 0x20ca45;
 if(c==tee_tick)return 0x20cb83;
 if(c==tee_delay)return 0x20cd28;
 if(c==tee_open_delay)return 0x20cd5e;
 if(c==tee_hold)return 0x20cd81;
 if(c==tee_wait_handicap)return 0x20cdac;
 if(c==tee_notice_entry)return 0x20ce7c;
 if(c==tee_handicap_entry)return 0x20cf21;
 if(c==tee_handicap_wait)return 0x20cfcc;
 if(c==tee_handicap_cleanup)return 0x20cfdd;
 if(c==round_entry)return 0x21480a;
 if(c==round_tick)return 0x2148b6;
 if(c==round_open)return 0x2148ca;
 if(c==round_wait)return 0x21490a;
 if(c==round_cleanup)return 0x214921;
 if(c==round_draw_entry)return 0x214941;
 if(c==round_draw_finish)return 0x214954;
 if(c==round_blink_entry)return 0x214c10;
 if(c==round_blink_tick)return 0x214c25;
 if(c==round_blink_shown)return 0x214c4b;
 if(c==round_blink_hidden)return 0x214c73;
 if(c==owner_wait_round)return 0x203f73;
 if(c==owner_tick)return 0x203f31;
 if(c==owner_wait_intro)return 0x203f59;
 if(c==intro_entry)return 0x20771d;
 if(c==intro_tick)return 0x2077d3;
 if(c==intro_visible)return 0x207809;
 if(c==intro_title)return 0x20781a;
 if(c==title_tick)return 0x2078da;
 if(c==tournament_delay)return 0x207959;
 if(c==tournament_wait)return 0x207990;
 if(c==title_cleanup)return 0x2079a7;
 if(c==ntm_task_finish)return 0x20132b;
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
int bridge_start(const ntm_course_initialization *init,const ntm_course_screen *s,const ntm_display_reset_state *disp,const ntm_course_palette *pal,const ntm_screen_asset *assets,const ntm_palette_job *jobs,const uint8_t *descriptor,const uint8_t *fi,const uint8_t *fo,const uint8_t *random,uint8_t *random_cursor,const ntm_cursor_assets *cursor,const uint8_t *tiles,const uint8_t *maps,const uint8_t *sprites,const uint8_t *spal,const uint8_t *controls,unsigned gate,const ntm_course_intro_resources *courses,const ntm_course_records *records){
 memset(&shot_application,0,sizeof shot_application);tee_sound_count=0;
 tee_application.context.sound_request=observe_tee_sound;
 if(records)continuation.records=*records;
 memset(sounds,0,sizeof sounds);memset(round_sounds,0,sizeof round_sounds);
 continuation.round_context.sound_request=round_sound;
 app.initialization=*init;app.screen=*s;app.display=*disp;app.palette=*pal;
 memcpy(app.tiles,tiles,8192);memcpy(app.maps,maps,4096);memcpy(app.sprites,sprites,256);memcpy(app.sprite_palettes,spal,64);memcpy(app.controls,controls,16);
 resources.assets=assets;resources.palette_jobs=jobs;resources.cursor_descriptor=descriptor;resources.fade_in=fi;resources.fade_out=fo;resources.random=random;resources.random_cursor=random_cursor;resources.cursor=cursor;
 d.context.sound_request=sound;d.context.gate=(uint8_t)gate;
 d.parent.viewport[0]=80;d.parent.viewport[3]=152;
 return shot_resources?ntm_shot_application_start(&shot_application,&resources,courses,title_resources,title_objects,round_resources,tee_resources,hole_resources,shot_resources):tee_resources?ntm_tee_setup_application_start(&tee_application,&resources,courses,title_resources,title_objects,round_resources,tee_resources,hole_resources):round_resources?ntm_course_intro_application_start_round_record(&continuation,&resources,courses,title_resources,title_objects,round_resources):title_resources?ntm_course_intro_application_start_titles(&continuation,&resources,courses,title_resources,title_objects):
     ntm_course_intro_application_start(&continuation,&resources,courses);
}
static uint32_t dispatch_trace[24];static unsigned dispatch_count;
static int bridge_dispatch_one(ntm_task_context *c)
{
 if(dispatch_count>=12)return -1;
 dispatch_trace[2*dispatch_count]=address(c->current);dispatch_trace[2*dispatch_count+1]=callback(c->current->callback);dispatch_count++;
 return ntm_task_dispatch_one(c);
}
unsigned bridge_trace(uint32_t *v){unsigned i;for(i=0;i<2*dispatch_count;i++)v[i]=dispatch_trace[i];return dispatch_count;}
int bridge_update(unsigned input){dispatch_count=0;return ntm_shot_result_application_update(&result_application,(uint8_t)input);}
void *bridge_pointer(unsigned n){switch(n){case 0:return &app.initialization;case 1:return &app.screen;case 2:return &app.display;case 3:return &app.palette;case 4:return app.controls;case 5:return app.tiles;case 6:return app.maps;case 7:return app.sprites;case 8:return app.sprite_palettes;case 9:return &app.cursor_engine;case 10:return &continuation.owner;case 11:return &continuation.records;case 12:return &continuation.intro;case 13:return &continuation.intro_context;case 14:return &continuation;case 15:return &continuation.title_cursor;case 16:return &continuation.title;case 17:return &continuation.title_context;case 18:return &continuation.round_prepare;case 19:return &continuation.round_context;case 20:return &continuation.round_cursor;case 21:return &continuation.round_drawing;case 22:return &continuation.round;case 23:return &continuation.round_blink;case 24:return tee_application.objects;case 25:return &tee_application.engine;case 26:return &tee_application.context;case 27:return &tee_application.hole;case 28:return tee_application.x;case 29:return &tee_application.owner;case 30:return &tee_application.overview;case 31:return &shot_application.scene;case 32:return &shot_application.banner;case 33:return &shot_application.animation;case 34:return &shot_application.exit;case 35:return &shot_application.raster;case 36:return &shot_application.palette;default:return 0;}}
void bridge_export(uint32_t *v){unsigned i;for(i=0;i<5;i++){
 ntm_task *t=d.nodes+i;*v++=callback(t->callback);*v++=address(t->previous);*v++=address(t->next);*v++=address(t->owner);*v++=t->priority;*v++=t->result;*v++=t->flags;*v++=t->substate;*v++=t->delay;}
 *v++=d.context.free_head?address(d.context.free_head):0xffffffff;*v++=address(d.context.current);*v++=address(d.context.published);
 *v++=d.context.selection;*v++=tee_application.started?tee_application.objects[0].velocity:continuation.round.phase?continuation.round_cursor.velocity:continuation.title.phase?continuation.title_cursor.velocity:d.parent.global_word;*v++=tee_application.started?tee_application.objects[0].flags:continuation.round.phase?continuation.round_cursor.flags:continuation.title.phase?continuation.title_cursor.flags:d.parent.global_flags;*v++=d.parent.random_remaining;*v++=d.parent.mapped_course;
 for(i=0;i<4;i++)*v++=d.parent.viewport[i];
 *v++=d.parent.course_index;
 for(i=0;i<3;i++)*v++=d.parent.course_order[i];
 for(i=0;i<3;i++)*v++=sounds[i];
 *v++=d.running;
 *v++=continuation.owner.phase;*v++=continuation.intro.phase;
 *v++=continuation.intro.asset_group;
 *v++=continuation.title.phase?(uint32_t)(continuation.title.descriptor_next-continuation.courses[d.parent.mapped_course].descriptor):
     continuation.intro.descriptor_next?(uint32_t)(continuation.intro.descriptor_next-continuation.courses[d.parent.mapped_course].descriptor):0;
 *v++=continuation.boundary;*v++=continuation.selector_exit_update;
 *v++=continuation.intro_start_update;*v++=continuation.first_visible_update;*v++=d.updates;
}

void bridge_title_export(uint32_t *v){*v++=continuation.title.phase;*v++=continuation.title_start_update;*v++=continuation.tournament_start_update;*v++=continuation.title_complete_update;*v++=continuation.title_x;}

void bridge_round_export(uint32_t *v){*v++=continuation.round.phase;*v++=continuation.round_start_update;*v++=continuation.round_open_update;*v++=continuation.round_complete_update;*v++=continuation.round_x;*v++=continuation.round_drawing_update;}

void bridge_round_sounds(uint32_t *v){unsigned i;for(i=0;i<4;i++)v[i]=round_sounds[i];}

void bridge_tee_export(uint32_t *v){
 *v++=tee_application.started;*v++=tee_application.boundary;*v++=tee_application.overview.phase;
 *v++=tee_application.start_update;*v++=tee_application.overview_update;*v++=tee_application.open_update;
 *v++=tee_application.hold_update;*v++=tee_application.complete_update;
 *v++=tee_application.overview.asset_group;*v++=tee_application.overview.asset_index;
 *v++=tee_application.owner.current_music;*v++=tee_application.owner.skip_overview;*v++=tee_application.owner.player_status;
}
unsigned bridge_tee_sounds(uint32_t *v){unsigned i,j;for(i=0;i<tee_sound_count;i++)for(j=0;j<3;j++)*v++=tee_sounds[i][j];return tee_sound_count;}

void bridge_tee_state(unsigned skip,unsigned music,unsigned badge){
 unsigned i;tee_application.owner.skip_overview=(uint8_t)skip;tee_application.owner.current_music=(uint8_t)music;
 if(badge)for(i=0;i<54;i++)tee_application.saved_hole_scores[i]=1;
}

void bridge_extra_export(uint32_t *v){unsigned i;for(i=0;i<4;i++){
 ntm_task *t=shot_application.extra_tasks+i;*v++=callback(t->callback);*v++=address(t->previous);*v++=address(t->next);*v++=address(t->owner);*v++=t->priority;*v++=t->result;*v++=t->flags;*v++=t->substate;*v++=t->delay;}}
void bridge_shot_export(uint32_t *v){
 ntm_shot_application *a=&shot_application;
 *v++=a->started;*v++=a->boundary;*v++=a->scene.phase;*v++=a->banner.phase;
 *v++=a->start_update;*v++=a->banner_update;*v++=a->complete_update;
 *v++=a->scene.player.selection;*v++=a->scene.player.active_player;*v++=a->scene.player.other_player;
 *v++=a->scene.player.stroke_snapshot;*v++=a->scene.terrain_detail;*v++=a->scene.player_status;
 *v++=a->banner.blink_delay;*v++=a->banner.blink_state;*v++=a->animation.variant;*v++=a->animation.inactive;
 *v++=a->animation.phase;*v++=a->exit.phase;
}

/* SPDX-License-Identifier: GPL-2.0-or-later. Test-only original-state adapter. */
void bridge_aim_reset(void){memset(&aim_application,0,sizeof aim_application);}
int bridge_aim_enable(const ntm_shot_aim_application_resources *r)
{return ntm_shot_aim_application_enable(&aim_application,r);}
void *bridge_aim_pointer(unsigned n)
{
 switch(n){case 0:return &aim_application.aim;case 1:return &aim_application.world;
 case 2:return aim_application.saved_hud;case 3:return &aim_application.phase;default:return 0;}
}
/* Exact scheduler drawing head only. This test function never dispatches the
 * unconverted209830 callback or advances input, tasks, or update count. */
int bridge_aim_drain_head(void)
{
 ntm_course_driver *driver=&d;
 if(driver->service_pump(driver->service_context)<0 ||
    ntm_course_screen_fill_queue(driver->screen,driver->maps)<0 ||
    ntm_course_screen_stamp_queue(driver->screen,driver->tiles,driver->maps)<0)return -1;
 return driver->drawing_head(driver->service_context);
}

/* SPDX-License-Identifier: GPL-2.0-or-later. Test-only original-state adapter. */
void bridge_open_reset(void){memset(&open_application,0,sizeof open_application);}
int bridge_open_enable(const ntm_shot_open_resources *r)
{return ntm_shot_open_application_enable(&open_application,r);}
void *bridge_open_pointer(unsigned n)
{
 switch(n){case 0:return &open_application.club;case 1:return &open_application.markers;
 case 2:return &open_application.hint;case 3:return &open_application.camera;
 case 4:return open_application.saved_map;case 5:return &open_application.phase;
 case 6:return &open_application.handlers_installed;case 7:return &open_application.input_dispatch;default:return 0;}
}

/* SPDX-License-Identifier: GPL-2.0-or-later. Test-only adapter. */
void bridge_control_reset(void){memset(&control_application,0,sizeof control_application);}
int bridge_control_enable(void){return ntm_shot_control_application_enable(&control_application);}
void *bridge_control_pointer(unsigned n)
{
 switch(n){case 0:return &control_application.camera;case 1:return &control_application.ready;
 case 2:return &control_application.partial_boundary;case 3:return &control_application.ready_update;
 case 4:return app.scroll_x;case 5:return app.scroll_y;default:return 0;}
}

void bridge_fire_reset(void){memset(&fire_application,0,sizeof fire_application);}
int bridge_fire_enable(const ntm_shot_fire_application_resources *r){return ntm_shot_fire_application_enable(&fire_application,r);}
void *bridge_fire_pointer(unsigned n)
{
 switch(n){case 0:return &fire_application.fire;case 1:return &fire_application.spin;
 case 2:return &fire_application.trajectory;case 3:return &fire_application.golfer;
 case 4:return &fire_application.drawing;case 5:return &fire_application.boundary;
 case 6:return &fire_application.fire_update;case 7:return &fire_application.drawing_update;
 case 8:return &fire_application.golfer_context;default:return 0;}
}
void bridge_fire_export(uint32_t *v)
{
 unsigned i;for(i=0;i<3;i++){
 ntm_task *t=fire_application.extra_tasks+i;*v++=callback(t->callback);*v++=address(t->previous);*v++=address(t->next);*v++=address(t->owner);
 *v++=t->priority;*v++=t->result;*v++=t->flags;*v++=t->substate;*v++=t->delay;
 }
}

void bridge_power_reset(void){memset(&power_application,0,sizeof power_application);}
int bridge_power_enable(const ntm_shot_power_application_resources *r){return ntm_shot_power_application_enable(&power_application,r);}
void *bridge_power_pointer(unsigned n)
{switch(n){case 0:return &power_application.power;case 1:return &power_application.meter;case 2:return &power_application.boundary;case 3:return &power_application.power_update;case 4:return &power_application.accuracy;default:return 0;}}

void bridge_swing_reset(void){memset(&swing_application,0,sizeof swing_application);}
int bridge_swing_enable(const ntm_shot_swing_application_resources *r){return ntm_shot_swing_application_enable(&swing_application,r);}
void *bridge_swing_pointer(unsigned n)
{switch(n){case 0:return &swing_application.swing;case 1:return &swing_application.trajectory;case 2:return &swing_application.boundary;default:return 0;}}

void bridge_result_reset(void){memset(&result_application,0,sizeof result_application);}
int bridge_result_enable(const ntm_shot_result_application_resources *r){return ntm_shot_result_application_enable(&result_application,r);}
void *bridge_result_pointer(unsigned n)
{switch(n){case 0:return &result_application.exit;case 1:return &result_application.miss;case 2:return &result_application.boundary;case 3:return &result_application.perfect;default:return 0;}}
