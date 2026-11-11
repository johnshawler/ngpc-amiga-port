/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_choice.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int oracle(ntm_course_choice *s,unsigned held,unsigned pressed,unsigned gate)
{
    int next=s->selection;
    if((pressed&32) && gate==0){s->result=2;return 2;}
    if(pressed&16){s->result=(held&64)?3:1;return 2;}
    if((pressed&4) && !(pressed&8))next--;
    else if((pressed&8) && !(pressed&4))next++;
    else return 0;
    if(next<0)next+=256;
    if(next>255)next-=256;
    if(next>=3)return 0;
    s->selection=(uint8_t)next;return 1;
}
int main(void)
{
    unsigned selection,pressed,held,gate,i;unsigned long cases=0;
    for(selection=0;selection<256;selection++)for(pressed=0;pressed<256;pressed++)
    for(held=0;held<256;held++)for(gate=0;gate<2;gate++) {
        ntm_course_choice a={(uint8_t)selection,(uint8_t)(selection^pressed)},b=a;
        int x=ntm_course_choice_update(&a,(uint8_t)held,(uint8_t)pressed,(uint8_t)gate);
        int y=oracle(&b,held,pressed,gate);assert(x==y && a.selection==b.selection && a.result==b.result);cases++;
    }
    for(gate=0;gate<256;gate++)for(pressed=0;pressed<256;pressed++) {
        ntm_course_choice a={1,0xa5},b=a;
        assert(ntm_course_choice_update(&a,64,(uint8_t)pressed,(uint8_t)gate)==oracle(&b,64,pressed,gate));
        assert(a.selection==b.selection && a.result==b.result);cases++;
    }
    {
        uint8_t a[16]={0},b[16]={0};ntm_course_choice x={0,0},y=x;
        a[5]=b[5]=7;a[6]=b[6]=32;
        for(i=0;i<65536;i++) {
            unsigned sample=(i>>3)&255;int actual=ntm_course_choice_tick(a,&x,(uint8_t)sample,(uint8_t)(i>>10));
            ntm_input_update(b,(uint8_t)sample);
            assert(actual==oracle(&y,b[0],b[1],(uint8_t)(i>>10)));
            assert(!memcmp(a,b,16) && x.selection==y.selection && x.result==y.result);
        }
        assert(ntm_course_choice_tick(0,&x,0,0)==-1);
        assert(ntm_course_choice_tick(a,0,0,0)==-1);
    }
    assert(ntm_course_choice_update(0,0,0,0)==-1);
    printf("PASS native course: %lu transitions plus 65536 composed ticks\n",cases);return 0;
}
