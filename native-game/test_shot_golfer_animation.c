/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_golfer_animation.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void putw(uint8_t *p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static unsigned animation_cases,pin_cases;
static const uint8_t kinds[5]={0,4,8,12,252};
static uint8_t identities[20];
static ntm_shot_golfer_animation_resources resources;
static void animation(unsigned kind,unsigned fg,unsigned delay,unsigned fgdelay,
    unsigned frame,unsigned fgframe,unsigned reload,unsigned fgreload,unsigned queue)
{
    ntm_shot_golfer s={0};ntm_course_screen screen={0};uint8_t before[8];unsigned layer,queued=0;
    s.phase=1;s.task_bytes[0]=(uint8_t)frame;s.task_bytes[1]=(uint8_t)delay;s.task_bytes[2]=(uint8_t)reload;s.task_bytes[3]=(uint8_t)kind;
    s.task_bytes[4]=(uint8_t)fgframe;s.task_bytes[5]=(uint8_t)fgdelay;s.task_bytes[6]=(uint8_t)fgreload;s.task_bytes[7]=(uint8_t)fg;
    memcpy(before,s.task_bytes,8);screen.stamp_bytes=(uint16_t)queue;
    assert(!ntm_shot_golfer_animation_tick(&s,&screen,&resources));
    for(layer=0;layer<2;layer++) {
        unsigned at=4*layer,row=layer?4:kind/4;
        if(before[at+3]==252)assert(!memcmp(s.task_bytes+at,before+at,4));
        else {
            assert(s.task_bytes[at+1]==(before[at+1]==1?before[at+2]:(uint8_t)(before[at+1]-1u)));
            assert(s.task_bytes[at]==(before[at+1]==1?((before[at]+2u)&7u):before[at]));
            assert(s.task_bytes[at+2]==before[at+2] && s.task_bytes[at+3]==before[at+3]);
            if(before[at+1]==1 && queue+8*queued<256) {
                ntm_screen_stamp *stamp=screen.stamps+queue/8+queued;
                assert(stamp->asset.data==resources.frames[row][s.task_bytes[at]/2].data);
                assert(stamp->offset==(layer?0xb44:0xa44) && stamp->tile==(layer?0x1b6:0x190));queued++;
            }
        }
    }
    assert(screen.stamp_bytes==queue+8*queued);animation_cases++;
}
int main(void)
{
    unsigned i,j,delay,frame,fgframe,q,reload;
    for(i=0;i<5;i++)for(j=0;j<4;j++)resources.frames[i][j].data=identities+4*i+j;
    for(i=0;i<5;i++)for(j=0;j<2;j++)for(delay=0;delay<256;delay++)
        animation(kinds[i],j?252:0,delay,255-delay,2*(delay%4),2*((delay/4)%4),delay,255-delay,(delay%3==0?0:delay%3==1?248:256));
    for(i=0;i<5;i++)for(j=0;j<2;j++)for(frame=0;frame<8;frame+=2)for(fgframe=0;fgframe<8;fgframe+=2)for(q=0;q<3;q++)for(reload=0;reload<4;reload++)
        animation(kinds[i],j?252:0,1,1,frame,fgframe,reload,255-reload,q==0?0:q==1?248:256);
    for(frame=1;frame<256;frame+=2)animation(0,0,0,255,frame,frame,3,5,0);
    /* Missing resources, unknown dispatch and odd expiring frame cannot partly
     * advance the other channel or append a job. Static channels need no art. */
    {
        ntm_shot_golfer s={0},before;ntm_course_screen screen={0},oldscreen;
        s.phase=1;s.task_bytes[1]=s.task_bytes[5]=1;s.task_bytes[7]=4;before=s;oldscreen=screen;
        assert(ntm_shot_golfer_animation_tick(&s,&screen,&resources)==NTM_TASK_BOUNDARY);
        assert(!memcmp(&s,&before,sizeof(s)) && !memcmp(&screen,&oldscreen,sizeof(screen)));
        s.task_bytes[7]=0;s.task_bytes[4]=1;before=s;
        assert(ntm_shot_golfer_animation_tick(&s,&screen,&resources)==NTM_TASK_BOUNDARY && !memcmp(&s,&before,sizeof(s)));
        s.task_bytes[4]=0;before=s;
        assert(ntm_shot_golfer_animation_tick(&s,&screen,0)==-1 && !memcmp(&s,&before,sizeof(s)));
        for(i=0;i<256;i++)if(i!=0 && i!=4 && i!=8 && i!=12 && i!=252) {
            s.task_bytes[3]=(uint8_t)i;before=s;
            assert(ntm_shot_golfer_animation_tick(&s,&screen,&resources)==NTM_TASK_BOUNDARY && !memcmp(&s,&before,sizeof(s)));
        }
        s.task_bytes[3]=s.task_bytes[7]=252;before=s;
        assert(!ntm_shot_golfer_animation_tick(&s,&screen,0) && !memcmp(&s,&before,sizeof(s)));
        screen.stamp_bytes=255;assert(ntm_shot_golfer_animation_tick(&s,&screen,0)==-1);
    }
    {
        ntm_shot_golfer s={0};ntm_hole_prepare_state hole={0};ntm_course_cursor objects[32]={{0}};
        uint16_t x[32]={0},words[2]={0};ntm_course_intro_context intro={0};ntm_tee_setup_context shared={0};ntm_shot_scene_object_context scene={0};
        ntm_shot_golfer_green green={0,32,255};ntm_course_cursor *pin=objects+5;
        intro.screen_words=words;shared.intro=&intro;shared.objects=objects;shared.x=x;scene.shared=&shared;scene.hole=&hole;
        x[5]=72;
        for(i=0;i<65536;i++) {
            putw(hole.hole+0x6a,i);pin->byte15=19;
            assert(!ntm_shot_golfer_pin_adjust(&s,&scene,&green));
            assert(pin->byte15==((i<192 || i>=32768)?3:19));pin_cases++;
        }
        for(j=0;j<3;j++)for(i=0;i<65536;i++) {
            s.surface=(uint8_t)(j?j+5:0);pin->velocity=(uint16_t)i;pin->flags=5;
            assert(!ntm_shot_golfer_pin_adjust(&s,&scene,&green));
            assert(pin->velocity==(j && i>112 && i<32768?112:i));
            assert(pin->flags==(j && i>112 && i<32768?133:5));pin_cases++;
        }
        putw(hole.hole+0x6a,100);green.status=0;green.center_x=0;green.half_width=32;
        for(i=0;i<144;i++) {
            x[5]=(uint16_t)i;pin->byte15=19;
            assert(!ntm_shot_golfer_pin_adjust(&s,&scene,&green));
            assert(pin->byte15==(i<18 || i>126 || (i>=40 && i<=104)?2:3));pin_cases++;
        }
        assert(ntm_shot_golfer_pin_adjust(&s,&scene,0)==-1);
    }
    printf("shot golfer animation:%u authored callback cases,%u pin arithmetic cases PASS\n",animation_cases,pin_cases);return 0;
}
