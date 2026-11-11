/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_screen.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef NTM_SCREEN_PRIVATE_ASSETS
/* Host/QEMU contract fixture, not the Amiga assembly copy implementation. */
void ntm_course_screen_copy_private(void *d,const void *s,unsigned n){memcpy(d,s,n);}
#endif
static void word(uint8_t *p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
int main(void)
{
    unsigned wide,flags,trial,cases=0,i,y,x;
    uint8_t a[256],pixels[32],t[8192],m[4096],et[8192],em[4096];
    for(i=0;i<32;i++)pixels[i]=(uint8_t)(i*13);
    for(wide=0;wide<2;wide++)for(flags=0;flags<4;flags++)for(trial=0;trial<512;trial++) {
        unsigned lengths[3]={1+trial%13,1+(trial/13)%17,1+(trial/17)%11};
        unsigned p=11,base=(trial*193u)&0x3ffeu;
        ntm_screen_stamp s={{a,0,pixels,sizeof pixels},128,(uint16_t)(base|(flags<<14))};
        memset(a,0,sizeof a);word(a,2);word(a+6,16);word(a+8,0);a[10]=(uint8_t)(3|(wide?128:0));
        memset(t,77,sizeof t);memset(m,99,sizeof m);memcpy(et,t,sizeof t);memcpy(em,m,sizeof m);
        for(i=0;i<32;i++)et[16*(base&511)+i]=pixels[(i+16)%32];
        for(y=0;y<3;y++) {
            for(x=0;x<lengths[y];x++) {
                unsigned v=wide?(trial*997u+y*431u+x*17777u)%65535u:(trial+y*31+x*13)%255;
                int dest=128+64*(int)y+2*((flags&2)?(int)lengths[0]-1-(int)x:(int)x);
                if(wide){word(a+p,v);p+=2;}else a[p++]=(uint8_t)v;
                word(em+dest,((v+base)&65535)^((flags&2)?32768:0));
            }
            a[p++]=255;if(wide)a[p++]=255;
        }
        s.asset.size=p;
        assert(!ntm_course_screen_stamp_valid(&s));
        assert(!ntm_course_screen_stamp(&s,t,m));
        assert(!memcmp(t,et,sizeof t)&&!memcmp(m,em,sizeof m));cases++;
    }
    /* Boundary and transactional failures, including a later wider reflected
     * row that would underflow although its first row is in bounds. */
    {
        uint8_t b[]={1,0,0,0,0,0,0,0,1,7,255};
        ntm_screen_stamp s={{b,sizeof b,pixels,sizeof pixels},4094,0x8000};
        ntm_course_screen queue={0},old;
        assert(!ntm_course_screen_stamp_valid(&s));
        assert(!ntm_course_screen_stamp(&s,t,m));
        memcpy(et,t,sizeof t);memcpy(em,m,sizeof m);
        for(i=0;i<7;i++) {
            s.asset.data=b;s.asset.size=sizeof b;s.offset=4094;s.tile=0x8000;b[8]=1;b[9]=7;
            if(i==0)s.asset.size--;
            if(i==1)b[8]=0;
            if(i==2)b[9]=255;
            if(i==3)s.offset=4095;
            if(i==4)s.asset.tiles_size=15;
            if(i==5){s.asset.tiles_size=32;s.asset.data=a;s.asset.size=sizeof a;memset(a,0,sizeof a);a[0]=1;a[8]=2;a[9]=0;a[10]=255;memset(a+11,1,34);a[45]=255;s.offset=0;}
            if(i==6){s.asset.tiles_size=32;s.asset.data=a;memset(a,0,sizeof a);a[0]=1;a[8]=1;a[9]=0;a[10]=1;a[11]=255;}
            assert(ntm_course_screen_stamp_valid(&s)==-1);
            assert(ntm_course_screen_stamp(&s,t,m)==-1);
            assert(!memcmp(t,et,sizeof t)&&!memcmp(m,em,sizeof m));
            queue.stamp_bytes=16;queue.stamps[0].asset.data=b;queue.stamps[0].asset.size=sizeof b;
            b[8]=1;b[9]=7;queue.stamps[0].asset.tiles=pixels;queue.stamps[0].asset.tiles_size=32;
            queue.stamps[0].offset=0;queue.stamps[0].tile=0;
            /* Use independently malformed second request, preserving the first. */
            queue.stamps[1]=queue.stamps[0];queue.stamps[1].asset.size=10;old=queue;
            assert(ntm_course_screen_stamp_queue(&queue,t,m)==-1);
            assert(!memcmp(&queue,&old,sizeof queue)&&!memcmp(t,et,sizeof t)&&!memcmp(m,em,sizeof m));
        }
    }
    printf("PASS reflected stamps: %u authored cases, boundary and atomic rejection checks\n",cases);
    return 0;
}
