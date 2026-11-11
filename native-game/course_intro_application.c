/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_intro_application.h"
static int title_tick(ntm_task_context *c,ntm_task *t)
{
    ntm_course_intro_application *a=(ntm_course_intro_application *)c;int result;
    a->title_context.control=a->selector.controls[1];
    result=ntm_course_title_tick(&a->title,t,&a->title_context,a->title_resources+a->selector.driver.parent.mapped_course);
    if(result<0)return -1;
    if(!a->tournament_prepare_update && a->title.phase==NTM_TITLE_TOURNAMENT_DELAY)a->tournament_prepare_update=a->selector.driver.updates+1;
    if(!a->tournament_start_update && a->title.phase==NTM_TITLE_TOURNAMENT_WAIT)a->tournament_start_update=a->selector.driver.updates+1;
    if(result==NTM_TITLE_COMPLETE && !a->title_complete_update)a->title_complete_update=a->selector.driver.updates+1;
    return 0;
}
static int tournament_delay(ntm_task_context *c,ntm_task *t){return title_tick(c,t);}
static int tournament_wait(ntm_task_context *c,ntm_task *t){return title_tick(c,t);}
static int title_cleanup(ntm_task_context *c,ntm_task *t){return title_tick(c,t);}
static int intro_boundary(ntm_course_intro_application *a,ntm_task *t,int request)
{
    if(request<0)return -1;
    if(request==NTM_INTRO_TITLE){
        if(a->title_resources) {
            a->title_start_update=a->selector.driver.updates+1;
            return ntm_course_title_initialize(&a->title,t,&a->title_context,
                a->title_resources+a->selector.driver.parent.mapped_course,&a->intro);
        }
        a->boundary=NTM_INTRO_BOUNDARY_TITLE;a->selector.driver.running=0;
    }
    return 0;
}
static int title_drawing_head(void *p)
{
    ntm_course_intro_application *a=p;ntm_course_application *s=&a->selector;
    if(!a->title.phase)return a->selector_drawing_head(p);
    if((s->initialization.flags&80)==80 && ntm_course_cursor_head(&s->cursor_engine,&s->display,
       s->tiles,s->sprites,s->sprite_palettes,a->title_objects+s->driver.parent.mapped_course)<0)return -1;
    s->initialization.flags&=63;return 0;
}
static int title_tail(void *p)
{
    ntm_course_intro_application *a=p;ntm_course_application *s=&a->selector;
    if(!a->title.phase)return a->selector_tail(p);
    if(ntm_course_palette_tail(&s->palette)<0)return -1;
    if((s->initialization.flags&144)==16) {
        if(ntm_course_cursor_tail(&s->cursor_engine,&a->title_cursor,&s->display,a->title_x,
           s->initialization.screen_words,&s->initialization.flags,a->title_objects+s->driver.parent.mapped_course)<0)return -1;
        s->initialization.flags|=192;
    }
    return 0;
}
static int intro_tick(ntm_task_context *c,ntm_task *t)
{
    ntm_course_intro_application *a=(ntm_course_intro_application *)c;
    int result=ntm_course_intro_tick(&a->intro,t,&a->intro_context);
    if(!a->first_visible_update && a->intro.phase==NTM_INTRO_VISIBLE_DELAY)a->first_visible_update=a->selector.driver.updates+1;
    return intro_boundary(a,t,result);
}
/* Original distinct callbacks retained even where they share native handling. */
static int intro_visible(ntm_task_context *c,ntm_task *t){return intro_tick(c,t);}
static int intro_title(ntm_task_context *c,ntm_task *t){return intro_tick(c,t);}
static int intro_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_course_intro_application *a=(ntm_course_intro_application *)c;
    unsigned course=a->selector.driver.parent.mapped_course;
    if(course>2)return -1;
    a->intro_start_update=a->selector.driver.updates+1;
    return intro_boundary(a,t,ntm_course_intro_initialize(&a->intro,t,&a->intro_context,a->courses+course));
}
static int owner_tick(ntm_task_context *c,ntm_task *t)
{
    ntm_course_intro_application *a=(ntm_course_intro_application *)c;
    int request=ntm_course_owner_tick(c,t,&a->owner,&a->records,&a->fallback);
    if(request<0)return -1;
    if(request) {
        a->boundary=request==NTM_OWNER_PRIOR_SELECTION?NTM_INTRO_BOUNDARY_PRIOR_SELECTION:
            request==NTM_OWNER_SPECIAL_RECORD?NTM_INTRO_BOUNDARY_SPECIAL_RECORD:NTM_INTRO_BOUNDARY_ROUND_RECORD;
        a->selector.driver.running=0;
    }
    return 0;
}
static int owner_wait_intro(ntm_task_context *c,ntm_task *t){return owner_tick(c,t);}
int ntm_course_intro_application_start(ntm_course_intro_application *a,
    const ntm_course_resources *selector,const ntm_course_intro_resources courses[3])
{
    ntm_course_application *s;
    if(!a || !courses || a->selector.driver.running || a->intro.phase || a->owner.phase)return -1;
    s=&a->selector;a->courses=courses;
    a->owner.intro_entry=intro_entry;a->owner.wait_intro=owner_wait_intro;
    a->intro.scenery_delay=intro_tick;a->intro.visible_delay=intro_visible;a->intro.title=intro_title;
    a->intro_context.screen=&s->screen;a->intro_context.display=&s->display;a->intro_context.palette=&s->palette;
    a->intro_context.maps=s->maps;a->intro_context.scroll_x=s->scroll_x;a->intro_context.scroll_y=s->scroll_y;
    a->intro_context.flags=&s->initialization.flags;a->intro_context.screen_words=s->initialization.screen_words;
    a->intro_context.viewport=s->driver.parent.viewport;
    s->driver.outer_callback=owner_tick;
    return ntm_course_application_start(s,selector);
}
int ntm_course_intro_application_start_titles(ntm_course_intro_application *a,
    const ntm_course_resources *selector,const ntm_course_intro_resources courses[3],
    const ntm_course_title_resources titles[3],const ntm_cursor_assets objects[3])
{
    int result;
    if(!a || !titles || !objects || a->title.phase)return -1;
    result=ntm_course_intro_application_start(a,selector,courses);if(result)return result;
    a->title_resources=titles;a->title_objects=objects;
    a->title.title_wait=title_tick;a->title.tournament_delay=tournament_delay;
    a->title.tournament_wait=tournament_wait;a->title.cleanup=title_cleanup;
    a->title_context.intro=&a->intro_context;a->title_context.cursor=&a->title_cursor;
    a->title_context.cursor_x=&a->title_x;a->title_context.tasks=&a->selector.driver.context;
    a->selector_drawing_head=a->selector.driver.drawing_head;a->selector_tail=a->selector.driver.service_tail;
    a->selector.driver.drawing_head=title_drawing_head;a->selector.driver.service_tail=title_tail;
    return 0;
}
int ntm_course_intro_application_update(ntm_course_intro_application *a,uint8_t buttons)
{
    int result;
    if(!a)return -1;
    result=ntm_course_application_update(&a->selector,buttons);
    if(a->selector.driver.parent.phase==NTM_PARENT_DONE && !a->selector_exit_update)a->selector_exit_update=a->selector.driver.updates;
    return result;
}
