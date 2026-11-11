/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "tee_objects.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Authored synthetic formats, not extracted graphics or original resources. */
static uint8_t one[]={2,192,4,0,0,128,0,0};
static uint8_t multi[]={2,0,8,0,3,128,16,0,0,0,1,2,8,128,3,4,8,0,5,6,0,128,7,8};
static uint8_t blinking[]={1,0,8,0,1,128,12,0,0,128,0,0,16,128,0,0};
static uint8_t shapes[]={128,0,0,0,0,0,0,0,128,0x11,0,0,0,0,1,0,0,255};
static uint8_t source[32],other_source[32],tiles[8192],sprites[256],palettes[64];
static ntm_tee_object_assets assets,other;
static const ntm_tee_object_assets *refs[32];
static ntm_tee_object_engine e,saved_e;static ntm_display_reset_state d,saved_d;
static ntm_course_cursor c[32],saved_c[32];static uint16_t x[32],scroll[2];static uint8_t global,saved_global;
static void assets_start(void)
{
    unsigned i;memset(&assets,0,sizeof assets);
    for(i=0;i<8;i++){assets.animations[i].data=one;assets.animations[i].size=sizeof one;assets.animations[i].frames=1;}
    assets.shapes=shapes;assets.shapes_size=sizeof shapes;assets.tiles=source;assets.tiles_size=sizeof source;
    assets.animations[4].data=multi;assets.animations[4].size=sizeof multi;assets.animations[4].frames=2;
    assets.animations[0].data=blinking;assets.animations[0].size=sizeof blinking;assets.animations[0].frames=2;
    other=assets;other.tiles=other_source;
}
static void start(unsigned count)
{
    unsigned i;assets_start();memset(&e,0,sizeof e);memset(c,0,sizeof c);memset(x,0,sizeof x);memset(scroll,0,sizeof scroll);
    memset(tiles,7,sizeof tiles);memset(sprites,7,sizeof sprites);memset(palettes,7,sizeof palettes);
    assert(!ntm_tee_objects_reset(&d,c));global=16;
    for(i=0;i<32;i++) {
        c[i].tag=30;c[i].width=52;c[i].velocity=12;c[i].tile=(uint16_t)(400+2*i);c[i].y=24;c[i].byte15=3;x[i]=56;
        refs[i]=i&1?&other:&assets;
        if(i<count)assert(!ntm_tee_object_register(&d,c+i));
    }
}
static void tail(void){assert(!ntm_tee_objects_tail(&e,c,&d,x,scroll,&global,refs));}
static void head(void){assert(!ntm_tee_objects_head(&e,&d,tiles,sprites,palettes));}
static void save(void){saved_e=e;saved_d=d;memcpy(saved_c,c,sizeof c);saved_global=global;}
static void unchanged(void){assert(!memcmp(&e,&saved_e,sizeof e));assert(!memcmp(&d,&saved_d,sizeof d));assert(!memcmp(c,saved_c,sizeof c));assert(global==saved_global);}
static void bad_tail(void){save();assert(ntm_tee_objects_tail(&e,c,&d,x,scroll,&global,refs)==-1);unchanged();}
int main(void)
{
    unsigned i,j,f,delay;
    for(i=0;i<32;i++){source[i]=(uint8_t)(i*13);other_source[i]=(uint8_t)(255-i*7);}
    start(4);c[0].second=1;c[1].second=3;c[2].second=0;c[3].second=3;
    tail();assert(d.list[0]==0x301 && d.list[1]==0x303 && d.list[2]==0x100 && d.list[3]==2);
    assert(d.zero_flags[10]==4 && d.zero_words[1]==24 && sprites[2]==7);
    assert(e.frame[0]==0 && e.delay[0]==2 && !c[0].flags);
    /* Resources may be rebound or records removed before head; queued jobs
     * retain their own spans and upload in original list order. */
    refs[1]=&assets;other.tiles=source;assert(!ntm_tee_object_remove(&d,c+1));head();
    for(i=0;i<16;i++){assert(tiles[16*402+i]==other_source[i]);assert(tiles[16*400+i]==source[i]);}
    assert(sprites[0]==(402&255) && sprites[4]==(406&255));
    assert(d.list[0]==2 && d.list[1]==0x303 && d.list[2]==0x100 && !d.list[3]);
    tail();assert(d.list[0]==2); /* Explicit removal alone does not dirty list. */
    c[2].second=255;tail();assert(d.list[0]==0xff02 && d.list[1]==0x303);
    for(i=0;i<4;i++)c[i].flags|=16;
    tail();assert(!d.zero_flags[8] && !d.zero_words[0]);head();
    for(i=0;i<256;i++)assert(!sprites[i]);
    global|=32;tail();assert(global&32); /* No objects: refresh flag retained. */
    start(32);save();assert(ntm_tee_object_register(&d,c)==1);unchanged();
    for(i=0;i<32;i++){assert(!ntm_tee_object_remove(&d,c+i));assert(ntm_tee_object_remove(&d,c+i)==1);}
    assert(!d.zero_flags[8]);
    for(i=0;i<32;i++) {
        start(32);c[i].flags|=16;tail();assert(d.zero_flags[8]==31 && c[i].tag==65535);
        for(j=0;j<31;j++)assert((d.list[j]&255)==(j==i?31:j));
    }
    start(1);c[0].tag=32;d.object_tags[0]=32;c[0].velocity=16;
    tail();assert(d.zero_flags[10]==2 && e.jobs[0].tile==400 && e.jobs[1].tile==401);
    assert(sprites[2]==7);head();assert(sprites[2]==57 && sprites[3]==26 && sprites[6]==67 && sprites[7]==36);
    tail();assert(e.delay[0]==1);tail();assert(e.frame[0]==4 && e.delay[0]==3 && c[0].byte30==128);head();
    assert(sprites[2]==69 && sprites[3]==38 && sprites[6]==63 && sprites[7]==32);
    for(i=0;i<65;i++) {
        start(1);c[0].tag=32;d.object_tags[0]=32;c[0].velocity=16;
        tail();d.zero_flags[10]=(uint8_t)i;d.zero_words[1]=(uint16_t)(6*i);
        for(j=0;j<i;j++)e.jobs[j]=e.jobs[0];
        c[0].flags|=128;
        tail();assert(d.zero_flags[10]==(i<63?i+2:64));
        if(i<64)assert(e.jobs[i].tile==400);
        if(i<63)assert(e.jobs[i+1].tile==401);
        head();
    }
    start(1);c[0].tag=29;d.object_tags[0]=29;c[0].velocity=0;
    tail();head();assert(d.zero_flags[13]==1);tail();assert(e.frame[0]==4 && !d.zero_flags[13] && !d.zero_flags[10]);
    head();tail();assert(e.frame[0]==0 && d.zero_flags[13]==1 && d.zero_flags[10]==1);
    /* Inclusive original edge: x=-7..159 and y=-7..151. Exhaust full words. */
    start(1);tail();head();c[0].flags=64;
    for(i=0;i<65536;i++) {
        x[0]=(uint16_t)i;tail();assert(d.zero_flags[13]==((uint16_t)(i+7)<=166));head();
    }
    x[0]=0;
    for(i=0;i<65536;i++) {
        c[0].y=(uint16_t)i;tail();assert(d.zero_flags[13]==((uint16_t)(i+7)<=158));head();
    }
    /* Independent finite animation flags/delay combinations, single component. */
    for(f=0;f<256;f++)for(delay=0;delay<256;delay++) {
        start(2);c[0].flags=(uint8_t)f;e.delay[0]=(uint8_t)delay;c[0].byte30=224;c[1].flags=192;
        tail();assert(e.delay[1]==3 && e.frame[1]==0 && c[1].flags==66);
        if(f&16)assert(c[0].tag==65535 && d.zero_flags[8]==1);
        else {
            unsigned expected=(f&128)?3:delay;
            if(!(f&64))expected=(expected-1)&255;
            assert(e.delay[0]==expected && e.frame[0]==0);
            assert((c[0].flags&1)==!!(f&32));
            if(!(f&64) && !expected)assert((c[0].flags&66)==66 && (c[0].byte30&64));
        }
        head();
    }
    /* Malformed inputs: every failure leaves engine/list/objects untouched. */
    start(2);refs[1]=NULL;bad_tail();refs[1]=&other;
    c[1].width=16;bad_tail();c[1].width=52;
    c[1].velocity=4;bad_tail();c[1].velocity=12;
    c[1].byte30=1;bad_tail();c[1].byte30=0;
    c[1].id=0;bad_tail();c[1].id=1;
    d.list[1]=0;bad_tail();d.list[1]=1;
    d.zero_words[0]=3;bad_tail();d.zero_words[0]=4;
    d.zero_flags[1]=8;bad_tail();d.zero_flags[1]=0;
    d.capacity=65;bad_tail();d.capacity=64;
    other.animations[3].size=7;bad_tail();other.animations[3].size=sizeof one;
    other.animations[3].frames=65;bad_tail();other.animations[3].frames=1;
    other.shapes_size=7;bad_tail();other.shapes_size=sizeof shapes;
    other.tiles_size=15;bad_tail();other.tiles_size=sizeof source;
    shapes[0]=0;bad_tail();shapes[0]=128;
    shapes[6]=2;bad_tail();shapes[6]=0;
    c[1].flags=0;e.frame[1]=2;bad_tail();e.frame[1]=0;c[1].flags=128;
    c[1].tile=512;bad_tail();c[1].tile=402;
    one[1]=1;bad_tail();one[1]=192;
    one[2]=255;bad_tail();one[2]=4;
    one[4]=127;one[5]=255;bad_tail();one[4]=0;one[5]=128;
    c[1].tag=32;d.object_tags[1]=32;c[1].velocity=16;c[1].flags|=32;bad_tail();
    start(1);tail();e.jobs[0].count=2;save();assert(ntm_tee_objects_head(&e,&d,tiles,sprites,palettes)==-1);unchanged();
    for(i=0;i<8192;i++)assert(tiles[i]==7);
    e.jobs[0].count=1;e.jobs[0].tiles_size=15;save();assert(ntm_tee_objects_head(&e,&d,tiles,sprites,palettes)==-1);unchanged();
    start(2);tail();e.jobs[1].tile=512;save();assert(ntm_tee_objects_head(&e,&d,tiles,sprites,palettes)==-1);unchanged();
    for(i=0;i<8192;i++)assert(tiles[i]==7); /* Entire queue validated first. */
    start(1);c[0].byte30=1;save();assert(ntm_tee_object_remove(&d,c)==-1);unchanged();
    start(1);tail();saved_e=e;memcpy(saved_c,c,sizeof c);assert(!ntm_tee_objects_reset(&d,c));
    assert(!memcmp(&e,&saved_e,sizeof e));
    for(i=0;i<32;i++){saved_c[i].tag=65535;saved_c[i].id=(uint8_t)i;}assert(!memcmp(c,saved_c,sizeof c));
    for(i=0;i<160;i++) {
        int valid=i==60 || i==148 || i==152 || i==156;
        start(0);c[11].tag=24;c[11].width=56;c[11].velocity=(uint16_t)i;c[11].tile=176;
        if(valid){
            other.animations[i/4]=other.animations[3];
            assert(!ntm_tee_object_register(&d,c+11));tail();head();
            other.animations[i/4].data=0;bad_tail();
        }else assert(ntm_tee_object_register(&d,c+11)==-1);
    }
    puts("PASS tee objects:65536 flags/delay pairs,131072 clipping positions,32-object order/removal,65 queue occupancies,component state,resource identity,malformed atomic rejection");return 0;
}
