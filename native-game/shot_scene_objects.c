/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_scene_objects.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static uint16_t shift3(uint16_t v){return (uint16_t)((v>>3)|((v&32768u)?57344u:0));}
static uint16_t magnitude(uint16_t x,uint16_t y)
{uint16_t d=(uint16_t)(x-y);return d&32768u?(uint16_t)(0u-d):d;}
static int valid(ntm_shot_scene_object_context *c,const ntm_shot_scene_object_resources *r)
{
    return c && c->shared && c->shared->intro && c->shared->intro->display &&
        c->shared->intro->screen_words && c->shared->objects && c->shared->x &&
        c->hole && c->player && c->player->active_player<2 && c->player->other_player<2 &&
        c->aim && r && r->metadata && r->metadata_size>=55;
}
static int add(ntm_shot_scene_object_context *c,const ntm_shot_scene_object_resources *r,
    unsigned slot,unsigned tag,unsigned tile)
{
    ntm_course_cursor *o=c->shared->objects+slot;const uint8_t *m=r->metadata+5*(tag-1);
    o->tag=(uint16_t)tag;c->shared->intro->display->object_tags[slot]=(uint16_t)tag;
    o->width=word(m);o->velocity=word(m+2);o->byte15=m[4];
    c->shared->x[slot]=o->y=0;o->second=o->byte14=0;o->tile=(uint16_t)tile;o->flags=128;
    return ntm_tee_object_register(c->shared->intro->display,o)<0?-1:0;
}
static void miniature(ntm_shot_scene_object_context *c,unsigned slot,unsigned at)
{
    const uint8_t *h=c->hole->hole;const uint16_t *s=c->shared->intro->screen_words;
    c->shared->x[slot]=(uint16_t)(shift3(word(h+at))+word(h+0x32)+s[0]);
    c->shared->objects[slot].y=(uint16_t)(shift3(word(h+at+4))+word(h+0x34)+s[1]);
}
int ntm_shot_scene_ball(ntm_shot_scene_object_context *c,const ntm_shot_scene_object_resources *r)
{
    const uint8_t *h,*other;uint16_t x,y;
    if(!valid(c,r))return -1;
    h=c->hole->hole;other=c->hole->players[c->player->other_player];
    if(add(c,r,0,1,1)<0)return -1;
    c->shared->x[0]=word(h+0x46);c->shared->objects[0].y=word(h+0x4a);
    x=word(other+6);y=word(other+10);
    if(other[1]!=255 && !other[0x22] &&
       !(magnitude(x,word(h+0x2e))<=10 && magnitude(y,word(h+0x30))<=7)) {
        if(add(c,r,17,1,206)<0)return -1;
        c->shared->x[17]=x;c->shared->objects[17].y=y;
    }
    if(add(c,r,1,2,59)<0)return -1;
    miniature(c,1,0x46);return 0;
}
int ntm_shot_scene_pin(ntm_shot_scene_object_context *c,const ntm_shot_scene_object_resources *r)
{
    if(!valid(c,r))return -1;
    if(add(c,r,5,6,5)<0)return -1;
    c->shared->x[5]=word(c->hole->hole+0x1a);c->shared->objects[5].y=word(c->hole->hole+0x1e);
    if(add(c,r,6,7,7)<0)return -1;
    miniature(c,6,0x1a);return 0;
}
int ntm_shot_scene_power_limit(ntm_hole_prepare_state *h,const ntm_shot_player_state *p,
    uint8_t club,const ntm_shot_scene_object_resources *r)
{
    unsigned index;uint8_t value;
    if(!h || !p || p->active_player>=2 || !r || !r->power_scale ||
       r->power_scale_size<256 || !r->remainders || r->remainders_size<10)return -1;
    index=(uint8_t)(h->players[p->active_player][0x23]+62u);if(club==13)index=59;
    value=r->power_scale[index^128u];h->hole[0x7e]=value;
    h->hole[0x7f]=(uint8_t)((value/10u)*8u+r->remainders[value%10u]);return 0;
}
int ntm_shot_scene_power_position(ntm_shot_scene_object_context *c)
{
    if(!c || !c->shared || !c->shared->intro || !c->shared->intro->screen_words ||
       !c->shared->objects || !c->hole)return -1;
    c->shared->objects[10].y=(uint16_t)(88u-c->hole->hole[0x7f]+c->shared->intro->screen_words[1]);return 0;
}
int ntm_shot_scene_gauges(ntm_shot_scene_object_context *c,const ntm_shot_scene_object_resources *r)
{
    unsigned j;const uint16_t *s;
    static const uint8_t tile[4]={62,8,10,12};
    if(!valid(c,r) || !r->power_scale || r->power_scale_size<256 ||
       !r->remainders || r->remainders_size<10)return -1;
    for(j=0;j<4;j++)if(add(c,r,7+j,8+j,tile[j])<0)return -1;
    if(ntm_shot_scene_power_limit(c->hole,c->player,c->aim->club,r)<0)return -1;
    s=c->shared->intro->screen_words;
    for(j=8;j<=10;j++)c->shared->x[j]=s[0];
    c->shared->objects[8].y=(uint16_t)(64u+s[1]);
    c->shared->objects[9].y=(uint16_t)(24u+s[1]);
    if(ntm_shot_scene_power_position(c)<0)return -1;
    c->shared->x[7]=(uint16_t)(8u+s[0]);
    c->shared->objects[7].y=(uint16_t)((uint8_t)(151u-c->hole->hole[0x78])+s[1]);return 0;
}
int ntm_shot_scene_save_map(ntm_shot_scene_object_context *c)
{
    unsigned row,j;
    if(!c || !c->shared || !c->shared->intro || !c->shared->intro->maps || !c->saved_map)return -1;
    for(row=0;row<4;row++)for(j=0;j<8;j++)c->saved_map[8*row+j]=c->shared->intro->maps[0x10e +64*row+j];
    return 0;
}
int ntm_shot_scene_objects_initialize(ntm_shot_scene_object_context *c,
    const ntm_shot_scene_object_resources *r,const uint8_t world[52])
{
    ntm_course_intro_context *i;
    if(!valid(c,r) || !world || !c->saved_map || !c->shared->intro->maps ||
       !r->power_scale || r->power_scale_size<256 || !r->remainders || r->remainders_size<10)return -1;
    i=c->shared->intro;
    if(ntm_tee_object_remove(i->display,c->shared->objects+4)<0)return -1;
    i->display->zero_flags[0]=0;i->display->capacity=34;
    i->screen_words[0]=word(world);i->screen_words[1]=word(world+2);
    if(ntm_shot_scene_ball(c,r)<0 || ntm_shot_scene_pin(c,r)<0 ||
       ntm_shot_scene_gauges(c,r)<0 || ntm_shot_scene_save_map(c)<0)return -1;
    return 0;
}
