/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "tee_objects.h"
static unsigned word(const uint8_t *p){return p[0]+256u*p[1];}
static int signed_byte(uint8_t n){return n<128?n:(int)n-256;}
static int display_valid(const ntm_display_reset_state *d)
{
    return d && !(d->zero_flags[1]&24) && d->capacity &&
        (unsigned)d->zero_flags[0]+d->capacity<=64 && d->zero_flags[8]<=32 &&
        d->zero_words[0]==2u*d->zero_flags[8] && d->zero_flags[10]<=64 &&
        d->zero_words[1]==6u*d->zero_flags[10];
}
static int tag_valid(const ntm_course_cursor *c)
{
    if(c->tag==5)return c->width>=68 && c->width<=76 && !(c->width&3) && c->velocity<=68 && !(c->velocity&3);
    if(c->width==56) {
        switch(c->tag) {
        case 1:return c->velocity==8;
        case 2:return c->velocity==44;
        case 3:return c->velocity==40;
        case 4:return c->velocity==48;
        case 6:return c->velocity==120;
        case 7:return c->velocity==52;
        case 8:return c->velocity==56;
        case 9:return c->velocity==128;
        case 10:return c->velocity==124;
        case 11:return c->velocity==132;
        case 24:return c->velocity==60 || c->velocity==148 || c->velocity==152 || c->velocity==156;
        default:return 0;
        }
    }
    if(c->width!=52)return 0;
    if(c->tag==29)return c->velocity==0;
    if(c->tag==30)return c->velocity==12;
    if(c->tag==31)return c->velocity==4 || c->velocity==8;
    if(c->tag==32)return c->velocity>=16 && c->velocity<=28 && !(c->velocity&3);
    return 0;
}
static int shape_valid(const uint8_t *shapes,size_t shapes_size,
    const uint8_t *tiles,size_t tiles_size,unsigned offset,unsigned *count)
{
    unsigned n=0,j;
    if(!shapes || (size_t)offset+2>shapes_size)return 0;
    if(shapes[offset+1]==255){*count=0;return 1;}
    if(!tiles)return 0;
    do {
        if(n==64 || (size_t)offset+2*n+2>shapes_size)return 0;
        n++;
    }while(!(shapes[offset+2*n-2]&128));
    if((size_t)offset+4*n+4>shapes_size)return 0;
    for(j=0;j<n;j++)if(16u*(size_t)word(shapes+offset+2*n+4+2*j)+16>tiles_size)return 0;
    *count=n;return 1;
}
static int job_valid(const ntm_tee_tile_job *j)
{
    unsigned n;
    return shape_valid(j->shapes,j->shapes_size,j->tiles,j->tiles_size,j->shape_offset,&n) &&
        n && n==j->count && (unsigned)j->tile+n<=512;
}
static uint8_t *bank_data(ntm_tee_object_engine *e,ntm_display_reset_state *d,unsigned bank,unsigned plane)
{return bank?e->bank1[plane]:d->pools[plane];}
int ntm_tee_object_register(ntm_display_reset_state *d,ntm_course_cursor *c)
{
    unsigned i,n;
    if(!d || !c || d->zero_flags[8]>32 || d->zero_words[0]!=2u*d->zero_flags[8])return -1;
    n=d->zero_flags[8];if(n==32)return 1;
    if(c->id>=32 || d->object_ids[c->id]!=c->id || !tag_valid(c))return -1;
    for(i=0;i<n;i++)if((d->list[i]&255)==c->id)return -1;
    d->list[n]=(uint16_t)(c->id+256u*c->second);d->zero_words[0]+=2;
    d->zero_flags[8]++;d->zero_flags[9]|=1;d->object_tags[c->id]=c->tag;
    c->flags|=128;c->byte30=0;c->link=65535;return 0;
}
int ntm_tee_object_remove(ntm_display_reset_state *d,ntm_course_cursor *c)
{
    unsigned i,n;
    if(!d || !c || d->zero_flags[8]>32 || d->zero_words[0]!=2u*d->zero_flags[8])return -1;
    if(c->tag==65535)return 1;
    if(c->id>=32 || (c->byte30&1))return -1;
    n=d->zero_flags[8];
    for(i=0;i<n;i++)if((d->list[i]&255)==c->id)break;
    if(i==n)return 1;
    c->tag=65535;d->object_tags[c->id]=65535;d->zero_words[0]-=2;d->zero_flags[8]--;
    d->list[i]=d->list[n-1];d->list[n-1]=0;return 0;
}
int ntm_tee_objects_reset(ntm_display_reset_state *d,ntm_course_cursor c[32])
{
    unsigned i;
    if(!d || !c)return -1;
    (void)ntm_display_reset(d);
    for(i=0;i<32;i++){c[i].id=d->object_ids[i];c[i].tag=d->object_tags[i];}
    return 0;
}
typedef struct {
    uint16_t shape[2],component[2];
    uint8_t count[2],components,frame,delay,flags,status,changed;
} object_plan;
static int plan_object(const ntm_tee_object_engine *e,const ntm_course_cursor *c,
    unsigned id,uint8_t global,const ntm_tee_object_assets *a,object_plan *p)
{
    unsigned component,total=0,k,f;uint8_t old=c->flags;const ntm_cursor_animation *anim;
    if(!a || !tag_valid(c))return 0;
    anim=a->animations+c->velocity/4;
    if(!anim->data || !anim->frames || anim->frames>64 || 4u*anim->frames>anim->size)return 0;
    p->frame=e->frame[id];p->delay=e->delay[id];p->flags=old;p->status=c->byte30&127;
    p->changed=(global&32)!=0;
    if(old&128){p->flags&=127;p->status&=(uint8_t)~96u;p->changed=1;p->frame=0;p->delay=(uint8_t)(anim->data[0]+1);}
    if((p->frame&3) || p->frame/4>=anim->frames)return 0;
    p->flags|=2;
    if(!(old&64)) {
        p->flags&=(uint8_t)~2u;p->delay=(uint8_t)(p->delay-1u);
        if(!p->delay) {
            f=anim->data[p->frame+1];if(f!=0 && f!=128 && f!=192)return 0;
            p->changed=1;
            if(f==192){p->flags|=66;p->status|=64;}
            else {
                if(f&128){p->status|=32;p->frame=252;}
                p->frame=(uint8_t)(p->frame+4);if(p->frame/4>=anim->frames)return 0;
                p->delay=anim->data[p->frame];p->status|=128;
            }
        }
    }
    f=anim->data[p->frame+1];
    if(!anim->data[p->frame] || (f!=0 && f!=128 && f!=192))return 0;
    p->flags|=1;if(!(old&32))p->flags&=(uint8_t)~1u;
    component=word(anim->data+p->frame+2);p->components=0;
    for(k=0;k<2;k++,component+=4) {
        unsigned n,shape;
        if((size_t)component+4>anim->size)return 0;
        shape=word(anim->data+component);
        if((old&32) && !(shape&32768))return 0;
        if(!shape_valid(a->shapes,a->shapes_size,a->tiles,a->tiles_size,shape&32767,&n))return 0;
        /* The original empty marker skips the component offset bytes. Only
         * terminal empty shapes occur in the supported animations. */
        if(!n && !(shape&32768))return 0;
        total+=n;if((unsigned)c->tile+total>512)return 0;
        p->component[k]=(uint16_t)component;p->shape[k]=(uint16_t)(shape&32767);p->count[k]=(uint8_t)n;p->components++;
        if(shape&32768)return 1;
    }
    return 0;
}
int ntm_tee_objects_tail(ntm_tee_object_engine *e,ntm_course_cursor c[32],
    ntm_display_reset_state *d,const uint16_t x[32],const uint16_t scroll[2],
    uint8_t *global,const ntm_tee_object_assets *const a[32])
{
    object_plan plan[32];uint16_t list[32];uint32_t seen=0;unsigned n,i,j,k,bank,used=0,dirty;
    if(!e || !c || !x || !scroll || !global || !a || !display_valid(d))return -1;
    n=d->zero_flags[8];dirty=d->zero_flags[9];
    for(i=0;i<d->zero_flags[10];i++)if(!job_valid(e->jobs+i))return -1;
    for(i=0;i<n;i++) {
        unsigned id=d->list[i]&255;
        if(id>=32 || (seen&((uint32_t)1<<id)) || c[id].id!=id ||
            d->object_ids[id]!=id || d->object_tags[id]!=c[id].tag || (c[id].byte30&0x1b))return -1;
        seen|=(uint32_t)1<<id;list[i]=d->list[i];
        if(!(c[id].flags&16) && !plan_object(e,c+id,id,*global,a[id],plan+id))return -1;
    }
    /* Removal visits the swapped-in last entry before advancing. Reordering
     * is stable for equal priorities, including the order after removals. */
    for(i=0;i<n;) {
        unsigned id=list[i]&255;
        if(c[id].flags&16){c[id].tag=65535;d->object_tags[id]=65535;list[i]=list[--n];list[n]=0;dirty|=1;}
        else {if((list[i]>>8)!=c[id].second){list[i]=(uint16_t)(id+256u*c[id].second);dirty|=1;}i++;}
    }
    if(dirty)for(i=1;i<n;i++) {
        uint16_t entry=list[i];j=i;
        while(j && (entry>>8)>(list[j-1]>>8)){list[j]=list[j-1];j--;}
        list[j]=entry;
    }
    for(i=0;i<d->zero_flags[8];i++)d->list[i]=list[i];
    d->zero_flags[8]=(uint8_t)n;d->zero_words[0]=(uint16_t)(2*n);d->zero_flags[9]=0;
    bank=d->zero_flags[1]&1;e->list_dirty_high=0;
    for(i=0;i<n;i++) {
        unsigned id=list[i]&255,tile=c[id].tile;object_plan *p=plan+id;ntm_course_cursor *o=c+id;
        const ntm_tee_object_assets *res=a[id];const ntm_cursor_animation *anim=res->animations+o->velocity/4;
        uint8_t old=o->flags;
        e->frame[id]=p->frame;e->delay[id]=p->delay;o->flags=p->flags;o->byte30=p->status;
        for(j=0;j<4;j++)e->bounds[id][j]=65535;
        d->reset_words[0]=(uint32_t)(uint16_t)(tile|((o->byte15&3u)<<11))|((uint32_t)tile<<16);
        for(k=0;k<p->components;k++) {
            unsigned shape=p->shape[k],count=p->count[k];
            int dx=signed_byte(anim->data[p->component[k]+2]),dy=signed_byte(anim->data[p->component[k]+3]);
            uint16_t bx=(uint16_t)(x[id]-scroll[0]+((old&8)?-dx:dx));
            uint16_t by=(uint16_t)(o->y-scroll[1]+((old&4)?-dy:dy));
            for(j=0;j<count;j++) {
                unsigned attr,g=res->shapes[shape+2*j],xy=res->shapes[shape+2*j+1];uint8_t *dst;
                uint16_t sx=(uint16_t)(bx+((old&8)?-8*(int)(xy&15)-8:8*(int)(xy&15)));
                uint16_t sy=(uint16_t)(by+((old&4)?-8*(int)(xy>>4)-8:8*(int)(xy>>4)));
                if((old&32) || (uint16_t)(sx+7)>166 || (uint16_t)(sy+7)>158 || used>=d->capacity)continue;
                attr=(uint16_t)((d->reset_words[0]&65535u)+j)|((g&240u)<<9);attr^=(old&12u)<<12;if(o->byte14&128)attr^=0x2000;
                dst=bank_data(e,d,bank,0)+4*used;dst[0]=(uint8_t)attr;dst[1]=(uint8_t)(attr>>8);dst[2]=(uint8_t)sx;dst[3]=(uint8_t)sy;
                e->palettes[bank][0][used]=(uint8_t)((g+o->byte14)&15);used++;
            }
            if(count && p->changed && d->zero_flags[10]<64) {
                ntm_tee_tile_job *job=e->jobs+d->zero_flags[10];
                job->shapes=res->shapes;job->tiles=res->tiles;job->shapes_size=res->shapes_size;job->tiles_size=res->tiles_size;
                job->shape_offset=(uint16_t)shape;job->tile=(uint16_t)tile;job->count=(uint8_t)count;
                d->zero_flags[10]++;d->zero_words[1]+=6;
            }
            tile+=count;d->reset_words[0]=(uint32_t)(uint16_t)(tile|(o->byte15<<11))|((uint32_t)tile<<16);
        }
    }
    if(n)*global&=(uint8_t)~32u;
    d->zero_flags[13]=(uint8_t)used;d->zero_flags[14]=d->zero_flags[15]=0;
    for(i=0;i<2;i++)for(j=i?0:used;j<d->capacity;j++) {
        uint8_t *dst=bank_data(e,d,bank,i)+4*j;for(k=0;k<4;k++)dst[k]=0;e->palettes[bank][i][j]=0;
    }
    d->zero_flags[1]^=1;return 0;
}
int ntm_tee_objects_head(ntm_tee_object_engine *e,ntm_display_reset_state *d,
    uint8_t tiles[8192],uint8_t sprites[256],uint8_t palettes[64])
{
    unsigned i,j,k,bank,plane;
    if(!e || !tiles || !sprites || !palettes || !display_valid(d))return -1;
    for(i=0;i<d->zero_flags[10];i++)if(!job_valid(e->jobs+i))return -1;
    for(i=0;i<d->zero_flags[10];i++) {
        ntm_tee_tile_job *job=e->jobs+i;unsigned offset=job->shape_offset+2u*job->count+4;
        for(j=0;j<job->count;j++){unsigned source=16u*word(job->shapes+offset+2*j);for(k=0;k<16;k++)tiles[16u*(job->tile+j)+k]=job->tiles[source+k];}
    }
    d->zero_flags[10]=0;d->zero_words[1]=0;bank=(d->zero_flags[1]&1)?0:1;
    for(plane=0;plane<2;plane++)for(i=0;i<64;i++)bank_data(e,d,bank,plane)[4*i+1]^=(e->palettes[bank][plane][i]&48)>>1;
    for(i=0;i<d->capacity;i++) {
        for(j=0;j<4;j++)sprites[4*(d->zero_flags[0]+i)+j]=bank_data(e,d,bank,0)[4*i+j];
        palettes[d->zero_flags[0]+i]=e->palettes[bank][0][i]&15;
    }
    e->uploaded_bank=(uint8_t)bank;d->zero_flags[11]=1;d->zero_flags[1]=(uint8_t)((d->zero_flags[1]&~4u)^2u);return 0;
}
