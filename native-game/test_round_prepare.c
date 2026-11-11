/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "round_prepare.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned long cases;
static int expected(ntm_round_prepare_state *s,uint8_t *cursor,const uint8_t *window)
{
    unsigned i,j,n=0;uint8_t selected[18]={0},holes[5],next=*cursor;
    if(s->mapped_course>2 || s->course_order_kind)return -1;
    for(i=0;i<256 && n<5;i++) {
        unsigned value=window[(unsigned)next^128u]%18;next=(uint8_t)(next+1u);
        if(!selected[value]) {selected[value]=1;holes[n++]=(uint8_t)value;}
    }
    if(n<5)return -1;
    for(i=0;i<2;i++)for(j=0;j<64;j++) {
        if((j>=0x26 && j<=0x2a) || (j>=0x35 && j<=0x37) ||
            (s->reset_kind!=255 && (j==0x2b || j==0x30)))s->players[i][j]=0;
        else if(j>=0x10 && j<=0x21)s->players[i][j]=255;
    }
    s->round_index=s->hole_index=0;
    for(i=0;i<5;i++) {s->course_order[2*i]=s->mapped_course;s->course_order[2*i+1]=holes[i];}
    s->players[s->active_player!=0][0x34]=s->course_record_flags[s->mapped_course];
    *cursor=next;return 0;
}
static void check(ntm_round_prepare_state s,uint8_t cursor,const uint8_t *window)
{
    ntm_round_prepare_state e=s;uint8_t next=cursor;int result=expected(&e,&next,window);
    assert(ntm_round_prepare(&s,&cursor,window)==result);
    assert(!memcmp(&s,&e,sizeof(s)) && cursor==next);cases++;
    if(!result) { /* Preparation is not a one-shot initializer. */
        result=expected(&e,&next,window);assert(!result);
        assert(ntm_round_prepare(&s,&cursor,window)==result);
        assert(!memcmp(&s,&e,sizeof(s)) && cursor==next);cases++;
    }
}
int main(void)
{
    ntm_round_prepare_state s,before;uint8_t window[256],cursor;unsigned bank,reset,course,i,pattern;
    for(i=0;i<256;i++)window[i]=(uint8_t)i;
    for(bank=0;bank<256;bank++)for(reset=0;reset<256;reset++)for(course=0;course<3;course++) {
        memset(&s,(int)(bank+reset+course),sizeof(s));
        s.active_player=(uint8_t)bank;s.reset_kind=(uint8_t)reset;s.mapped_course=(uint8_t)course;s.course_order_kind=0;
        for(i=0;i<3;i++)s.course_record_flags[i]=(uint8_t)(bank+reset+37*i);
        check(s,(uint8_t)(bank+reset+course),window);
    }
    for(pattern=0;pattern<6;pattern++) {
        for(i=0;i<256;i++) {
            switch(pattern) {
            case 0:window[i]=0;break;
            case 1:window[i]=(uint8_t)(i%4);break;
            case 2:window[i]=(uint8_t)(i%5);break;
            case 3:window[i]=(uint8_t)(i/16);break;
            case 4:window[i]=(uint8_t)((i*157+19)&255);break;
            default:window[i]=(uint8_t)(i<252?i%4:4);break;
            }
        }
        for(bank=0;bank<256;bank++)for(course=0;course<3;course++) {
            memset(&s,0xa5,sizeof(s));s.active_player=(uint8_t)bank;s.reset_kind=255;
            s.mapped_course=(uint8_t)course;s.course_order_kind=0;
            check(s,(uint8_t)bank,window);
        }
    }
    for(i=0;i<256;i++)window[i]=(uint8_t)i;
    memset(&s,0x5a,sizeof(s));s.mapped_course=0;s.course_order_kind=0;before=s;cursor=254;
    assert(ntm_round_prepare(NULL,&cursor,window)==-1 && cursor==254);
    assert(ntm_round_prepare(&s,NULL,window)==-1 && !memcmp(&s,&before,sizeof(s)));
    assert(ntm_round_prepare(&s,&cursor,NULL)==-1 && cursor==254 && !memcmp(&s,&before,sizeof(s)));
    for(course=3;course<256;course++) {s=before;s.mapped_course=(uint8_t)course;check(s,254,window);}
    for(i=1;i<256;i++) {s=before;s.course_order_kind=(uint8_t)i;check(s,254,window);}
    printf("PASS native round preparation: %lu exact-state cases, repeated calls, invalid contracts\n",cases);
    return 0;
}
