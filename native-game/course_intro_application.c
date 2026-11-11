/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_intro_application.h"
static int round_tick(ntm_task_context *c,ntm_task *t)
{
    ntm_course_intro_application *a=(ntm_course_intro_application *)c;int result;
    a->round_context.control=a->selector.controls[1];
    result=ntm_round_record_tick(&a->round,t,&a->round_context,a->round_resources);
    if(result<0)return -1;
    if(!a->round_open_update && a->round.phase==NTM_ROUND_WAIT)a->round_open_update=a->selector.driver.updates+1;
    if(result==NTM_ROUND_COMPLETE && !a->round_complete_update)a->round_complete_update=a->selector.driver.updates+1;
    return 0;
}
static int round_open(ntm_task_context *c,ntm_task *t){return round_tick(c,t);}
static int round_wait(ntm_task_context *c,ntm_task *t){return round_tick(c,t);}
static int round_cleanup(ntm_task_context *c,ntm_task *t){return round_tick(c,t);}
static int round_draw_finish(ntm_task_context *c,ntm_task *t)
{ntm_course_intro_application *a=(ntm_course_intro_application *)c;return ntm_round_record_drawing_finish(&a->round_drawing,t,&a->round_context,a->round_resources)<0?-1:0;}
static int round_draw_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_course_intro_application *a=(ntm_course_intro_application *)c;
    a->round_drawing_update=a->selector.driver.updates+1;
    return ntm_round_record_drawing_initialize(&a->round_drawing,t,&a->round_context,a->round_resources);
}
static int round_blink_tick(ntm_task_context *c,ntm_task *t)
{ntm_course_intro_application *a=(ntm_course_intro_application *)c;return ntm_round_record_blink_tick(&a->round_blink,t,&a->round_context,a->round_resources);}
static int round_blink_shown(ntm_task_context *c,ntm_task *t){return round_blink_tick(c,t);}
static int round_blink_hidden(ntm_task_context *c,ntm_task *t){return round_blink_tick(c,t);}
static int round_blink_entry(ntm_task_context *c,ntm_task *t)
{ntm_course_intro_application *a=(ntm_course_intro_application *)c;return ntm_round_record_blink_initialize(&a->round_blink,t,&a->round_context,a->round_resources);}
static int round_prepare(void *context)
{
    ntm_course_intro_application *a=context;
    int result=ntm_round_prepare(&a->round_prepare,a->selector.resources->random_cursor,a->selector.resources->random);
    if(!result){a->round_prepare.players[0][0x24]=1;a->round_prepare.players[1][0x24]=0;}
    return result;
}
static int round_entry(ntm_task_context *c,ntm_task *t)
{
    ntm_course_intro_application *a=(ntm_course_intro_application *)c;
    ntm_round_prepare_state *p=&a->round_prepare;
    p->mapped_course=a->selector.driver.parent.mapped_course;p->active_player=a->records.active_player;
    p->players[0][1]=a->records.golfer[0];p->players[1][1]=a->records.golfer[1];
    p->players[0][0x33]=a->records.record_kind[0];p->players[1][0x33]=a->records.record_kind[1];
    a->round_context.course=p->mapped_course;a->round_context.golfer=p->players[0][1];
    a->round_context.record_kind=p->players[0][0x33];a->round_context.score=p->active_player?p->players[0][0x34]:p->course_record_flags[p->mapped_course];
    a->round_cursor=a->title_cursor;a->round_x=a->title_x;
    a->round_start_update=a->selector.driver.updates+1;
    return ntm_round_record_initialize(&a->round,t,&a->round_context,a->round_resources);
}
static int owner_wait_round(ntm_task_context *c,ntm_task *t)
{
    ntm_course_intro_application *a=(ntm_course_intro_application *)c;
    if(!t->next)return -1;
    if(t->priority<t->next->priority)return 0;
    a->boundary=NTM_INTRO_BOUNDARY_TEE_SHOT;a->selector.driver.running=0;return 0;
}
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
    if(a->round.phase) {
        if((s->initialization.flags&80)==80 && ntm_course_cursor_head(&s->cursor_engine,&s->display,
           s->tiles,s->sprites,s->sprite_palettes,a->round_resources->object)<0)return -1;
        s->initialization.flags&=63;return 0;
    }
    if(!a->title.phase)return a->selector_drawing_head(p);
    if((s->initialization.flags&80)==80 && ntm_course_cursor_head(&s->cursor_engine,&s->display,
       s->tiles,s->sprites,s->sprite_palettes,a->title_objects+s->driver.parent.mapped_course)<0)return -1;
    s->initialization.flags&=63;return 0;
}
static int title_tail(void *p)
{
    ntm_course_intro_application *a=p;ntm_course_application *s=&a->selector;
    if(a->round.phase) {
        if(ntm_course_palette_tail(&s->palette)<0)return -1;
        if((s->initialization.flags&144)==16) {
            if(ntm_course_cursor_tail(&s->cursor_engine,&a->round_cursor,&s->display,a->round_x,
               s->initialization.screen_words,&s->initialization.flags,a->round_resources->object)<0)return -1;
            s->initialization.flags|=192;
        }
        return 0;
    }
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
    if(request==NTM_OWNER_ROUND_RECORD && a->round_resources) {
        /* This is the owner's real later traversal, not title-child cleanup. */
        if(a->title_context.game_mode || a->round_context.game_mode || a->title_context.input_mode || a->records.record_kind[0] ||
           a->round_context.alternate || a->round_context.record_mode || a->round_prepare.course_order_kind || a->records.golfer[0]>=6 ||
           a->selector.driver.parent.mapped_course>=3 ||
           (a->records.active_player?a->round_prepare.players[0][0x34]:
            a->round_prepare.course_record_flags[a->selector.driver.parent.mapped_course])!=255) {
            a->boundary=NTM_INTRO_BOUNDARY_ROUND_UNSUPPORTED;a->selector.driver.running=0;return 0;
        }
        (void)ntm_task_allocate(c,t,round_entry,&a->fallback);t->callback=owner_wait_round;
        a->round_owner_active=1;return 0;
    }
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
int ntm_course_intro_application_start_round_record(ntm_course_intro_application *a,
    const ntm_course_resources *selector,const ntm_course_intro_resources courses[3],
    const ntm_course_title_resources titles[3],const ntm_cursor_assets objects[3],
    const ntm_round_record_resources *r)
{
    int result;unsigned i;
    if(!a || !r || a->round.phase)return -1;
    result=ntm_course_intro_application_start_titles(a,selector,courses,titles,objects);if(result)return result;
    a->round_resources=r;
    a->round.drawing_entry=round_draw_entry;a->round.blink_entry=round_blink_entry;
    a->round.allocate_blink=round_tick;a->round.open=round_open;a->round.wait=round_wait;a->round.cleanup=round_cleanup;
    a->round_drawing.finish=round_draw_finish;
    a->round_blink.initial_delay=round_blink_tick;a->round_blink.shown_delay=round_blink_shown;a->round_blink.hidden_delay=round_blink_hidden;
    a->round_context.intro=&a->intro_context;a->round_context.tasks=&a->selector.driver.context;
    a->round_context.fallback=&a->fallback;a->round_context.cursor=&a->round_cursor;a->round_context.cursor_x=&a->round_x;
    a->round_context.prepare=round_prepare;a->round_context.prepare_context=a;
    a->round_context.tiles=a->selector.tiles;a->round_context.score=255;
    for(i=0;i<3;i++)a->round_prepare.course_record_flags[i]=255;
    a->round_prepare.players[0][0x34]=a->round_prepare.players[1][0x34]=255;
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
