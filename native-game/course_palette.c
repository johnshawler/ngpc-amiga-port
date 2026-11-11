/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_palette.h"
static unsigned palette_word(const uint8_t *p){return p[0]+256u*p[1];}
static int palette_job_valid(const ntm_palette_job *j)
{
    unsigned i;
    if(!j || (j->offset&1))return 0;
    if(j->kind==1) {
        if(!j->count || !j->offsets || !j->colors || j->offsets_size<2u*j->count || (unsigned)j->offset+8u*j->count>512)return 0;
        for(i=0;i<j->count;i++)if((size_t)palette_word(j->offsets+2*i)+8>j->colors_size)return 0;
        return 1;
    }
    return j->kind==2 && j->offset<=510 && j->pending_index<2 &&
        (j->control_offset==0x118 || j->control_offset==0x12);
}
static int palette_slots_valid(const ntm_course_palette *s)
{
    unsigned i,seen=0;
    for(i=0;i<8;i++){unsigned n=s->slots[i]&255;if(n>28 || (n&3) || (seen&(1u<<(n/4))))return 0;seen|=1u<<(n/4);}
    return 1;
}
int ntm_course_palette_job_valid(const ntm_course_palette *s,const ntm_palette_job *job)
{return s && palette_slots_valid(s) && palette_job_valid(job);}
int ntm_course_palette_enqueue(ntm_course_palette *s,const ntm_palette_job *job)
{
    unsigned i;
    if(!s || !palette_slots_valid(s) || !palette_job_valid(job))return -1;
    for(i=0;i<8;i++)if(s->slots[i]&0x8000) {
        ntm_palette_job *j=s->jobs+(s->slots[i]&255)/4;
        j->kind=job->kind;j->pending_index=job->pending_index;j->offset=job->offset;j->count=job->count;
        j->color=job->color;j->control_offset=job->control_offset;j->offsets=job->offsets;j->colors=job->colors;
        j->offsets_size=job->offsets_size;j->colors_size=job->colors_size;
        s->slots[i]=(uint16_t)((s->slots[i]&255)|(job->kind==1?0x400:0x2000));s->status=65535;return 0;
    }
    return 1;
}
int ntm_course_palette_tail(ntm_course_palette *s)
{
    unsigned i,j,queued=0;uint16_t slots[8];
    if(!s || !palette_slots_valid(s) || s->copy_bytes>32 || (s->copy_bytes&3))return -1;
    for(i=0;i<8;i++)slots[i]=s->slots[i];
    if(s->status)for(i=1;i<8;i++){uint16_t v=slots[i];j=i;while(j && slots[j-1]>v){slots[j]=slots[j-1];j--;}slots[j]=v;}
    for(i=0;i<8 && !(slots[i]&0x8000);i++) {
        ntm_palette_job *job=s->jobs+(slots[i]&255)/4;
        if(!palette_job_valid(job) || (slots[i]>>8)!=(job->kind==1?4:32))return -1;
        queued++;
    }
    if(s->copy_bytes/4+queued>8)return -1;
    for(i=0;i<8;i++)s->slots[i]=slots[i];
    s->status=0;
    for(i=0;i<queued;i++) {
        ntm_palette_job *job=s->jobs+(slots[i]&255)/4;ntm_palette_copy *copy=s->copies+s->copy_bytes/4;
        copy->offset=job->offset;copy->words=job->kind==1?(uint16_t)(4*job->count):1;s->copy_bytes+=4;
        if(job->kind==1)for(j=0;j<job->count;j++) {
            unsigned k,offset=palette_word(job->offsets+2*j);
            for(k=0;k<8;k++)s->staging[job->offset+8*j+k]=job->colors[offset+k];
        }else {
            uint8_t control=job->control_offset==0x118?s->background:s->window_control;
            s->staging[job->offset]=(uint8_t)job->color;s->staging[job->offset+1]=(uint8_t)(job->color>>8);
            s->pending[job->pending_index]=(uint8_t)((control&248)|((job->offset>>1)&7));
        }
        s->slots[i]|=0xff00;s->status=65535;
    }
    return 0;
}
int ntm_course_palette_head(ntm_course_palette *s)
{
    unsigned i,j,count;
    if(!s || s->copy_bytes>32 || (s->copy_bytes&3))return -1;
    if(!s->copy_bytes)return 0;
    count=s->copy_bytes/4;
    for(i=0;i<count;i++)if(!s->copies[i].words || (s->copies[i].offset&1) || (unsigned)s->copies[i].offset+2u*s->copies[i].words>512)return -1;
    s->copy_bytes=0;
    for(i=0;i<count;i++)for(j=0;j<2u*s->copies[i].words;j++)s->palette[s->copies[i].offset+j]=s->staging[s->copies[i].offset+j];
    if(s->pending[0]!=255){s->background=s->pending[0];s->pending[0]=255;}
    if(s->pending[1]!=255){s->window_control=s->pending[1];s->pending[1]=255;}
    return 0;
}
