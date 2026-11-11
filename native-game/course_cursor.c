/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_cursor.h"
static unsigned cursor_word(const uint8_t *p){return p[0]+256u*p[1];}
static int cursor_signed(uint8_t n){return n<128?n:(int)n-256;}
static uint8_t *cursor_bank(ntm_course_cursor_engine *e,ntm_display_reset_state *d,unsigned bank,unsigned plane)
{return bank?e->bank1[plane]:d->pools[plane];}
static int cursor_display_valid(const ntm_display_reset_state *d)
{return d && !(d->zero_flags[1]&24) && d->capacity && (unsigned)d->zero_flags[0]+d->capacity<=64 && d->zero_flags[8]<=1 && d->zero_flags[10]<=64 && d->zero_words[1]==6u*d->zero_flags[10];}
static int cursor_shape(const ntm_cursor_assets *a,unsigned offset,unsigned *count)
{
    unsigned n=0,j;
    if(!a || !a->shapes || !a->tiles || (size_t)offset+2>a->shapes_size || a->shapes[offset+1]==255)return 0;
    do {if(n==64 || (size_t)offset+2*n+2>a->shapes_size)return 0;n++;}while(!(a->shapes[offset+2*n-2]&128));
    if((size_t)offset+4*n+4>a->shapes_size)return 0;
    for(j=0;j<n;j++)if(16u*(size_t)cursor_word(a->shapes+offset+2*n+4+2*j)+16>a->tiles_size)return 0;
    *count=n;return 1;
}
static void cursor_clear(ntm_course_cursor_engine *e,ntm_display_reset_state *d,unsigned bank,unsigned used)
{
    unsigned plane,i,j;
    for(plane=0;plane<2;plane++)for(i=plane?0:used;i<d->capacity;i++) {
        uint8_t *p=cursor_bank(e,d,bank,plane);
        for(j=0;j<4;j++)p[4*i+j]=0;
        e->palettes[bank][plane][i]=0;
    }
}
int ntm_course_cursor_tail(ntm_course_cursor_engine *e,ntm_course_cursor *c,
    ntm_display_reset_state *d,uint16_t x,const uint16_t scroll[2],uint8_t *global,const ntm_cursor_assets *a)
{
    unsigned bank,n,i,used=0,shape,component;uint8_t frame,delay,flags,status,old;int changed,dx,dy;uint16_t bx,by;
    const ntm_cursor_animation *anim;
    if(!e || !c || !scroll || !global || !cursor_display_valid(d))return -1;
    bank=d->zero_flags[1]&1;
    if(d->zero_flags[8] && (c->byte30&0x1b))return -1;
    if(d->zero_flags[8] && !(c->flags&16)) {
        if(!a || c->id || d->list[0]!=(uint16_t)(c->id+256u*c->second))return -1;
        if(c->tag==46) {
            if(c->width!=16 || (c->velocity!=0 && c->velocity!=4))return -1;
        }else if(c->tag==38) {
            if((c->width!=20 && c->width!=24 && c->width!=28) || c->velocity)return -1;
        }else return -1;
        anim=a->animations+c->velocity/4;
        if(!anim->data || !anim->frames || 4u*anim->frames>anim->size)return -1;
        frame=e->frame;delay=e->delay;old=c->flags;flags=old;status=c->byte30&127;changed=(*global&32)!=0;
        if(old&128){flags&=127;status&=(uint8_t)~96u;changed=1;frame=0;delay=(uint8_t)(anim->data[0]+1);}
        if((frame&3) || frame/4>=anim->frames)return -1;
        flags|=2;
        if(!(old&64)) {
            flags&=(uint8_t)~2u;delay=(uint8_t)(delay-1u);
            if(!delay) {
                unsigned f=anim->data[frame+1];if(f!=0 && f!=128 && !(c->tag==38 && f==192))return -1;
                changed=1;
                /* Original200d4d stops on the current frame. It sets freeze
                 * and stopped bits, leaving delay0 and frame unchanged. The
                 * expiry still refreshes tiles, but does not set status128. */
                if(f==192){flags|=66;status|=64;}
                else {
                    if(f&128){status|=32;frame=252;}
                    frame=(uint8_t)(frame+4);if(frame/4>=anim->frames)return -1;
                    delay=anim->data[frame];status|=128;
                }
            }
        }
        if(!anim->data[frame] || (anim->data[frame+1]!=0 && anim->data[frame+1]!=128 && !(c->tag==38 && anim->data[frame+1]==192)))return -1;
        flags|=1;if(!(old&32))flags&=(uint8_t)~1u;
        component=cursor_word(anim->data+frame+2);
        if((size_t)component+4>anim->size || !(cursor_word(anim->data+component)&32768))return -1;
        shape=cursor_word(anim->data+component)&32767;
        if(!cursor_shape(a,shape,&n) || (unsigned)c->tile+n>512)return -1;
        dx=cursor_signed(anim->data[component+2]);dy=cursor_signed(anim->data[component+3]);
        bx=(uint16_t)(x-scroll[0]+((old&8)?-dx:dx));by=(uint16_t)(c->y-scroll[1]+((old&4)?-dy:dy));
        e->frame=frame;e->delay=delay;c->flags=flags;c->byte30=status;
        for(i=0;i<4;i++)e->bounds[i]=65535;
        for(i=0;i<n;i++) {
            unsigned attr,g=a->shapes[shape+2*i],xy=a->shapes[shape+2*i+1];uint8_t *p;
            uint16_t sx=(uint16_t)(bx+((old&8)?-8*(int)(xy&15)-8:8*(int)(xy&15)));
            uint16_t sy=(uint16_t)(by+((old&4)?-8*(int)(xy>>4)-8:8*(int)(xy>>4)));
            if((old&32) || (uint16_t)(sx+7)>=166 || (uint16_t)(sy+7)>=158 || used>=d->capacity)continue;
            attr=(c->tile+i)|((c->byte15&3u)<<11)|((g&240u)<<9);attr^=(old&12u)<<12;if(c->byte14&128)attr^=0x2000;
            p=cursor_bank(e,d,bank,0)+4*used;p[0]=(uint8_t)attr;p[1]=(uint8_t)(attr>>8);p[2]=(uint8_t)sx;p[3]=(uint8_t)sy;
            e->palettes[bank][0][used]=(uint8_t)((g+c->byte14)&15);used++;
        }
        if(changed && d->zero_flags[10]<64){ntm_cursor_tile_job *j=e->jobs+d->zero_flags[10];j->shape_offset=(uint16_t)shape;j->tile=c->tile;j->count=(uint8_t)n;d->zero_flags[10]++;d->zero_words[1]+=6;}
        d->reset_words[0]=(uint32_t)(uint16_t)((c->tile+n)|(c->byte15<<11))|((uint32_t)(c->tile+n)<<16);
        *global&=(uint8_t)~32u;
    }else if(d->zero_flags[8]) {
        c->tag=65535;d->object_tags[0]=65535;d->list[0]=0;d->zero_words[0]=0;d->zero_flags[8]=0;
    }
    d->zero_flags[9]=0;e->list_dirty_high=0;d->zero_flags[13]=(uint8_t)used;d->zero_flags[14]=d->zero_flags[15]=0;
    cursor_clear(e,d,bank,used);d->zero_flags[1]^=1;return 0;
}
int ntm_course_cursor_head(ntm_course_cursor_engine *e,ntm_display_reset_state *d,
    uint8_t tiles[8192],uint8_t sprites[256],uint8_t palettes[64],const ntm_cursor_assets *a)
{
    unsigned i,j,k,bank,plane;
    if(!e || !tiles || !sprites || !palettes || !cursor_display_valid(d))return -1;
    for(i=0;i<d->zero_flags[10];i++){unsigned n;ntm_cursor_tile_job *job=e->jobs+i;if(!cursor_shape(a,job->shape_offset,&n) || n!=job->count || (unsigned)job->tile+n>512)return -1;}
    for(i=0;i<d->zero_flags[10];i++) {
        ntm_cursor_tile_job *job=e->jobs+i;unsigned offset=job->shape_offset+2u*job->count+4;
        for(j=0;j<job->count;j++){unsigned source=16u*cursor_word(a->shapes+offset+2*j);for(k=0;k<16;k++)tiles[16u*(job->tile+j)+k]=a->tiles[source+k];}
    }
    d->zero_flags[10]=0;d->zero_words[1]=0;bank=(d->zero_flags[1]&1)?0:1;
    for(plane=0;plane<2;plane++)for(i=0;i<64;i++)cursor_bank(e,d,bank,plane)[4*i+1]^=(e->palettes[bank][plane][i]&48)>>1;
    for(i=0;i<d->capacity;i++) {
        for(j=0;j<4;j++)sprites[4*(d->zero_flags[0]+i)+j]=cursor_bank(e,d,bank,0)[4*i+j];
        palettes[d->zero_flags[0]+i]=e->palettes[bank][0][i]&15;
    }
    e->uploaded_bank=(uint8_t)bank;d->zero_flags[11]=1;d->zero_flags[1]=(uint8_t)((d->zero_flags[1]&~4u)^2u);return 0;
}
