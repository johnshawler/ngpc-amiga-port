/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_golfer.h"
#include "shot_markers.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static uint32_t lng(const uint8_t *p){return (uint32_t)word(p)|((uint32_t)word(p+2)<<16);}
static int gt(uint16_t a,uint16_t b){return (a^32768u)>(b^32768u);}
static const ntm_screen_asset *asset(const ntm_shot_golfer_resources *r,uint16_t selector)
{
    size_t i;for(i=0;i<r->asset_count;i++)if(r->assets[i].selector==selector)return &r->assets[i].asset;
    return 0;
}
static int choose_pin(const ntm_shot_golfer_resources *r,uint16_t distance,uint16_t *velocity)
{
    size_t i;for(i=0;i+4<=r->pin_records_size;i+=4)if(distance<=word(r->pin_records+i)){*velocity=word(r->pin_records+i+2);return 0;}
    return -1;
}
static int ground(ntm_shot_golfer *s,const ntm_hole_prepare_state *h,const ntm_shot_golfer_resources *r,
    const ntm_shot_golfer_context *c,ntm_terrain_result *query,uint16_t prior_bc,uint16_t *selector)
{
    const uint8_t *b=h->hole,*record;uint16_t kind=4,image=prior_bc,angle;uint8_t code,advance;size_t i;
    s->surface=6;s->task_bytes[0]=0;
    if(b[0x70]==4 && !(word(h->auxiliary+7)&32768u)){
        kind=(uint16_t)((word(h->auxiliary+11)&65280u)|(uint8_t)(h->auxiliary[11]+8u));
    }else if(b[0x70]!=6){
        s->delta_x=ntm_shot_marker_multiply(lng(b+0xd4),24u<<16);
        s->delta_y=ntm_shot_marker_multiply(lng(b+0xd8),24u<<16);
        s->sample_x=s->delta_x+lng(b+0x44);s->sample_y=s->delta_y+lng(b+0x48);s->attempts=3;
        if(b[0x70]>=11){
            s->sample_x=ntm_shot_marker_multiply(lng(b+0xd4),28u<<16)+lng(b+0x44);
            s->sample_y=ntm_shot_marker_multiply(lng(b+0xd8),28u<<16)+lng(b+0x48);
        }
        for(;;){
            if(ntm_shot_terrain(b,c->player_status,(uint16_t)(s->sample_x>>16),(uint16_t)(s->sample_y>>16),c->terrain,query)<0)return -1;
            code=query->terrain;if((size_t)code*4+4>r->ground_records_size)return -1;
            s->surface=code;record=r->ground_records+4u*code;image=word(record);kind=record[2];advance=record[3];
            if(!advance)break;
            s->sample_x+=s->delta_x;s->sample_y+=s->delta_y;s->attempts=(uint8_t)(s->attempts-1u);
            if(!s->attempts)break;
        }
        if(code==11){if(2u*s->attempts+2u>r->overrides_size)return -1;image=word(r->overrides+2u*s->attempts);}
        if(advance || code==3){
            if(!gt(word(b+0x6a),160)){
                for(i=0;i+6<=r->green_records_size;i+=6)if(word(b+0x6a)<=word(r->green_records+i))break;
                if(i+6>r->green_records_size)return -1;
                if(ntm_shot_aim_angle(word(b+0x46),word(b+0x4a),(uint16_t)(word(b+0x3c)+28u),(uint16_t)(word(b+0x3e)+28u),r->aim,&angle)<0)return -1;
                angle=(uint16_t)((c->scene->aim->angle+4096u-angle)&4095u);angle=(uint16_t)(angle-4096u);
                if(angle&32768u)angle=(uint16_t)(0u-angle);
                if(!gt(angle,word(r->green_records+i+4))){s->surface=3;kind=252;image=word(r->green_records+i+2);}
            }
        }
    }
    s->task_bytes[3]=(uint8_t)kind;*selector=image;
    if(!(kind&128u)){unsigned index=(uint8_t)kind>>2;if(index>=r->frames_size)return -1;s->task_bytes[1]=1;s->task_bytes[2]=r->frames[index];}
    return 0;
}
int ntm_shot_golfer_initialize(ntm_shot_golfer *s,ntm_task *t,ntm_shot_golfer_context *c,const ntm_shot_golfer_resources *r)
{
    ntm_shot_golfer plan;ntm_shot_scene_object_context *scene;ntm_tee_setup_context *q;ntm_course_intro_context *g;
    ntm_terrain_result query;ntm_shot_projection projected;ntm_display_reset_state display;ntm_course_cursor object;
    const ntm_screen_asset *background,*middle=0,*foreground;const uint8_t *b,*record;uint16_t ground_selector,velocity;unsigned course,index;
    if(!s || !t || !c || !r || s->phase || !s->drawing || !(scene=c->scene) || !scene->hole || !scene->aim ||
       !(q=scene->shared) || !(g=q->intro) || !q->objects || !q->x || !g->screen || !g->display || !g->scroll_x ||
       !g->scroll_y || !g->screen_words || !c->terrain || !c->terrain_detail || !r->assets || !r->aim ||
       !r->course_selectors || r->course_selectors_size<3 || !r->ground_records || !r->foreground_records ||
       !r->frames || !r->overrides || !r->green_records || !r->pin_records || !r->metadata || r->metadata_size<5 ||
       g->screen->stamp_bytes>256 || (g->screen->stamp_bytes&7))return -1;
    b=scene->hole->hole;course=b[4];if(course>=3 || (size_t)b[0x70]*4u+4u>r->foreground_records_size)return -1;
    if(!ntm_course_palette_job_valid(g->palette,r->palettes[course]) || !ntm_course_palette_job_valid(g->palette,r->palettes[course]+1))return -1;
    background=asset(r,r->course_selectors[course]);if(!background)return -1;
    plan=*s;query.x=(uint16_t)g->display->reset_words[0];query.y=(uint16_t)(g->display->reset_words[0]>>16);query.detail=*c->terrain_detail;query.terrain=0;
    if(ground(&plan,scene->hole,r,c,&query,g->screen->stamp_bytes,&ground_selector)<0)return -1;
    if(plan.task_bytes[3]&128u){middle=asset(r,ground_selector);if(!middle)return -1;}
    record=r->foreground_records+4u*b[0x70];foreground=asset(r,word(record));if(!foreground)return -1;
    plan.task_bytes[4]=0;plan.task_bytes[7]=record[2];index=record[2]>>2;if(index>=r->frames_size)return -1;
    plan.task_bytes[5]=plan.task_bytes[6]=r->frames[index];
    object=q->objects[5];object.tag=51;object.width=word(r->metadata);object.velocity=word(r->metadata+2);object.byte15=r->metadata[4];
    object.y=65456;object.second=0;object.byte14=0;object.tile=206;object.flags=128;
    display=*g->display;display.object_tags[5]=51;if(ntm_tee_object_register(&display,&object)<0)return -1;
    if(choose_pin(r,word(b+0x6a),&velocity)<0)return -1;
    plan.direction_x=lng(b+0xd4);plan.direction_y=lng(b+0xd8);
    if(ntm_shot_project(lng(b+0x18),lng(b+0x1c),lng(b+0x50),lng(b+0x54),plan.direction_x,plan.direction_y,&projected)<0)return -1;
    object.y=(uint16_t)(72u+g->screen_words[1]);object.velocity=velocity;
    plan.depth_scratch=projected.depth_scratch;plan.phase=1;
    g->scroll_x[1]=g->scroll_y[1]=0;
    (void)ntm_course_palette_enqueue(g->palette,r->palettes[course]);(void)ntm_course_palette_enqueue(g->palette,r->palettes[course]+1);
    (void)ntm_course_screen_enqueue_stamp(g->screen,background,0x804,0x14c);
    if(middle)(void)ntm_course_screen_enqueue_stamp(g->screen,middle,0xa44,0x190);
    (void)ntm_course_screen_enqueue_stamp(g->screen,foreground,0xb44,0x1b6);
    *g->display=display;q->objects[5]=object;q->x[5]=(uint16_t)(projected.x+72u+g->screen_words[0]);
    g->display->reset_words[0]=projected.lateral_scratch;*c->terrain_detail=query.detail;
    *s=plan;t->callback=s->drawing;return 0;
}
