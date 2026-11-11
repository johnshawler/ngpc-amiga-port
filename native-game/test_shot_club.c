/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_club.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned sound_count;
static void sound(void *p,uint8_t service,uint16_t command,uint16_t parameter)
{(void)p;assert(service==2 && !command && !parameter);sound_count++;}
static int tick(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static void put(uint8_t *p,unsigned n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
int main(void)
{
    ntm_shot_club_resources r;ntm_shot_aim_resources math;ntm_shot_scene_object_resources scene;
    ntm_shot_club s;ntm_task t;ntm_shot_scene_object_context c;ntm_tee_setup_context shared;
    ntm_course_intro_context intro;ntm_course_screen screen;ntm_display_reset_state display;
    ntm_hole_prepare_state hole;ntm_shot_player_state player;ntm_shot_aim aim;ntm_course_cursor objects[32];
    uint8_t powers[56],lies[42],overflow[42],minimums[256],scale[256],remainders[10];uint16_t words[2];
    uint8_t maps[4096],saved[32],tiles[8192],art[23][11],pixels[16];
    unsigned i,j,k,cases=0,controls=0;
    memset(&r,0,sizeof r);memset(&math,0,sizeof math);memset(&scene,0,sizeof scene);memset(&c,0,sizeof c);
    memset(&shared,0,sizeof shared);memset(&intro,0,sizeof intro);memset(&hole,0,sizeof hole);memset(&player,0,sizeof player);
    memset(&aim,0,sizeof aim);memset(objects,0,sizeof objects);memset(&display,0,sizeof display);memset(minimums,1,sizeof minimums);
    memset(scale,100,sizeof scale);memset(remainders,0,sizeof remainders);words[0]=7;words[1]=40;
    for(i=0;i<14;i++){put(powers+4*i,400-i*20);put(powers+4*i+2,i);lies[3*i]=2;put(lies+3*i+1,300+i);overflow[3*i]=247;put(overflow+3*i+1,800+i);}
    for(i=0;i<6;i++){math.minimums[i]=minimums;math.minimum_sizes[i]=256;}
    scene.power_scale=scale;scene.power_scale_size=256;scene.remainders=remainders;scene.remainders_size=10;
    r.aim=&math;r.scene=&scene;
    for(i=0;i<12;i++){r.powers[i].data=powers;r.powers[i].size=56;for(j=0;j<15;j++){
        r.lies[i][j].data=lies;r.lies[i][j].size=42;r.overflow_lies[i][j].data=overflow;r.overflow_lies[i][j].size=42;}}
    intro.screen=&screen;intro.display=&display;intro.screen_words=words;shared.intro=&intro;shared.objects=objects;
    c.shared=&shared;c.hole=&hole;c.player=&player;c.aim=&aim;c.saved_map=saved;
    intro.maps=maps;shared.sound_request=sound;
    for(i=0;i<32;i++)saved[i]=(uint8_t)(i*7+3);
    memset(pixels,0xa5,sizeof pixels);
    for(i=0;i<23;i++) {
        memset(art[i],0,sizeof art[i]);art[i][0]=1;art[i][8]=1;art[i][9]=(uint8_t)i;art[i][10]=255;
        r.highlights[i].data=art[i];r.highlights[i].size=11;r.highlights[i].tiles=pixels;r.highlights[i].tiles_size=16;
        r.labels[i]=r.highlights[i];
    }
    for(i=0;i<11;i++)r.digits[i]=r.highlights[i];
    for(i=0;i<19;i++)r.lie_icons[i]=r.highlights[i];
    for(i=0;i<14;i++)for(j=0;j<15;j++)for(k=0;k<3;k++) {
        unsigned expected=i?i:13;
        memset(&s,0,sizeof s);memset(&t,0,sizeof t);memset(&screen,0,sizeof screen);
        s.wait=tick;s.change_flag=31;aim.club=(uint8_t)i;hole.hole[0x70]=(uint8_t)j;hole.hole[0x77]=255;
        put(hole.hole+0x46,100);put(hole.hole+0x4a,100);screen.stamp_bytes=(uint16_t)(k==0?0:k==1?224:240);
        if(j==2 || j==6 || j==11){if(i==0)expected=12;else if(i==13)expected=1;}
        assert(ntm_shot_club_initialize(&s,&t,&c,&r)==0);
        assert(s.phase==1 && t.callback==tick && s.change_flag==31 && !s.delta && !s.restore_delay && !hole.hole[0x77]);
        assert(aim.club==expected && s.label==expected && s.minimum==1);
        assert(hole.hole[0x7b]==(k==2?247:2));
        assert(screen.stamp_bytes==(k?256:40));
        assert(hole.hole[0x7e]==100 && hole.hole[0x7f]==80 && objects[10].y==48);
        assert(ntm_shot_club_tick(&s,&c)==0);aim.reset_4416=1;assert(ntm_shot_club_tick(&s,&c)==2);aim.reset_4416=0;
        cases++;
    }
    assert(cases==630);assert(ntm_shot_club_initialize(0,&t,&c,&r)==-1);
    hole.hole[0x70]=0;hole.players[0][0x33]=0;s.minimum=0;s.maximum=13;
    for(i=0;i<256;i++)for(j=0;j<4;j++) {
        unsigned changed=!(i&12) && (j==1 || j==2);
        memset(&screen,0,sizeof screen);aim.club=7;aim.reset_d1=aim.reset_4416=0;
        s.delta=s.restore_delay=0;s.change_flag=77;sound_count=0;
        assert(!ntm_shot_club_control(&s,&c,&r,(uint8_t)i,(uint8_t)j));
        assert(aim.club==(changed?(j==1?6:8):7) && s.restore_delay==(changed?31:0));
        assert(s.change_flag==(changed?255:77) && sound_count==changed && !s.delta);
        assert(screen.stamp_bytes==(changed?48:0));
        if(changed) {
            assert(screen.stamps[5].asset.data==r.highlights[aim.club].data);
            assert(screen.stamps[5].offset==0x10e && screen.stamps[5].tile==0x20e4);
            assert(!ntm_course_screen_stamp_queue(&screen,tiles,maps));
            assert(maps[0x10e]==(uint8_t)(0xe4+aim.club) && maps[0x10f]==0x20);
        }
        controls++;
    }
    for(i=0;i<256;i++)for(j=0;j<3;j++) {
        unsigned active=i<2 || i>=128,changed=active && !j;
        memset(&screen,0,sizeof screen);memset(maps,0xcc,sizeof maps);aim.club=7;aim.reset_d1=(uint8_t)i;
        aim.reset_4416=(uint8_t)(j==1);hole.hole[0x77]=(uint8_t)(j==2);
        s.delta=0;s.restore_delay=1;s.change_flag=77;sound_count=0;
        assert(!ntm_shot_club_control(&s,&c,&r,1,1));
        assert(aim.club==(changed?6:7) && s.restore_delay==(changed?31:active?0:1));
        assert(sound_count==changed && s.change_flag==(changed?255:77));
        for(k=0;k<32;k++)assert(maps[0x10e +64*(k/8)+k%8]==(active&&!changed?saved[k]:0xcc));
        controls++;
    }
    /* The legacy boundary wrapper must not silently skip signed-negative states. */
    aim.reset_d1=255;s.restore_delay=1;
    assert(ntm_shot_club_tick(&s,&c)==2);
    aim.reset_d1=127;assert(!ntm_shot_club_tick(&s,&c));
    aim.reset_d1=aim.reset_4416=hole.hole[0x77]=0;
    for(i=0;i<256;i++)for(j=0;j<3;j++) {
        unsigned changed=j!=0;
        memset(&screen,0,sizeof screen);memset(maps,0xcc,sizeof maps);aim.club=7;
        s.delta=(uint8_t)(j==1?1:j==2?255:0);s.restore_delay=(uint8_t)i;sound_count=0;
        assert(!ntm_shot_club_control(&s,&c,&r,0,0));
        assert(aim.club==(j==1?8:j==2?6:7) && s.restore_delay==(i?i-1:0));
        assert(!sound_count && !s.delta && screen.stamp_bytes==(changed?(i>1?48:40):0));
        for(k=0;k<32;k++)assert(maps[0x10e +64*(k/8)+k%8]==(i==1?saved[k]:0xcc));
        controls++;
    }
    /* Both signed wrap directions, nonzero modifier highlight, repeated press
     * resets the32-update timer; a held direction alone does not repeat. */
    memset(&screen,0,sizeof screen);aim.club=0;s.delta=s.restore_delay=0;
    assert(!ntm_shot_club_control(&s,&c,&r,1,1) && aim.club==13 && s.restore_delay==31);
    hole.players[0][0x33]=6;
    assert(!ntm_shot_club_control(&s,&c,&r,2,2) && aim.club==0 && s.restore_delay==31);
    assert(screen.stamps[11].tile==0x3ae4);
    assert(!ntm_course_screen_stamp_queue(&screen,tiles,maps));
    for(i=0;i<31;i++)assert(!ntm_shot_club_control(&s,&c,&r,2,0));
    assert(!s.restore_delay && aim.club==0);
    for(k=0;k<32;k++)assert(maps[0x10e +64*(k/8)+k%8]==saved[k]);
    assert(!screen.stamp_bytes && t.callback==tick && s.phase==1);
    assert(ntm_shot_club_control(0,&c,&r,0,0)==-1);
    printf("shot club:%u authored bounds and%u control cases, signed bounds, exact highlight queues, expiry and sound requests\n",cases,controls);
    return 0;
}
