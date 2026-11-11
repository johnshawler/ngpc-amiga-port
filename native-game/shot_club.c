/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_club.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static void put(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static int valid(const ntm_shot_club *s,const ntm_shot_scene_object_context *c,
    const ntm_shot_club_resources *r)
{
    return s && c && c->shared && c->shared->intro && c->shared->intro->screen &&
        c->shared->intro->display && c->hole && c->player && c->player->active_player<2 &&
        c->aim && r && r->aim && r->scene;
}
int ntm_shot_club_redraw(ntm_shot_club *s,ntm_shot_scene_object_context *c,
    const ntm_shot_club_resources *r)
{
    const uint8_t *player,*power,*lie;ntm_course_intro_context *i;ntm_course_screen *screen;
    unsigned modifier,golfer,terrain,club,label,icon,j,overflow;const ntm_shot_club_table *lie_table;uint16_t distance,strength;uint8_t digits[3];uint32_t scratch;
    if(!valid(s,c,r))return -1;
    player=c->hole->players[c->player->active_player];golfer=player[1];modifier=(uint8_t)(golfer+player[0x33]);
    terrain=c->hole->hole[0x70];club=(uint8_t)(c->aim->club+s->delta);
    if((club^128u)<(s->minimum^128u))club=s->maximum;
    if((club^128u)>(s->maximum^128u))club=s->minimum;
    if(modifier>=12 || terrain>=15 || club>=14 || !r->powers[modifier].data || r->powers[modifier].size<56)return -1;
    /*200513 leaves BC=0100 when the final digit request is dropped. Original
     *20da24 overwrites C only, so its three-byte record offset gains0300.
     * Preserve that observable queue-saturation effect with a resolved span. */
    overflow=c->shared->intro->screen->stamp_bytes>224;
    lie_table=overflow?&r->overflow_lies[modifier][terrain]:&r->lies[modifier][terrain];
    if(!lie_table->data || lie_table->size<42)return -1;
    power=r->powers[modifier].data+4*club;lie=lie_table->data+3*club;
    label=word(power+2);icon=lie[0];if(label>=23 || (!overflow && icon>=19))return -1;
    distance=(uint16_t)(((uint32_t)word(power)*40960u)>>16);
    for(j=3;j;j--){digits[j-1]=(uint8_t)(distance%10u);distance=(uint16_t)(distance/10u);}
    for(j=0;j<2 && !digits[j];j++)digits[j]=10;
    i=c->shared->intro;screen=i->screen;scratch=i->display->reset_words[0];
    c->aim->club=(uint8_t)club;s->delta=0;s->label=(uint8_t)label;
    if(ntm_course_screen_enqueue_stamp(screen,r->labels+label,0x450,0xc4)<0)return -1;
    for(j=0;j<3;j++) {
        scratch=(scratch&~((uint32_t)255<<(8*j)))|((uint32_t)digits[j]<<(8*j));
        if(ntm_course_screen_enqueue_stamp(screen,r->digits+digits[j],(uint16_t)(0x456+2*j),(uint16_t)(0xc8+2*j))<0)return -1;
    }
    i->display->reset_words[0]=scratch;
    c->hole->hole[0x7b]=(uint8_t)icon;strength=word(lie+1);
    if(c->hole->hole[0x70]==6 && !player[0x33])strength=(uint16_t)(strength+strength);
    put(c->hole->hole+0x79,strength);
    if(ntm_course_screen_enqueue_stamp(screen,r->lie_icons+(overflow?0:icon),0x380,0x22da)<0 ||
       ntm_shot_scene_power_limit(c->hole,c->player,c->aim->club,r->scene)<0 ||
       ntm_shot_scene_power_position(c)<0)return -1;
    if(s->restore_delay && ntm_course_screen_enqueue_stamp(screen,r->highlights+label,
        0x10e,player[0x33]?0x3ae4:0x20e4)<0)return -1;
    return 0;
}
int ntm_shot_club_initialize(ntm_shot_club *s,ntm_task *t,
    ntm_shot_scene_object_context *c,const ntm_shot_club_resources *r)
{
    const uint8_t *p;unsigned golfer,index;int result;
    if(!valid(s,c,r) || !t || !s->wait || s->phase)return -1;
    p=c->hole->players[c->player->active_player];golfer=p[1];index=(uint8_t)((uint8_t)(c->hole->hole[0x70]+p[3])^128u);
    if(golfer>=6 || !r->aim->minimums[golfer] || index>=r->aim->minimum_sizes[golfer])return -1;
    s->delta=s->restore_delay=0;c->hole->hole[0x77]=0;s->maximum=12;
    if(!ntm_shot_aim_at_tee(c->hole->hole) && c->hole->hole[0x70]!=11 && c->hole->hole[0x70]!=6 && c->hole->hole[0x70]!=2)s->maximum=13;
    s->minimum=r->aim->minimums[golfer][index];
    result=ntm_shot_club_redraw(s,c,r);if(result)return result;
    t->callback=s->wait;s->phase=1;return 0;
}
int ntm_shot_club_tick(ntm_shot_club *s,ntm_shot_scene_object_context *c)
{
    if(!s || !c || !c->aim || !c->hole || s->phase!=1)return -1;
    if((c->aim->reset_d1^128u)>=130u)return 0;
    if(c->aim->reset_4416 || s->restore_delay || s->delta)return 2;
    return 0;
}

int ntm_shot_club_control(ntm_shot_club *s,ntm_shot_scene_object_context *c,
    const ntm_shot_club_resources *r,uint8_t held,uint8_t pressed)
{
    unsigned edge,row,j;ntm_tee_setup_context *shared;
    if(!valid(s,c,r) || s->phase!=1)return -1;
    /*20ad20 RET GE is signed, unlike a host unsigned byte comparison. */
    if((c->aim->reset_d1^128u)>=130u)return 0;
    edge=pressed&3u;
    if(!c->aim->reset_4416 && !c->hole->hole[0x77] && !(held&12u) &&
       (edge==1 || edge==2)) {
        s->delta=(uint8_t)(edge==1?255:1);s->restore_delay=32;s->change_flag=255;
        shared=c->shared;
        if(shared->sound_request)shared->sound_request(shared->sound_context,2,0,0);
    }
    if(s->restore_delay) {
        if(s->restore_delay==1 && (!c->saved_map || !c->shared->intro->maps))return -1;
        s->restore_delay--;
        if(!s->restore_delay)for(row=0;row<4;row++)for(j=0;j<8;j++)
            c->shared->intro->maps[0x10e +64*row+j]=c->saved_map[8*row+j];
    }
    return s->delta?ntm_shot_club_redraw(s,c,r):0;
}
