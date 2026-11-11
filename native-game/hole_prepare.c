/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "hole_prepare.h"
#include "random_byte.h"
static unsigned word(const uint8_t *p){return p[0]+256u*p[1];}
static uint32_t lng(const uint8_t *p){return (uint32_t)word(p)|((uint32_t)word(p+2)<<16);}
static void putw(uint8_t *p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void putl(uint8_t *p,uint32_t v){putw(p,v);putw(p+2,v>>16);}
static uint32_t scaled(uint32_t n)
{
    uint32_t m=(n&0x80000000u)?0u-n:n;
    uint32_t v=(m>>16)*40960u+(((m&65535u)*40960u)>>16);
    /* Original uses a short multiply below one fixed-point unit. That
     * path truncates toward zero; the full signed product rounds downward. */
    if((n&0x80000000u) && m>=65536u && (m&7u))v++;
    return (n&0x80000000u)?0u-v:v;
}
static uint32_t direction(const uint8_t *table,unsigned angle)
{
    unsigned negative=0;uint32_t v;
    angle&=4095u;
    if(angle>2048)angle=(0u-angle)&2047u;
    if(angle>1024){negative=1;angle=(0u-angle)&1023u;}
    v=lng(table+4u*(1024u-angle));
    return negative?v:0u-v;
}
int ntm_hole_prepare(ntm_hole_prepare_state *s,const ntm_hole_prepare_resources *r,
    uint8_t *random_cursor,const uint8_t random_window[256])
{
    unsigned course,hole,index,j,bank,x,y,draw,angle,value;size_t h;
    const ntm_hole_geometry *g;const uint8_t *d;
    if(!s || !r || !random_cursor || !random_window || s->hole[6]>=18 ||
       s->difficulty>7 || s->selected_difficulty>7 || s->alternate || s->secondary_mode)return -1;
    course=s->order[2u*s->hole[6]];hole=s->order[2u*s->hole[6]+1];index=18u*course+hole;
    if(course>=3 || hole>=18 || !r->holes || index>=r->hole_count ||
       !r->hazards || r->hazards_size<2 || !r->quarter || r->quarter_size<4100 ||
       !r->variation || r->variation_size<22 || !r->threshold || r->threshold_size<8 ||
       !r->wind_low || r->wind_low_size<16 || !r->wind_high || r->wind_high_size<16 ||
       !r->wind_limit || r->wind_limit_size<18)return -1;
    g=r->holes+index;d=g->descriptor;
    if(!g->pins || g->pins_size<64)return -1;
    for(h=0;h+2<=r->hazards_size && word(r->hazards+h)!=65535;h+=10)
        if(h+10>r->hazards_size)return -1;
    if(h+2>r->hazards_size)return -1;
    s->hole[4]=(uint8_t)course;s->hole[5]=(uint8_t)hole;
    putl(s->hole+0x10,g->descriptor_identity);s->hole[0x14]=d[0];s->hole[0x15]=d[1];putw(s->hole+0x16,word(d+2));
    x=word(d+4);y=word(d+6);
    putw(s->hole+0x2e,x);putw(s->hole+0x30,y);
    for(bank=0;bank<2;bank++){putl(s->players[bank]+12,0);putl(s->players[bank]+4,(uint32_t)x<<16);putl(s->players[bank]+8,(uint32_t)y<<16);}
    putw(s->hole+0x32,d[8]);putw(s->hole+0x34,d[9]);putw(s->hole+0x3c,word(d+10));putw(s->hole+0x3e,word(d+12));
    draw=(unsigned)ntm_random_next(random_cursor,random_window)&15u;
    putw(s->hole+0x1a,word(g->pins+4*draw)+word(d+10));putw(s->hole+0x1e,word(g->pins+4*draw+2)+word(d+12));
    putl(s->hole+0x20,scaled(lng(s->hole+0x18)));putl(s->hole+0x24,scaled(lng(s->hole+0x1c)));
    putl(s->hole+0x36,g->path_identity);putw(s->hole+0x28,g->map_index);putw(s->hole+0x3a,g->aux_index);
    putw(s->hole+0x2a,32u*g->dimensions[0]-112u);putw(s->hole+0x2c,32u*g->dimensions[1]-152u);
    putl(s->hole+0x40,g->aux_identity);s->ready_flag=255;
    draw=(unsigned)ntm_random_next(random_cursor,random_window)&7u;
    draw+=((unsigned)ntm_random_next(random_cursor,random_window)&7u)+s->selected_difficulty;
    s->auxiliary[15]=s->hole[6]>=r->threshold[s->difficulty]?r->variation[draw]:0;
    angle=(unsigned)ntm_random_next(random_cursor,random_window)&15u;s->auxiliary[16]=(uint8_t)angle;
    putl(s->hole+0xec,direction(r->quarter,256u*angle+1024u));putl(s->hole+0xf0,direction(r->quarter,256u*angle));
    putw(s->auxiliary+7,65535);putw(s->auxiliary+9,65535);s->auxiliary[11]=0;putw(s->auxiliary+12,32767);
    for(j=0;j<h;j+=10)if(word(r->hazards+j)==256u*course+hole){
        putw(s->auxiliary+7,word(r->hazards+j+2));putw(s->auxiliary+9,word(r->hazards+j+4));
        putw(s->auxiliary+11,word(r->hazards+j+6));putw(s->auxiliary+12,word(r->hazards+j+8));break;
    }
    for(bank=0;bank<2;bank++)s->players[bank][2]=s->players[bank][42]=s->players[bank][34]=s->players[bank][37]=s->players[bank][3]=0;
    s->selected_difficulty=s->difficulty;
    s->hole[0x6f]=(uint8_t)((unsigned)ntm_random_next(random_cursor,random_window)&15u);
    draw=(unsigned)ntm_random_next(random_cursor,random_window)&15u;
    value=(uint8_t)(s->difficulty+(s->difficulty<=3?r->wind_low[draw]:r->wind_high[draw]));
    if(value&128)value=0;
    if(value>r->wind_limit[s->hole[6]])value=r->wind_limit[s->hole[6]];
    if(value>15)value=15;
    s->hole[0x6e]=(uint8_t)value;
    return 0;
}
