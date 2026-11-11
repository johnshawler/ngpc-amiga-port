/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_camera.h"
#include "task_allocation.h"
#include <string.h>
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]+256u*p[1]);}
static uint32_t lng(const uint8_t *p){return (uint32_t)word(p)|((uint32_t)word(p+2)<<16);}
static void putw(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void putl(uint8_t *p,uint32_t v){putw(p,(uint16_t)v);putw(p+2,(uint16_t)(v>>16));}
static int gt(uint16_t a,uint16_t b){return (a^32768u)>(b^32768u);}
static int signed_word(uint16_t n){return n<32768u?(int)n:(int)n-65536;}
static uint16_t asr(uint16_t n,unsigned bits)
{return (uint16_t)((n>>bits)|((n&32768u)?(65535u<<(16-bits)):0));}
static int valid(const ntm_shot_marker_context *c,const ntm_shot_marker_resources *r)
{return c && c->shared && c->shared->intro && c->shared->intro->screen_words &&
    c->shared->intro->viewport && c->shared->objects && c->shared->x && c->hole &&
    c->aim && c->dirty && r && r->quarter && r->quarter_size>=4100;}
/*20ca34: unlike20b03b,20b1de deliberately updates only the miniature marker. */
static void positions(ntm_shot_marker_context *c,const ntm_shot_marker_resources *r)
{
    ntm_tee_setup_context *q=c->shared;uint8_t *h=c->hole->hole;
    uint16_t *s=q->intro->screen_words,length=word(h+0x6a),x;uint32_t dx,dy,distance;unsigned i;
    q->x[6]=(uint16_t)(asr(word(h+0x1a),3)+word(h+0x32)+s[0]);
    q->objects[6].y=(uint16_t)(asr(word(h+0x1e),3)+word(h+0x34)+s[1]);
    q->x[1]=(uint16_t)(asr(word(h+0x46),3)+word(h+0x32)+s[0]);
    q->objects[1].y=(uint16_t)(asr(word(h+0x4a),3)+word(h+0x34)+s[1]);
    for(i=8;i<=10;i++)q->x[i]=s[0];
    q->objects[8].y=(uint16_t)(64u+s[1]);q->objects[9].y=(uint16_t)(24u+s[1]);
    q->objects[10].y=(uint16_t)(88u-h[0x7f]+s[1]);
    q->x[7]=(uint16_t)(8u+s[0]);q->objects[7].y=(uint16_t)((uint8_t)(151u-h[0x78])+s[1]);
    *c->dirty=0;dx=ntm_shot_marker_direction(r->quarter,(uint16_t)(c->aim->angle+1024u));
    dy=ntm_shot_marker_direction(r->quarter,c->aim->angle);putl(h+0xd4,dx);putl(h+0xd8,dy);
    distance=gt(length,160)?30u<<16:(uint32_t)(length>>3)<<16;
    q->x[3]=(uint16_t)(q->x[1]+(ntm_shot_marker_multiply(dx,distance)>>16));
    q->objects[3].y=(uint16_t)(q->objects[1].y+(ntm_shot_marker_multiply(dy,distance)>>16));
    q->objects[3].flags&=(uint8_t)~32u;x=(uint16_t)(q->x[3]-s[0]);
    if(!gt(x,128))q->objects[3].flags|=32;
}
int ntm_shot_camera_active(ntm_shot_camera_state *s,ntm_task *t,ntm_task_context *tasks,
    ntm_shot_marker_context *c,const ntm_shot_marker_resources *r,uint8_t pressed)
{
    static const int8_t dx[16]={0,0,0,0,-8,-8,-8,0,8,8,8,0,0,0,0,0};
    static const int8_t dy[16]={0,-8,8,0,0,-8,8,0,0,-8,8,0,0,0,0,0};
    unsigned buttons;uint16_t x,y;uint8_t *v;ntm_shot_aim *a;
    if(!s || !t || !tasks || !c || !c->aim)return -1;
    a=c->aim;
    if(a->reset_d1){
        if(ntm_task_remove_descendants(tasks,t)<0 || ntm_task_finish(tasks,t)<0)return -1;
        return 0;
    }
    if(a->reset_4416!=0 && a->reset_4416!=2 && a->reset_4416!=4)return 2;
    if(!valid(c,r))return -1;
    v=c->shared->intro->viewport;
    if(a->reset_4416==4){v[0]=v[1]=0;v[2]=160;v[3]=152;a->reset_4416=0;return 0;}
    if(!a->reset_4416){
        if(!(pressed&32u))return 0;
        a->reset_4416=2;s->saved_x=a->origin_x;s->saved_y=a->origin_y;
    }
    if(!(c->held&32u)){
        a->reset_4416=4;
        if(a->origin_x==s->saved_x && a->origin_y==s->saved_y)return 0;
        a->origin_x=s->saved_x;a->origin_y=s->saved_y;
        v[0]=v[1]=v[2]=0;v[3]=152;
    }else{
        buttons=c->held&15u;
        if(dx[buttons] || dy[buttons]){
            /*204e94 receives wrapped x+76 and subtracts60 before clamp; y+80
             * subtracts80. The final x subtract16 retains negative origin-16. */
            x=(uint16_t)(a->origin_x+dx[buttons]+16u);y=(uint16_t)(a->origin_y+dy[buttons]);
            if(x&32768u)x=0;else if(gt(x,word(c->hole->hole+0x2a)))x=word(c->hole->hole+0x2a);
            if(y&32768u)y=0;else if(gt(y,word(c->hole->hole+0x2c)))y=word(c->hole->hole+0x2c);
            a->origin_x=(uint16_t)(x-16u);a->origin_y=y;
        }
    }
    c->shared->intro->screen_words[0]=a->origin_x;c->shared->intro->screen_words[1]=a->origin_y;
    positions(c,r);return 0;
}
static const uint8_t *resolve(const ntm_aim_world_span *s,int offset,size_t n)
{
    size_t at;if(!s || !s->data || s->origin>s->size)return NULL;
    if(offset<0){size_t back=(size_t)(-offset);if(back>s->origin)return NULL;at=s->origin-back;}
    else{if((size_t)offset>s->size-s->origin)return NULL;at=s->origin+(size_t)offset;}
    return n>s->size-at?NULL:s->data+at;
}
static int block(const ntm_aim_world_resources *r,uint16_t x,uint16_t y,uint8_t *maps,unsigned base)
{
    uint16_t tx=asr(x,3),ty=asr(y,3),index,b;unsigned dest,k;const uint8_t *p;
    dest=((ty&31u)*32u+(tx&31u))*2u;
    index=(uint16_t)(asr(ty,2)*r->width+asr(tx,2));
    p=resolve(&r->layout,signed_word((uint16_t)(2u*index)),2);if(!p)return -1;
    b=(uint16_t)(word(p)<<5);p=resolve(&r->blocks,signed_word(b),32);
    if(!p || dest+200u>2048u)return -1;
    if(maps)for(k=0;k<4;k++)memcpy(maps+base+dest+64*k,p+8*k,8);
    return 0;
}
static int strip(ntm_aim_world_state *s,const ntm_aim_world_resources *r,uint8_t *maps,unsigned base,int direction)
{
    uint16_t x=(uint16_t)(word(s->bytes+4)-32u),y=(uint16_t)(word(s->bytes+6)-32u);unsigned i;
    if(direction==1)x=(uint16_t)(word(s->bytes+4)+160u);
    if(direction==3)y=(uint16_t)(word(s->bytes+6)+160u);
    for(i=0;i<7;i++){
        putw(s->bytes+40,x);putw(s->bytes+42,y);
        if(block(r,x,y,maps,base)<0)return -1;
        if(direction<2)y=(uint16_t)(y+32u);else x=(uint16_t)(x+32u);
    }
    putw(s->bytes+40,x);putw(s->bytes+42,y);return 0;
}
static int scroll_run(ntm_aim_world_state *s,const ntm_aim_world_resources *r,uint8_t *maps,unsigned base)
{
    unsigned i;uint16_t d;
    /* Each looping iteration moves the vertical base32 toward its wrapped
     * origin;1025 covers the signed16 envelope plus the final nonmoving pass. */
    for(i=0;i<1025;i++){
        d=(uint16_t)(word(s->bytes)-word(s->bytes+4));
        if(!((uint16_t)(d-32u)&32768u)){
            putw(s->bytes+4,(uint16_t)(word(s->bytes+4)+32u));if(strip(s,r,maps,base,1)<0)return -1;
        }else if((uint16_t)(d+32u)&32768u){
            putw(s->bytes+4,(uint16_t)(word(s->bytes+4)-32u));if(strip(s,r,maps,base,0)<0)return -1;
        }
        d=(uint16_t)(word(s->bytes+2)-word(s->bytes+6));
        if(!((uint16_t)(d-32u)&32768u)){
            putw(s->bytes+6,(uint16_t)(word(s->bytes+6)+32u));if(strip(s,r,maps,base,3)<0)return -1;
        }else if((uint16_t)(d+32u)&32768u){
            putw(s->bytes+6,(uint16_t)(word(s->bytes+6)-32u));if(strip(s,r,maps,base,2)<0)return -1;
        }else return 0;
    }
    return -1;
}
int ntm_shot_camera_scroll(ntm_aim_world_state *s,const ntm_aim_world_resources *r,uint8_t maps[4096],uint8_t scroll[2])
{
    ntm_aim_world_state check;unsigned base;
    if(!s || !r || !maps || !scroll)return -1;
    base=word(s->bytes+44);if(base!=0x9000u && base!=0x9800u)return -1;base-=0x9000u;
    if(lng(s->bytes+8)!=r->layout_tag || lng(s->bytes+12)!=r->blocks_tag || s->bytes[16]!=r->width)return -1;
    check=*s;if(scroll_run(&check,r,NULL,base)<0)return -1;
    scroll[0]=s->bytes[0];scroll[1]=s->bytes[2];return scroll_run(s,r,maps,base);
}
