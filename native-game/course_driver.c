/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_driver.h"
#include "input_state.h"
static int driver_render(ntm_task_context *c,ntm_task *t)
{
    ntm_course_driver *d=(ntm_course_driver *)c;(void)t;
    return ntm_course_screen_tick(d->screen,c->selection,d->labels)<0?-1:0;
}
static int driver_services(ntm_course_driver *d,ntm_task *t,int request)
{
    unsigned budget=16;
    while(request>0 && budget--) {
        if(request==NTM_PARENT_RENDER)request=ntm_course_parent_render(&d->context,t,&d->parent,d->screen,d->labels);
        else if(request==NTM_PARENT_RANDOM)request=ntm_course_parent_generate_random(&d->context,t,&d->parent,d->random_cursor,d->random_window);
        else if(request==NTM_PARENT_FADE)request=ntm_course_parent_fade(&d->context,t,&d->parent,d->fade_window);
        else if(request==NTM_PARENT_COURSE_SETUP)request=ntm_course_parent_setup(&d->context,t,&d->parent);
        else if(request==NTM_PARENT_SERVICE54) {
            if(d->completion_submit(d->service_context,84)<0)return -1;
            request=ntm_course_parent_resume(&d->context,t,&d->parent);
        }
        else if(request==NTM_PARENT_EXIT)request=ntm_course_parent_exit(&d->context,t,&d->parent,d->display);
        else if(request==NTM_PARENT_SOUND_ZERO || request==NTM_PARENT_SOUND_ONE || request==NTM_PARENT_SOUND_TWO) {
            uint8_t sound=request==NTM_PARENT_SOUND_ZERO?0:request==NTM_PARENT_SOUND_ONE?1:2;
            if(d->context.sound_request)d->context.sound_request(d->context.sound_context,sound);
            request=ntm_course_parent_resume(&d->context,t,&d->parent);
        }else return -1;
    }
    return request==0?0:-1;
}
static int driver_wait(ntm_task_context *c,ntm_task *t)
{ntm_course_driver *d=(ntm_course_driver *)c;return driver_services(d,t,ntm_course_parent_begin(c,t,&d->parent));}
static int driver_tick(ntm_task_context *c,ntm_task *t)
{ntm_course_driver *d=(ntm_course_driver *)c;return driver_services(d,t,ntm_course_parent_tick(c,t,&d->parent));}
/* Distinct callback identities are part of the converted timer state machine. */
#define CONT(name) static int name(ntm_task_context *c,ntm_task *t){return driver_tick(c,t);}
CONT(driver_cancel) CONT(driver_accept) CONT(driver_random) CONT(driver_cycle)
CONT(driver_fade) CONT(driver_setup) CONT(driver_cancel_exit) CONT(driver_accept_exit)
static int driver_initialize(ntm_task_context *c,ntm_task *t)
{ntm_course_driver *d=(ntm_course_driver *)c;return ntm_course_parent_initialize(c,t,driver_render,d->nodes+4,&d->parent);}
static int driver_intro_fade(ntm_task_context *c,ntm_task *t)
{
    ntm_course_driver *d=(ntm_course_driver *)c;int busy;
    t->delay=(uint16_t)(t->delay-1u);if(!(t->delay&0x8000))return 0;
    busy=ntm_viewport_fade(t,d->parent.viewport,d->fade_in_window);
    if(busy<0)return -1;
    return busy?0:driver_initialize(c,t);
}
static int driver_intro_reset(ntm_task_context *c,ntm_task *t)
{(void)c;t->substate=0;t->delay=0;t->callback=driver_intro_fade;return 0;}
int ntm_course_driver_start(ntm_course_driver *d)
{
    ntm_task *n;
    if(!d || d->running || !d->context.controls || d->context.selection>2 || !d->screen || !d->display ||
       !d->labels || !d->fade_window || !d->random_window || !d->random_cursor || !d->tiles || !d->maps || !d->service_pump || !d->service_tail || !d->completion_submit)return -1;
    n=d->nodes;
    n[0].next=n+1;n[0].previous=n+1;n[0].priority=0;
    n[1].next=n;n[1].previous=n;n[1].owner=n+4;n[1].priority=1;n[1].callback=driver_initialize;
    n[2].next=n+3;n[3].next=0;d->context.free_head=n+2;d->context.current=n+1;
    d->parent.wait_callback=driver_wait;d->parent.cancel_delay_callback=driver_cancel;d->parent.accept_delay_callback=driver_accept;
    d->parent.random_delay_callback=driver_random;d->parent.random_cycle_callback=driver_cycle;d->parent.fade_callback=driver_fade;
    d->parent.setup_callback=driver_setup;d->parent.cancel_exit_callback=driver_cancel_exit;d->parent.accept_exit_callback=driver_accept_exit;
    d->updates=0;d->running=1;return 0;
}
int ntm_course_driver_update(ntm_course_driver *d,uint8_t buttons)
{
    unsigned budget=8;
    if(!d || !d->running)return -1;
    if(ntm_input_update(d->context.controls,buttons)<0 || d->service_pump(d->service_context)<0 ||
       ntm_course_screen_fill_queue(d->screen,d->maps)<0 || ntm_course_screen_stamp_queue(d->screen,d->tiles,d->maps)<0)return -1;
    if(d->drawing_head && d->drawing_head(d->service_context)<0)return -1;
    d->context.current=d->nodes[0].next;
    while(d->context.current!=d->nodes) {
        if(!budget-- || ntm_task_dispatch_one(&d->context)<0)return -1;
    }
    if(d->service_tail(d->service_context)<0)return -1;
    d->updates++;
    if(d->parent.phase==NTM_PARENT_DONE){d->running=0;return 1;}
    return 0;
}

int ntm_course_driver_start_intro(ntm_course_driver *d,const uint8_t window[256])
{
    if(!window || ntm_course_driver_start(d)<0)return -1;
    d->fade_in_window=window;d->nodes[1].callback=driver_intro_reset;return 0;
}
