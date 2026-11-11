/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "tee_setup.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static ntm_course_screen screen;static ntm_display_reset_state display;static ntm_course_palette palette;
static ntm_course_cursor objects[32];static uint16_t x[32],words[2];static uint8_t tiles[8192],maps[4096],sx[2],sy[2],flags,vp[4];
static ntm_task nodes[5];static ntm_task_context tasks;static ntm_course_intro_context intro;
static ntm_tee_setup_context context;static ntm_tee_setup_resources resources;static ntm_tee_setup tee;
static ntm_tee_object_assets object_resources;static uint8_t saved[54],handicaps[18];
static uint8_t image[]={1,0,0,0,0,0,0,0,129,0,0,255,255},pixels[16];
static const uint8_t metadata[4][5]={{52,0,12,0,3},{52,0,0,0,3},{52,0,4,0,3},{52,0,16,0,3}};
static unsigned requests,preparations;static uint16_t commands[8],parameters[8];static uint8_t services[8];
#define CB(n) static int cb##n(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
CB(0) CB(1) CB(2) CB(3) CB(4) CB(5) CB(6) CB(7)
static void sound(void *unused,uint8_t service,uint16_t command,uint16_t parameter)
{(void)unused;assert(requests<8);services[requests]=service;commands[requests]=command;parameters[requests++]=parameter;}
static int prepare(void *unused,ntm_tee_setup_context *c)
{(void)unused;assert(requests==2);preparations++;c->par=4;c->distance=305;return 0;}
static void ring(void)
{
    memset(nodes,0,sizeof nodes);memset(&tasks,0,sizeof tasks);
    nodes[0].previous=nodes+1;nodes[0].next=nodes+2;nodes[0].owner=nodes+1;nodes[0].priority=2;nodes[0].callback=cb0;
    nodes[1].next=nodes;nodes[1].previous=nodes+2;nodes[1].priority=1;
    nodes[2].previous=nodes;nodes[2].next=nodes+1;
    nodes[4].flags=1;tasks.current=nodes;
}
static void seed(void)
{
    unsigned i,j,k;ntm_screen_asset a={image,sizeof image,pixels,sizeof pixels};
    memset(&screen,0,sizeof screen);memset(&display,0,sizeof display);memset(&palette,0,sizeof palette);
    memset(objects,0x5a,sizeof objects);memset(x,0x5a,sizeof x);memset(maps,0xa5,sizeof maps);memset(tiles,0xa5,sizeof tiles);
    memset(&context,0,sizeof context);memset(&resources,0,sizeof resources);memset(&tee,0,sizeof tee);memset(saved,10,sizeof saved);memset(handicaps,0,sizeof handicaps);
    ring();requests=preparations=0;
    intro.screen=&screen;intro.display=&display;intro.palette=&palette;intro.maps=maps;intro.scroll_x=sx;intro.scroll_y=sy;intro.flags=&flags;intro.viewport=vp;intro.screen_words=words;
    context.intro=&intro;context.tasks=&tasks;context.fallback=nodes+4;context.objects=objects;context.x=x;context.tiles=tiles;
    context.saved_hole_scores=saved;context.handicaps=handicaps;context.prepare=prepare;context.sound_request=sound;
    context.par=4;context.pin_x=500;context.pin_y=600;context.origin_x=470;context.origin_y=550;
    tee.notice_entry=cb0;tee.notice_wait=cb1;tee.setup_delay=cb2;tee.open_delay=cb3;tee.hold=cb4;tee.handicap_entry=cb5;tee.handicap_wait=cb6;
    for(i=0;i<8;i++)palette.slots[i]=(uint16_t)(0xff00+4*i);
    for(i=0;i<2;i++){resources.palettes[i].kind=2;resources.palettes[i].offset=(uint16_t)(480+2*i);resources.palettes[i].control_offset=0x118;}
    resources.font=resources.header=a;resources.objects=&object_resources;
    for(i=0;i<18;i++)resources.hole_numbers[i]=a;
    for(i=0;i<3;i++)resources.pars[i]=a;
    for(i=0;i<3;i++)for(j=0;j<18;j++){
        for(k=0;k<3;k++)resources.holes[i][j].views[k]=a;
        resources.holes[i][j].marker_x=(uint16_t)(i*10+j);resources.holes[i][j].marker_y=(uint16_t)(200-j);
    }
    for(i=0;i<4;i++){resources.metadata[i]=metadata[i];resources.metadata_size[i]=5;}
    for(i=0;i<32;i++){objects[i].id=display.object_ids[i]=(uint8_t)i;objects[i].tag=display.object_tags[i]=65535;}
    words[0]=0x1234;words[1]=0xabcd;flags=255;memset(sx,7,2);memset(sy,9,2);
}
int main(void)
{
    unsigned phase,delay,skip,cases=0,i,j,course,hole,badge;uint8_t before[8192];uint32_t scratch;
    seed();
    for(i=0;i<65536;i++)for(j=1;j<=4;j++){
        unsigned n=i,k;uint8_t digits[4];uint32_t expected=0xaabbccdd;
        for(k=j;k;k--){digits[k-1]=(uint8_t)(n%10);n/=10;}
        for(k=0;k+1<j && !digits[k];k++)digits[k]=16;
        for(k=0;k<j;k++)expected=(expected&~((uint32_t)255<<(8*k)))|((uint32_t)digits[k]<<(8*k));
        scratch=0xaabbccdd;assert(!ntm_tee_draw_number((uint16_t)i,(uint8_t)j,0x28,0xfff0,4088,&scratch,maps));assert(scratch==expected);
        for(k=0;k<j;k++){unsigned tile=(digits[k]+16+0x2800+0xfff0)&65535;assert(maps[4088+2*k]==(tile&255) && maps[4089+2*k]==tile/256);}
    }
    memset(before,0x33,sizeof before);memcpy(tiles,before,sizeof before);
    {ntm_screen_asset a=resources.font;a.size=7;assert(ntm_tee_copy_tiles(&a,0,tiles)==-1);assert(!memcmp(tiles,before,sizeof before));a=resources.font;a.tiles_size=15;assert(ntm_tee_copy_tiles(&a,0,tiles)==-1);assert(!memcmp(tiles,before,sizeof before));}
    assert(ntm_tee_draw_number(0,0,0,0,0,&scratch,maps)==-1);assert(ntm_tee_draw_number(0,4,0,0,4090,&scratch,maps)==-1);
    for(i=0;i<256;i++)for(j=0;j<256;j++){
        unsigned d=(i-j)&255,expected=i==1?0:!(d&128)?255:((d+4)*4)&255;
        assert(ntm_tee_badge_offset((uint8_t)i,(uint8_t)j)==expected);
    }
    for(i=0;i<33;i++){
        seed();screen.stamp_bytes=(uint16_t)(8*i);tasks.free_head=nodes+3;
        assert(!ntm_tee_setup_initialize(&tee,nodes,&context,&resources));
        assert(requests==2 && preparations==1 && services[0]==1 && services[1]==1 && commands[0]==65535 && commands[1]==65534 && parameters[0]==8 && parameters[1]==8);
        assert(tee.phase==1 && nodes[0].callback==cb1 && nodes[0].next==nodes+3 && nodes[3].callback==cb0);
        assert(words[0]==0 && words[1]==0 && !sx[0] && !sy[1] && !context.text_base);
        assert(screen.stamp_bytes==(i<31?8*(i+2):256));assert(maps[0x902]==0x13 && maps[0x904]==0x10 && maps[0x906]==0x15);
        for(j=0;j<40;j++)assert(!maps[j]);
        assert(maps[0x800]==0x50);
        assert(ntm_tee_setup_initialize(&tee,nodes,&context,&resources)==-1);
    }
    for(course=0;course<3;course++)for(hole=0;hole<18;hole++)for(badge=0;badge<5;badge++){
        seed();context.course=(uint8_t)course;context.round_index=(uint8_t)hole;
        saved[18*course+hole]=(uint8_t)(badge==4?10:badge);tee.phase=1;nodes[0].callback=cb1;
        assert(!ntm_tee_setup_tick(&tee,nodes,&context,&resources));assert(tee.phase==2 && nodes[0].delay==1 && screen.stamp_bytes==24);
        assert(display.zero_flags[8]==(badge==4?3:4));assert(objects[0].tag==30 && objects[1].tag==29 && objects[2].tag==31);
        assert(x[0]==96 && objects[0].y==8 && x[1]==126 && objects[1].y==58 && x[2]==course*10+hole && objects[2].y==200-hole);
        assert(tee.asset_group==4*(course+36) && tee.asset_index==4*hole);
        assert(words[0]==0x1200 && words[1]==0xab00);
    }
    seed();
    for(phase=2;phase<=4;phase++)for(delay=0;delay<65536;delay++)for(skip=0;skip<2;skip++){
        unsigned expired=(phase==4 && skip)||(((delay-1u)&65535)&32768);
        ring();tee.phase=(uint8_t)phase;nodes[0].callback=phase==2?cb2:phase==3?cb3:cb4;nodes[0].delay=(uint16_t)delay;context.control=(uint8_t)(skip?16:0);
        assert(!ntm_tee_setup_tick(&tee,nodes,&context,&resources));
        if(!expired){assert(tee.phase==phase && nodes[0].delay==(uint16_t)(delay-1));}
        else if(phase==2){assert(tee.phase==3 && nodes[0].callback==cb3 && nodes[0].delay==3 && vp[2]==160);}
        else if(phase==3){assert(tee.phase==4 && nodes[0].callback==cb4 && nodes[0].delay==179);}
        else {assert(tee.phase==5 && nodes[0].callback==cb6 && nodes[0].delay==(uint16_t)(skip?delay:delay-1));}
        cases++;
    }
    for(delay=0;delay<65536;delay++){
        ntm_tee_handicap h={cb0,cb7,1,0};ring();nodes[0].callback=cb0;nodes[0].delay=(uint16_t)delay;
        assert(ntm_tee_handicap_tick(&h,nodes,&context)==(((delay-1u)&32768)?NTM_TEE_COMPLETE:0));
        assert(nodes[0].delay==(uint16_t)(delay-1));cases++;
    }
    for(i=0;i<256;i++)for(course=0;course<3;course++)for(j=0;j<2;j++){
        ntm_tee_owner owner={cb0,cb1,0,0,0xa5,0,0,sound,0};
        ring();requests=0;owner.skip_overview=(uint8_t)i;owner.course=(uint8_t)course;owner.current_music=(uint8_t)(course+j);
        assert(ntm_tee_owner_tick(&owner,nodes,&tasks,nodes+4)==(i!=0));
        if(!i){assert(owner.phase==1 && nodes[0].callback==cb1 && !requests);assert(ntm_tee_owner_tick(&owner,nodes,&tasks,nodes+4)==1);assert(owner.player_status==0);}
        else assert(owner.player_status==0xa5);
        assert(owner.phase==2 && !owner.skip_overview && requests==2*j);
        if(j)assert(services[0]==0 && commands[0]==65535 && commands[1]==65534 && parameters[0]==course && parameters[1]==course);
        assert(ntm_tee_owner_tick(&owner,nodes,&tasks,nodes+4)==1 && requests==2*j);
    }
    seed();context.game_mode=2;assert(ntm_tee_setup_initialize(&tee,nodes,&context,&resources)==-1 && !requests);
    context.game_mode=0;context.prepare=0;assert(ntm_tee_setup_initialize(&tee,nodes,&context,&resources)==-1 && !requests);
    printf("PASS tee overview: %u timer states,262144 decimal values,65536 badge pairs,all54 holes and owner branches\n",cases);return 0;
}
