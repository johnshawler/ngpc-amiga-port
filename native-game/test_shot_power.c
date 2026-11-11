/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_power.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define CALLBACK(name) static int name(ntm_task_context *c,ntm_task *t){(void)c;(void)t;assert(0);return -1;}
CALLBACK(meter) CALLBACK(delay) CALLBACK(ready) CALLBACK(accuracy)
CALLBACK(adelay) CALLBACK(aready) CALLBACK(rebuild) CALLBACK(restore)
CALLBACK(back) CALLBACK(closed)
static ntm_shot_power initial(void)
{ntm_shot_power p={meter,delay,ready,accuracy,adelay,aready,rebuild,restore,back,closed,0,0};return p;}
int main(void)
{
    static uint8_t maps[4096],saved[32],saved_meter[4]={0x12,0x34,0x56,0x78};
    ntm_hole_prepare_state hole={0};ntm_shot_aim aim={0};ntm_course_intro_context intro={0};
    ntm_tee_setup_context shared={0};ntm_task_context tasks={0};ntm_task nodes[8]={{0}},fallback={0};
    uint8_t overlay=0,view[4]={0};uint16_t words[2]={0};unsigned i,j,n,p,phase,count=0;
    ntm_shot_scene_object_context scene={&shared,&hole,0,&aim,saved};
    ntm_shot_power_context ctx={&scene,&overlay,saved_meter};
    intro.maps=maps;intro.viewport=view;intro.screen_words=words;shared.intro=&intro;shared.tasks=&tasks;shared.fallback=&fallback;
    for(i=0;i<32;i++)saved[i]=(uint8_t)(i+1);
    for(n=0;n<3;n++)for(p=0;p<256;p++){
        ntm_shot_power s=initial();memset(nodes,0,sizeof(nodes));memset(maps,0xa5,sizeof(maps));
        nodes[0].next=nodes+1;nodes[1].previous=nodes;nodes[0].priority=(uint8_t)p;
        tasks.current=nodes;tasks.free_head=n?nodes+2:0;
        for(i=0;i<n;i++){nodes[2+i].next=i+1<n?nodes+3+i:0;nodes[2+i].result=0x5a;}
        aim.reset_d1=255;overlay=123;
        assert(!ntm_shot_power_initialize(&s,nodes,&ctx));
        assert(aim.reset_d1==2 && overlay==123 && s.phase==1 && nodes[0].delay==3 && nodes[0].callback==delay);
        for(i=0;i<4096;i++){
            unsigned rel=i-0x10e;int changed=i>=0x10e && rel<256 && rel%64<8;
            assert(maps[i]==(changed?saved[8*(rel/64)+rel%64]:0xa5));
        }
        if(n){assert(nodes[0].next==nodes+2 && nodes[2].callback==meter && nodes[2].owner==nodes);assert(nodes[2].priority==(uint8_t)(p+1) && nodes[2].result==0x5a);}
        else assert(nodes[0].next==nodes+1);
        count++;
    }
    /* Exhaust every signed delay word with all consequential edge combinations.
     * Child callbacks are deliberately non-executable test identities. */
    for(phase=1;phase<=4;phase++)for(n=0;n<65536;n++)for(p=0;p<4;p++){
        ntm_shot_power s=initial();int crosses=!!((uint16_t)(n-1u)&32768u);int gate=phase==2 || phase==4 || crosses;
        memset(nodes,0,sizeof(nodes));nodes[0].next=nodes+1;tasks.free_head=0;tasks.current=nodes;
        nodes[0].callback=phase==1?delay:phase==2?ready:phase==3?adelay:aready;nodes[0].delay=(uint16_t)n;
        s.phase=(uint8_t)phase;aim.club=0;aim.reset_d1=2;overlay=0;
        assert(ntm_shot_power_tick(&s,nodes,&ctx,(uint8_t)(p*16))==(gate && phase>=3 && (p&1)?2:0));
        if(gate && phase<=2){
            if(p&2)assert(s.phase==6 && nodes[0].callback==restore && overlay==255);
            else if(p&1)assert(s.phase==3 && nodes[0].callback==adelay && nodes[0].delay==3 && aim.reset_d1==3);
            else assert(s.phase==2 && nodes[0].callback==ready);
        }
        if(gate && phase>=3)assert((p&1)?s.boundary==NTM_SHOT_POWER_SWING:s.phase==4);
        count++;
    }
    for(i=0;i<256;i++){
        ntm_shot_power s=initial();s.phase=4;nodes[0].callback=aready;aim.club=13;
        assert(ntm_shot_power_tick(&s,nodes,&ctx,(uint8_t)i)==2 && s.boundary==NTM_SHOT_POWER_SWING);
        assert(ntm_shot_power_tick(&s,nodes,&ctx,0)==-1);count++;
    }
    for(n=0;n<4;n++)for(p=0;p<8;p++){
        ntm_shot_power s=initial();memset(nodes,0,sizeof(nodes));tasks.free_head=nodes+5;tasks.current=nodes;
        nodes[0].priority=10;nodes[0].previous=nodes+1;nodes[1].next=nodes;nodes[1].priority=10;
        for(i=0;i<n;i++){nodes[2+i].priority=11;nodes[2+i].flags=(uint8_t)((p>>i)&1);nodes[2+i].previous=i?nodes+1+i:nodes;nodes[2+i].next=i+1<n?nodes+3+i:nodes+1;}
        nodes[0].next=n?nodes+2:nodes+1;nodes[1].previous=n?nodes+1+n:nodes;
        assert(!ntm_shot_power_cancel(&s,nodes,&ctx));assert(nodes[0].next==nodes+1 && nodes[1].previous==nodes);
        assert(overlay==255 && view[0]==0 && view[1]==0 && view[2]==0 && view[3]==152 && s.phase==5 && nodes[0].callback==rebuild);
        for(i=0,j=5;i<n;i++)if(!((p>>i)&1)){assert(nodes[2+i].next==nodes+j);j=2+i;}
        assert(tasks.free_head==nodes+j);memset(maps,0xa5,sizeof(maps));words[0]=words[1]=123;aim.reset_d1=255;
        assert(ntm_shot_power_tick(&s,nodes,&ctx,0)==2 && s.boundary==NTM_SHOT_POWER_WORLD);
        assert(!aim.reset_d1 && !words[0] && !words[1]);
        for(i=0;i<4096;i++){unsigned rel=i-0x84;int changed=i>=0x84 && rel<15*64 && rel%64<28;assert(maps[i]==(changed?0:0xa5));}
        assert(ntm_shot_power_tick(&s,nodes,&ctx,0)==-1);count++;
    }
    {
        ntm_shot_power s=initial();s.phase=2;nodes[0].callback=ready;memset(maps,0xa5,sizeof(maps));
        assert(!ntm_shot_power_tick(&s,nodes,&ctx,48) && s.phase==6);
        assert(!ntm_shot_power_tick(&s,nodes,&ctx,0) && s.phase==7 && aim.reset_d1==1);
        for(i=0;i<4096;i++){unsigned rel=i-0x40;int changed=i>=0x40 && rel<12*64 && rel%64<4;assert(maps[i]==(changed?saved_meter[rel%64]:0xa5));}
        assert(!ntm_shot_power_tick(&s,nodes,&ctx,0) && s.phase==8 && nodes[0].callback==closed && nodes[0].delay==3);
        assert(ntm_shot_power_tick(&s,nodes,&ctx,0)==-1);count++;
    }
    {ntm_shot_power s=initial();assert(ntm_shot_power_initialize(0,nodes,&ctx)==-1);assert(ntm_shot_power_tick(&s,nodes,&ctx,0)==-1);}
    printf("shot power parent: %u allocation, delay, input and cancellation cases PASS\n",count);return 0;
}
