/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_aim_setup.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
int ntm_shot_aim_panorama(ntm_tee_setup_context *c,const ntm_shot_aim_setup_resources *r,
    uint8_t course,uint8_t hole)
{
    ntm_course_cursor *o;ntm_course_intro_context *i;
    if(!c || !r || !(i=c->intro) || !i->screen || !i->display || !c->objects || !c->x ||
       course>=3 || hole>=18 || !r->metadata || r->metadata_size<5)return -1;
    if(ntm_course_screen_enqueue_stamp(i->screen,&r->panorama[course][hole],0x20,0x1460)<0)return -1;
    o=c->objects+4;o->tag=5;i->display->object_tags[4]=5;
    o->width=word(r->metadata);o->velocity=word(r->metadata+2);o->byte15=r->metadata[4];
    c->x[4]=128;o->y=88;o->second=255;o->byte14=0;o->tile=156;o->flags=128;
    if(ntm_tee_object_register(i->display,o)<0)return -1;
    o->width=(uint16_t)(0x44+4u*course);o->velocity=(uint16_t)(4u*hole);return 0;
}
