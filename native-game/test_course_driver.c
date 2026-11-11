/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_driver.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t tiles[8192],maps[4096],controls[16],table[256],cursor;
static uint8_t image[]={1,0,0,0,0,0,0,0,129,0,0,255,255},pixels[16];
static unsigned sounds[3],heads,tails,submissions;
static int submit(void *p,uint16_t command){(void)p;assert(command==84);submissions++;return 0;}
static int head(void *p){(void)p;heads++;assert(heads==tails+1);return 0;}
static int tail(void *p){(void)p;tails++;assert(heads==tails);return 0;}
static void sound(void *p,uint8_t value){(void)p;assert(value<3);sounds[value]++;}
static void start(ntm_course_driver *d,ntm_course_screen *screen,ntm_service54_state *queue,ntm_display_reset_state *display,ntm_screen_asset labels[3],int intro)
{
    unsigned i;memset(d,0,sizeof *d);memset(screen,0,sizeof *screen);memset(queue,0,sizeof *queue);
    memset(controls,0,sizeof controls);memset(tiles,0,sizeof tiles);memset(maps,0,sizeof maps);memset(sounds,0,sizeof sounds);
    heads=tails=submissions=0;screen->cached_selection=65535;memset(screen->fill_status,255,8);
    for(i=0;i<8;i++)queue->slots[i]=(uint16_t)(0x8000+4*i);
    for(i=0;i<3;i++){labels[i].data=image;labels[i].size=sizeof image;labels[i].tiles=pixels;labels[i].tiles_size=sizeof pixels;}
    for(i=0;i<256;i++)table[i]=(uint8_t)(i*73);
    cursor=7;d->context.controls=controls;d->context.sound_request=sound;
    d->screen=screen;d->display=display;d->labels=labels;d->fade_window=table;d->random_window=table;
    d->random_cursor=&cursor;d->tiles=tiles;d->maps=maps;d->service_pump=head;d->service_tail=tail;d->completion_submit=submit;
    d->parent.viewport[2]=160;d->parent.viewport[3]=152;assert(!(intro?ntm_course_driver_start_intro(d,table):ntm_course_driver_start(d)));
}
static int boundary_task(ntm_task_context *c,ntm_task *t){assert(c->published==t);return NTM_TASK_BOUNDARY;}
static int boundary_head(void *p){(void)p;assert(heads==1 && tails==0);return NTM_TASK_BOUNDARY;}
int main(void)
{
    ntm_course_driver d;ntm_course_screen screen;ntm_service54_state queue;ntm_display_reset_state display;ntm_screen_asset labels[3];unsigned mode,steps;
    for(mode=0;mode<3;mode++) {
        start(&d,&screen,&queue,&display,labels,0);
        assert(!ntm_course_driver_update(&d,0));assert(screen.stamp_bytes==8 && d.nodes[1].next==d.nodes+3 && !maps[770]);
        assert(!ntm_course_driver_update(&d,8));assert(d.context.selection==1 && sounds[2]==1 && screen.stamp_bytes==8);
        assert(maps[770]==45 && !maps[782]); /* Previous selection drains first. */
        assert(!ntm_course_driver_update(&d,0));assert(maps[782]==45 && !maps[770]);
        assert(!ntm_course_driver_update(&d,(uint8_t)(mode==0?16:mode==1?32:80)));
        assert(d.nodes[1].result==(mode==0?1:mode==1?2:3) && d.parent.phase==NTM_PARENT_WAIT);
        assert(d.nodes[1].next==d.nodes+2); /* Selector removed, renderer still traversed. */
        steps=0;
        while(d.running){int r=ntm_course_driver_update(&d,0);assert(r==0 || r==1);assert(++steps<120);}
        assert(submissions==(mode==1?0u:1u));
        assert(d.nodes[4].result==(mode==1?252:0));assert(heads==tails && heads==d.updates);
        assert(sounds[mode==1?1:0]==1 && sounds[2]==(mode==2?31u:1u));
        assert(d.nodes[0].next==d.nodes && d.nodes[0].previous==d.nodes);
    }
    start(&d,&screen,&queue,&display,labels,1);
    for(steps=0;steps<7;steps++){assert(!ntm_course_driver_update(&d,0));assert(d.nodes[1].next==d.nodes);}
    assert(!ntm_course_driver_update(&d,8) && d.context.selection==1 && d.nodes[1].next==d.nodes+3);
    assert(screen.stamp_bytes==8 && d.nodes[1].substate==14);
    start(&d,&screen,&queue,&display,labels,0);d.pre_task=boundary_head;
    assert(ntm_course_driver_update(&d,4)==1 && !d.running && !d.updates && heads==1 && !tails && controls[0]==4);
    start(&d,&screen,&queue,&display,labels,0);d.nodes[1].callback=boundary_task;
    assert(ntm_course_driver_update(&d,8)==1 && !d.running && !d.updates && heads==1 && !tails);
    assert(d.context.current==d.nodes+1 && d.context.published==d.nodes+1 && controls[0]==8);
    puts("PASS native course driver: navigation, deferred drawing, confirmation, cancellation and random choice");return 0;
}
