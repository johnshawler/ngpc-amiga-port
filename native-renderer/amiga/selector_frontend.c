/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "selector_frontend.h"
uint8_t selector_joystick(uint16_t joy,uint8_t fire,uint16_t pot)
{
    uint8_t b=0;
    /* Standard Amiga joystick port 2 quadrature lines, active-low buttons. */
    if(joy&0x0200)b|=4;
    if(joy&0x0002)b|=8;
    if((joy^(joy>>1))&0x0100)b|=1;
    if((joy^(joy>>1))&0x0001)b|=2;
    if(!(fire&0x80))b|=16;
    if(!(pot&0x4000))b|=32;
    if((b&1) && (b&16))b|=64; /* Up+fire = modified/random accept. */
    return b;
}
unsigned selector_due(selector_clock *clock,uint32_t elapsed,uint8_t sample)
{
    unsigned n=0;clock->sample=sample;
    /* Explicit long-stall handling avoids overflow and unbounded catch-up. */
    { uint32_t seconds=elapsed/SELECTOR_CIA_HZ;
      elapsed%=SELECTOR_CIA_HZ;
      clock->dropped+=seconds*30u+seconds/8u;
      clock->phase+=(seconds%8u)*SELECTOR_CIA_HZ; }
    clock->phase+=elapsed*241u;
    while(clock->phase>=SELECTOR_PHASE_LIMIT) {
        clock->phase-=SELECTOR_PHASE_LIMIT;
        if(n<2)n++;else clock->dropped++;
    }
    clock->updates+=n;return n;
}
void selector_initialize(ntm_course_application *a)
{
    unsigned i;
    for(i=0;i<8;i++){a->screen.fill_status[i]=255;a->palette.slots[i]=(uint16_t)(0xff00+4*i);}
    a->palette.pending[0]=a->palette.pending[1]=255;
    a->palette.background=129;a->palette.window_control=7;
    a->driver.parent.viewport[0]=80;a->driver.parent.viewport[3]=152;
    a->controls[5]=7;a->controls[6]=32;a->screen.displayed_selection=1;
}
uint16_t selector_color(const uint8_t palette[512],unsigned pen,unsigned invert)
{
    unsigned v=palette[pen*2]+256u*palette[pen*2+1];
    if(invert)v^=0xfff;
    return nr_amiga_rgb12((uint16_t)v);
}
static unsigned word(const uint8_t *p){return p[0]+256u*p[1];}
static void mark_rows(uint8_t *dirty,int y)
{unsigned i;for(i=0;i<8;i++)if(y+(int)i>=0 && y+(int)i<152)dirty[y+i]=1;}
static void sprite_positions(const nr_scene *s,int16_t *x,int16_t *y)
{
    unsigned i;int lx=0,ly=0;
    for(i=0;i<64;i++){
        unsigned d=word(s->sprites+4*i);int xx=s->sprites[4*i+2],yy=s->sprites[4*i+3];
        if(d&0x400)xx+=lx;
        if(d&0x200)yy+=ly;
        lx=xx;ly=yy;
        xx=(xx+s->sprite_x)&255;if(xx>248)xx-=256;
        yy+=s->sprite_y;yy=yy>248 && yy<256?yy-256:yy&255;
        x[i]=(int16_t)xx;y[i]=(int16_t)yy;
    }
}
void selector_dirty_rows_pump(const nr_scene *old,const nr_scene *n,uint8_t dirty[152],void (*pump)(void))
{
    uint8_t tile_changed[512];int16_t ox[64],oy[64],nx[64],ny[64];unsigned i,j,p,changed;
    changed=!old || old->window_x!=n->window_x || old->window_y!=n->window_y || old->window_w!=n->window_w || old->window_h!=n->window_h || old->swap!=n->swap || old->background!=n->background || old->outside!=n->outside;
    if(!changed)for(p=0;p<2;p++)if(old->scroll_x[p]!=n->scroll_x[p] || old->scroll_y[p]!=n->scroll_y[p])changed=1;
    for(i=0;i<152;i++)dirty[i]=(uint8_t)changed;
    if(changed)return;
    for(i=0;i<512;i++){if(pump && !(i&63))pump();tile_changed[i]=0;for(j=0;j<16;j++)if(old->tiles[i*16+j]!=n->tiles[i*16+j]){tile_changed[i]=1;break;}}
    for(p=0;p<2;p++)for(i=0;i<1024;i++){
        unsigned od=word(old->maps+p*2048+i*2),nd=word(n->maps+p*2048+i*2);
        if(pump && !(i&63))pump();
        int x=(int)((i%32*8-n->scroll_x[p])&255),y=(int)((i/32*8-n->scroll_y[p])&255);
        if(x>248)x-=256;
        if(y>248)y-=256;
        if(x>=160 || x+8<=0)continue;
        if(od!=nd || tile_changed[od&511] || tile_changed[nd&511])mark_rows(dirty,y);
    }
    sprite_positions(old,ox,oy);sprite_positions(n,nx,ny);
    for(i=0;i<64;i++){
        unsigned od=word(old->sprites+i*4),nd=word(n->sprites+i*4);
        if(od!=nd || ox[i]!=nx[i] || oy[i]!=ny[i] || old->sprite_pal[i]!=n->sprite_pal[i] || tile_changed[od&511] || tile_changed[nd&511]){
            if((od&0x1800) && ox[i]<160)mark_rows(dirty,oy[i]);
            if((nd&0x1800) && nx[i]<160)mark_rows(dirty,ny[i]);
        }
    }
}

void selector_dirty_rows(const nr_scene *old,const nr_scene *next,uint8_t dirty[152])
{selector_dirty_rows_pump(old,next,dirty,0);}
