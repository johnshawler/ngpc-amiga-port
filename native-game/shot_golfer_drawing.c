/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_golfer_drawing.h"
#include "shot_markers.h"
#include "shot_math.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static uint32_t sar(uint32_t n,unsigned bits)
{return (n>>bits)|((n&0x80000000u)?(0xffffffffu<<(32-bits)):0);}
static uint32_t divide(uint32_t a,uint32_t b)
{
    unsigned negative=(a^b)>>31;uint32_t q;
    if(a&0x80000000u)a=0u-a;
    if(b&0x80000000u)b=0u-b;
    q=ntm_shot_divide(a,b);return negative?0u-q:q;
}
int ntm_shot_golfer_project_feature(uint16_t lateral,uint16_t depth,
    uint16_t *center,uint32_t *scratch)
{
    uint32_t n,scale;
    if(!center || !scratch)return -1;
    n=(uint32_t)lateral<<16;scale=divide(0x00f2db6du,sar((uint32_t)depth<<16,2)+262144u);
    *center=(uint16_t)(ntm_shot_marker_multiply(sar(n,3),scale)>>16);*scratch=n;return 0;
}
int ntm_shot_golfer_draw_initialize(ntm_shot_golfer_drawing *s,ntm_shot_golfer *golfer,
    ntm_task *task,ntm_shot_scene_object_context *c,const ntm_shot_golfer_drawing_resources *r,
    ntm_task_callback animation)
{
    ntm_shot_golfer_drawing plan;ntm_shot_golfer next;ntm_course_screen queue;
    ntm_display_reset_state display;ntm_course_cursor objects[32];uint16_t x[32];
    ntm_shot_scene_object_context local;ntm_tee_setup_context shared;ntm_course_intro_context intro;
    ntm_shot_golfer_green green;ntm_screen_stamp stamps[3];
    unsigned count=2,variant=4,i,j,row,source=0,dest=0,width=0;int clear=0,rc;size_t at;
    uint16_t choice=16,velocity=65535;uint32_t scratch;int32_t index;
    if(!s || s->phase || !golfer || golfer->phase!=1 || !task || task->callback!=golfer->drawing ||
       !animation || !c || !c->hole || !c->saved_map || !c->shared || !c->shared->intro ||
       !c->shared->objects || !c->shared->x || !c->shared->tiles || !c->shared->intro->maps ||
       !c->shared->intro->screen || !c->shared->intro->display || !c->shared->intro->screen_words || !r)return -1;
    plan=*s;next=*golfer;queue=*c->shared->intro->screen;
    rc=ntm_shot_golfer_animation_tick(&next,&queue,r->animation);if(rc)return rc;
    rc=ntm_shot_golfer_features_select(&plan.features,c->hole,&r->features);if(rc)return rc;
    scratch=c->shared->intro->display->reset_words[0];
    stamps[0].asset=stamps[1].asset=r->rim;
    stamps[0].offset=0x1c4;stamps[0].tile=0x1f5;stamps[1].offset=0x1d8;stamps[1].tile=0x81f5;
    if(plan.features.view[0]!=255) {
        if(!r->eligible || golfer->surface>=r->eligible_size)return -1;
        if(r->eligible[golfer->surface])plan.features.view[0]=255;
        else {
            clear=1;if(!r->depth_records)return -1;
            for(at=0;at+4<=r->depth_records_size;at+=4)
                if(word(plan.features.view+3)<=word(r->depth_records+at))break;
            if(at+4>r->depth_records_size)return -1;
            choice=word(r->depth_records+at+2);if(choice>16 || (choice&3))return NTM_TASK_BOUNDARY;
            variant=choice/4;
            if(variant<4) {
                static const uint16_t offsets[4]={0xa8,0xaa,0xea,0x12c};
                plan.half_width=(uint16_t)(variant==0?32:variant==3?16:24);
                velocity=(uint16_t)(variant==0?20:variant==1?16:65535);
                stamps[2].asset=r->green[variant];stamps[2].offset=offsets[variant];stamps[2].tile=0x124;count=3;
                (void)ntm_shot_golfer_project_feature(word(plan.features.view+1),word(plan.features.view+3),&plan.center_x,&scratch);
                index=((int32_t)((plan.center_x&65528u)^32768u)-32768)/2;
                if(!r->clip_records || r->clip_origin>65535 || index<-(int32_t)r->clip_origin)return NTM_TASK_BOUNDARY;
                at=(size_t)(index+(int32_t)r->clip_origin);
                if(at>r->clip_records_size || r->clip_records_size-at<4)return NTM_TASK_BOUNDARY;
                dest=word(r->clip_records+at);source=0xa8u+r->clip_records[at+2];width=r->clip_records[at+3];
                if(width && (dest<0x9000 || dest>0x9fff || (dest&1) || (source&1) ||
                   dest-0x9000u+512u+2u*width>4096 || source+512u+2u*width>4096))return -1;
                dest-=0x9000u;
            }
        }
    }
    for(i=0;i<count;i++)if(ntm_course_screen_stamp_valid(stamps+i)<0)return -1;
    intro=*c->shared->intro;display=*intro.display;intro.display=&display;
    shared=*c->shared;shared.intro=&intro;shared.objects=objects;shared.x=x;
    local=*c;local.shared=&shared;
    for(i=0;i<32;i++){objects[i]=c->shared->objects[i];x[i]=c->shared->x[i];}
    if(velocity!=65535) {
        ntm_course_cursor *o=objects+18;
        if(!r->metadata || r->metadata_size<5)return -1;
        o->tag=52;display.object_tags[18]=52;o->width=word(r->metadata);o->velocity=word(r->metadata+2);o->byte15=r->metadata[4];
        o->y=0;x[18]=0;o->second=o->byte14=0;o->tile=0x145;o->flags=128;
        if(ntm_tee_object_register(&display,o)<0)return -1;
        x[18]=(uint16_t)(((uint16_t)(plan.center_x+72u)&65528u)+intro.screen_words[0]);
        o->y=(uint16_t)(96u+intro.screen_words[1]);o->velocity=velocity;
    }
    green.center_x=plan.center_x;green.half_width=plan.half_width;green.status=plan.features.view[0];
    if(ntm_shot_golfer_pin_adjust(golfer,&local,&green)<0)return -1;
    /* All contracts validated. Preserve the original immediate drawing order. */
    for(i=0;i<2;i++)(void)ntm_course_screen_stamp(stamps+i,shared.tiles,intro.maps);
    if(clear)for(row=0;row<9;row++)for(j=0;j<16;j++)intro.maps[0xa8+64*row+j]=0;
    if(variant<4) {
        (void)ntm_course_screen_stamp(stamps+2,shared.tiles,intro.maps);
        if(width)for(row=0;row<9;row++)for(j=0;j<2*width;j+=2) {
            /* OriginalLDIRW reads a complete word before writing, in forward
             * order, even if source/destination overlap. */
            uint16_t v=word(intro.maps+source+64*row+j);
            intro.maps[dest+64*row+j]=(uint8_t)v;intro.maps[dest+64*row+j+1]=(uint8_t)(v>>8);
        }
    }
    display.reset_words[0]=scratch;*c->shared->intro->display=display;
    for(i=0;i<32;i++){c->shared->objects[i]=objects[i];c->shared->x[i]=x[i];}
    (void)ntm_shot_scene_save_map(c);*c->shared->intro->screen=queue;*golfer=next;
    plan.phase=1;*s=plan;task->callback=animation;return 0;
}
