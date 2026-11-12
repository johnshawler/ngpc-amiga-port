/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_trajectory_swing.h"
static int valid(const ntm_shot_trajectory_swing *s,const ntm_shot_trajectory *p,
    ntm_task_context *tasks,ntm_task *task,const ntm_shot_scene_object_context *c)
{
    return s && p && p->phase==1 && tasks && task && task->owner && task->next && task->previous &&
        c && c->shared && c->shared->objects && c->shared->intro && c->shared->intro->screen && c->aim && c->hole;
}
static int advance(ntm_shot_trajectory_swing *s,ntm_shot_trajectory *p,
    ntm_task_context *tasks,ntm_task *task,ntm_shot_scene_object_context *c,
    const ntm_shot_trajectory_swing_resources *r,uint8_t suppress,uint8_t label,int dry)
{
    uint8_t timer=(uint8_t)(s->timer-1u),counter=p->counter,result=s->result;
    unsigned frame=0,g=0,category=0;int draw=0,sound=0;uint16_t sound_command=0;
    if(!timer){
        if(counter>=15 && counter<128)result=255;
        else {
            if(counter>=128)return NTM_TASK_BOUNDARY;
            if(!r)return -1;
            frame=counter;
            if(c->aim->club==13){
                if(!r->putter_frames || counter>=r->putter_frames_size)return -1;
                frame=r->putter_frames[counter];
            }
            counter=(uint8_t)(counter+1u);timer=p->last_label;
            if(p->counter==(c->aim->club==13?10:9) && !c->aim->reset_d2){
                unsigned terrain=c->hole->hole[0x70],index;
                if(!r->terrain_sounds || terrain>=r->terrain_sounds_size)return -1;
                index=r->terrain_sounds[terrain];
                if(index==255){if(!r->club_sounds || label>=r->club_sounds_size)return -1;index=r->club_sounds[label];}
                result=1;sound=1;sound_command=(uint16_t)(0x100u|index);
            }else if(p->counter==(c->aim->club==13?14:10)){
                sound=1;sound_command=0x104;counter=(uint8_t)(counter+1u);frame++;
            }
            if(frame>=15 || p->hud_delta%60 || p->hud_delta>120)return NTM_TASK_BOUNDARY;
            category=p->hud_delta/60;
            if(!r->hud_bases || r->hud_bases_size<6)return -1;
            for(g=0;g<6;g++)if(p->hud_base==4u*r->hud_bases[g])break;
            if(g==6)return NTM_TASK_BOUNDARY;
            draw=1;
        }
    }
    if(dry)return 0;
    s->timer=timer;s->result=result;p->counter=counter;
    if(sound && c->shared->sound_request){
        if(!suppress)c->shared->sound_request(c->shared->sound_context,0,65533,0);
        c->shared->sound_request(c->shared->sound_context,2,sound_command,0);
    }
    if(draw)(void)ntm_course_screen_enqueue_stamp(c->shared->intro->screen,&r->hud[g][category][frame],0x2ca,0x2be0);
    if(s->result){task->owner->result=s->result;return ntm_task_finish(tasks,task);}
    return 0;
}
int ntm_shot_trajectory_swing_tick(ntm_shot_trajectory_swing *s,ntm_shot_trajectory *p,
    ntm_task_context *tasks,ntm_task *task,ntm_shot_scene_object_context *c,
    const ntm_shot_trajectory_swing_resources *r,uint8_t suppress,uint8_t label)
{
    if(!valid(s,p,tasks,task,c) || s->phase!=1 || task->callback!=s->animate)return -1;
    return advance(s,p,tasks,task,c,r,suppress,label,0);
}
int ntm_shot_trajectory_swing_enter(ntm_shot_trajectory_swing *s,ntm_shot_trajectory *p,
    ntm_task_context *tasks,ntm_task *task,ntm_shot_scene_object_context *c,
    const ntm_shot_trajectory_swing_resources *r,uint8_t suppress,uint8_t label)
{
    ntm_shot_trajectory_swing next;ntm_shot_trajectory trajectory;ntm_course_cursor *o;int out;
    uint8_t power;
    if(!valid(s,p,tasks,task,c) || s->phase || !s->animate)return -1;
    next=*s;trajectory=*p;power=c->hole->hole[0x7d];
    trajectory.last_label=(uint8_t)(power>=60 && power<128?3:4);
    next.timer=1;next.result=0;next.phase=1;
    /* Check the next frame/resource contract before entry mutations. */
    out=advance(&next,&trajectory,tasks,task,c,r,suppress,label,1);
    if(out)return out;
    o=c->shared->objects+16;o->velocity=(uint16_t)(o->velocity+(trajectory.last_label==4?4:0));
    o->flags=(uint8_t)((o->flags&191u)|128u);task->callback=s->animate;
    *s=next;*p=trajectory;
    out=advance(s,p,tasks,task,c,r,suppress,label,0);return out;
}
