/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_application.h"
static int application_head(void *p){return ntm_course_palette_head(&((ntm_course_application *)p)->palette);}
static int application_drawing_head(void *p)
{
    ntm_course_application *a=p;
    if(!a->resources->cursor)return 0;
    if((a->initialization.flags&80)==80 && ntm_course_cursor_head(&a->cursor_engine,&a->display,a->tiles,a->sprites,a->sprite_palettes,a->resources->cursor)<0)return -1;
    a->initialization.flags&=63;return 0;
}
static int application_tail(void *p)
{
    ntm_course_application *a=p;ntm_course_cursor *c=&a->initialization.cursor;
    if(ntm_course_palette_tail(&a->palette)<0)return -1;
    if(a->resources->cursor && (a->initialization.flags&144)==16) {
        c->velocity=a->driver.parent.global_word;c->flags=a->driver.parent.global_flags;
        c->tag=a->display.object_tags[0];c->id=a->display.object_ids[0];
        if(ntm_course_cursor_tail(&a->cursor_engine,c,&a->display,a->screen.cursor_x,a->initialization.screen_words,&a->initialization.flags,a->resources->cursor)<0)return -1;
        a->driver.parent.global_flags=c->flags;a->initialization.flags|=192;
    }
    return 0;
}
static int application_submit(void *p,uint16_t command)
{
    ntm_course_application *a=p;
    return command==84?ntm_course_palette_enqueue(&a->palette,a->resources->palette_jobs+2):-1;
}
int ntm_course_application_start(ntm_course_application *a,const ntm_course_resources *r)
{
    ntm_course_driver *d;int result;
    if(!a || !r || !r->assets || !r->palette_jobs || !r->cursor_descriptor || !r->fade_in || !r->fade_out || !r->random || !r->random_cursor || a->driver.running)return -1;
    result=ntm_course_initialize(&a->initialization,&a->screen,&a->display,&a->palette,a->sprites,a->sprite_palettes,r->assets,r->cursor_descriptor,r->palette_jobs);
    if(result)return result; /* Guard bypass requires the alternate parent path. */
    a->resources=r;d=&a->driver;
    d->context.controls=a->controls;d->context.selection=a->initialization.selection;
    d->parent.global_word=a->initialization.cursor.velocity;d->parent.global_flags=a->initialization.cursor.flags;
    d->parent.mapped_course=a->initialization.mapped_course;
    d->screen=&a->screen;d->display=&a->display;d->labels=r->assets+2;
    d->fade_window=r->fade_out;d->random_window=r->random;d->random_cursor=r->random_cursor;
    d->tiles=a->tiles;d->maps=a->maps;d->service_context=a;
    d->service_pump=application_head;d->service_tail=application_tail;d->drawing_head=application_drawing_head;d->completion_submit=application_submit;
    if(application_tail(a)<0)return -1; /* Initializer tick's deferred palette preparation. */
    return ntm_course_driver_start_intro(d,r->fade_in);
}
int ntm_course_application_update(ntm_course_application *a,uint8_t buttons)
{
    int result;if(!a)return -1;result=ntm_course_driver_update(&a->driver,buttons);
    a->initialization.selection=a->driver.context.selection;a->initialization.mapped_course=a->driver.parent.mapped_course;
    a->initialization.cursor.velocity=a->driver.parent.global_word;a->initialization.cursor.flags=a->driver.parent.global_flags;
    a->initialization.cursor.tag=a->display.object_tags[0];a->initialization.cursor.id=a->display.object_ids[0];
    return result;
}
int ntm_course_application_scene(const ntm_course_application *a,nr_scene *s)
{
    unsigned i;if(!a || !s)return -1;
    s->tiles=a->tiles;s->maps=a->maps;s->sprites=a->sprites;s->sprite_pal=a->sprite_palettes;
    for(i=0;i<2;i++){s->scroll_x[i]=a->scroll_x[i];s->scroll_y[i]=a->scroll_y[i];}
    s->sprite_x=a->initialization.sprite_x;s->sprite_y=a->initialization.sprite_y;s->swap=a->initialization.plane_swap;
    s->window_x=a->driver.parent.viewport[0];s->window_y=a->driver.parent.viewport[1];
    s->window_w=a->driver.parent.viewport[2];s->window_h=a->driver.parent.viewport[3];
    s->background=a->palette.background;s->outside=a->palette.window_control&7;return 0;
}
