/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_palette.h"
static unsigned word(const uint8_t *p){return p[0]+256u*p[1];}
static int slots_valid(const ntm_course_palette *p)
{
    unsigned i,seen=0;if(!p)return 0;
    for(i=0;i<8;i++){unsigned n=p->slots[i]&255;if(n>28 || (n&3) || (seen&(1u<<(n/4))))return 0;seen|=1u<<(n/4);}return 1;
}
static int valid(const ntm_shot_palette_job *j)
{
    if(!j || !j->blend || j->blend_size<512 || (j->offset&1))return 0;
    if(j->kind==24)return j->count && j->count<=64 && (unsigned)j->offset+8u*j->count<=512 && j->source && j->target && j->source_size>=8u*j->count && j->target_size>=8u*j->count;
    return j->kind==40 && j->offset<=510 && j->pending_index<2 && (j->control_offset==0x118 || j->control_offset==0x12);
}
int ntm_shot_palette_enqueue(ntm_course_palette *p,ntm_shot_palette *s,const ntm_shot_palette_job *j)
{
    unsigned i,k;if(!slots_valid(p) || !s || !valid(j))return -1;
    for(i=0;i<8;i++)if(p->slots[i]&32768){
        k=(p->slots[i]&255)/4;s->jobs[k]=*j;s->fraction[k]=0;s->level[k]=255;
        p->slots[i]=(uint16_t)((p->slots[i]&255)+256u*j->kind);p->status=65535;return 0;
    }return 1;
}
static uint16_t blend(unsigned source,unsigned target,unsigned level,const uint8_t *table)
{
    unsigned shift;uint8_t components[3];
    for(shift=0;shift<12;shift+=4){int a=(source>>shift)&15,b=(target>>shift)&15;unsigned v=table[16+32*level+b-a];int delta=v<128?(int)v:(int)v-256;components[shift/4]=(uint8_t)(a+delta);}
    return (uint16_t)(components[0]|(uint8_t)(components[1]<<4)|((uint16_t)components[2]<<8));
}
int ntm_shot_palette_tail(ntm_course_palette *p,ntm_shot_palette *s)
{
    uint16_t order[8];unsigned i,j,n=0,copies=0;
    if(!s || !slots_valid(p) || p->copy_bytes>32 || (p->copy_bytes&3))return -1;
    for(i=0;i<8;i++)order[i]=p->slots[i];
    if(p->status)for(i=1;i<8;i++){uint16_t v=order[i];j=i;while(j && order[j-1]>v){order[j]=order[j-1];j--;}order[j]=v;}
    for(n=0;n<8 && !(order[n]&32768);n++){
        unsigned k=(order[n]&255)/4,kind=order[n]>>8;
        if(kind==4 || kind==32){
            ntm_palette_job *v=p->jobs+k;
            if(!ntm_course_palette_job_valid(p,v) || v->kind!=(kind==4?1:2))return -1;
            copies++;
        }else{
            ntm_shot_palette_job *v=s->jobs+k;
            if(!valid(v) || v->kind!=kind || (s->level[k]!=255 && s->level[k]>15))return -1;
            if((uint8_t)(s->fraction[k]+v->speed)>=16)copies++;
        }
    }
    if(p->copy_bytes/4+copies>8)return -1;
    for(i=0;i<8;i++)p->slots[i]=order[i];
    p->status=0;
    for(i=0;i<n;i++){
        unsigned k=(order[i]&255)/4,kind=order[i]>>8,done=1,offset,words;
        if(kind==4 || kind==32){
            ntm_palette_job *v=p->jobs+k;offset=v->offset;words=kind==4?4u*v->count:1;
            if(kind==4)for(j=0;j<v->count;j++){unsigned q,a=word(v->offsets+2*j);for(q=0;q<8;q++)p->staging[offset+8*j+q]=v->colors[a+q];}
            else{uint8_t control=v->control_offset==0x118?p->background:p->window_control;p->staging[offset]=(uint8_t)v->color;p->staging[offset+1]=(uint8_t)(v->color>>8);p->pending[v->pending_index]=(uint8_t)((control&248)|((offset>>1)&7));}
        }else{
            ntm_shot_palette_job *v=s->jobs+k;unsigned sum=(uint8_t)(s->fraction[k]+v->speed),level;
            s->fraction[k]=(uint8_t)sum;if(sum<16)continue;
            s->fraction[k]=(uint8_t)(sum&15);level=(uint8_t)(s->level[k]+(sum>>4));if(level>=15)level=15;s->level[k]=(uint8_t)level;
            offset=v->offset;words=kind==24?4u*v->count:1;done=level==15;
            if(kind==24)for(j=0;j<v->count;j++){unsigned q;for(q=1;q<4;q++){unsigned at=8*j+2*q;uint16_t value=blend(word(v->source+at),word(v->target+at),level,v->blend);p->staging[offset+at]=(uint8_t)value;p->staging[offset+at+1]=(uint8_t)(value>>8);}}
            else{uint16_t value=blend(v->source_color,v->target_color,level,v->blend);uint8_t control=v->control_offset==0x118?p->background:p->window_control;p->staging[offset]=(uint8_t)value;p->staging[offset+1]=(uint8_t)(value>>8);p->pending[v->pending_index]=(uint8_t)((control&248)|((offset>>1)&7));}
        }
        p->copies[p->copy_bytes/4].offset=(uint16_t)offset;p->copies[p->copy_bytes/4].words=(uint16_t)words;p->copy_bytes+=4;
        if(done){p->slots[i]|=0xff00;p->status=65535;}
    }return 0;
}
