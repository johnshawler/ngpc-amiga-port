/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "course_cursor.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Deliberately synthetic data, not cartridge-derived title artwork. */
static uint8_t animation[]={1,192,4,0,0,128,0,0};
static uint8_t shapes[4*61+4],source[62*16],tiles[8192],sprites[256],palettes[64];
static ntm_course_cursor_engine engine;
static ntm_display_reset_state display;
static ntm_course_cursor object;
static ntm_cursor_assets assets={{{animation,sizeof animation,1},{0,0,0}},shapes,source,sizeof shapes,sizeof source};
static uint16_t scroll[2];static uint8_t global;
static void start(unsigned index)
{
    memset(&engine,0,sizeof engine);memset(&object,0,sizeof object);
    assert(!ntm_display_reset(&display));display.zero_flags[8]=1;display.zero_words[0]=2;
    display.list[0]=0xff00;display.object_tags[0]=38;
    object.tag=38;object.width=(uint16_t)index;object.second=255;object.y=48;
    object.byte15=3;object.flags=128;object.tile=450;object.link=65535;global=16;
}
int main(void)
{
    unsigned i,index,flags,delay,cases=0;
    for(i=0;i<61;i++) {
        shapes[2*i]=(uint8_t)(i==60?128:0);shapes[2*i+1]=(uint8_t)((i/10)*16+i%10);
        shapes[2*61+4+2*i]=(uint8_t)(i+1);
    }
    for(i=0;i<sizeof source;i++)source[i]=(uint8_t)(i*13+7);
    for(index=20;index<=28;index+=4) {
        start(index);assert(!ntm_course_cursor_tail(&engine,&object,&display,40,scroll,&global,&assets));
        assert(engine.frame==0 && engine.delay==1 && object.flags==0 && object.byte30==0);
        assert(display.zero_flags[10]==1 && display.zero_flags[13]==61);
        assert(!ntm_course_cursor_head(&engine,&display,tiles,sprites,palettes,&assets));
        assert(!memcmp(tiles+450*16,source+16,61*16));
        assert(!ntm_course_cursor_tail(&engine,&object,&display,40,scroll,&global,&assets));
        assert(engine.frame==0 && engine.delay==0 && object.flags==66 && object.byte30==64);
        assert(display.zero_flags[10]==1); /* Expiry refreshes the held frame. */
        assert(!ntm_course_cursor_head(&engine,&display,tiles,sprites,palettes,&assets));
        for(i=0;i<100;i++) {
            assert(!ntm_course_cursor_tail(&engine,&object,&display,40,scroll,&global,&assets));
            assert(!display.zero_flags[10] && engine.delay==0 && object.flags==66 && object.byte30==64);
        }
        /* Full byte-domain timer/flag edges, including frozen restart and the
         * zero-delay wrap. The original remains the independent oracle. */
        for(flags=0;flags<256;flags++)for(delay=0;delay<256;delay++) {
            unsigned expected_delay=delay,status=96,expected_flags=flags;
            start(index);object.flags=(uint8_t)flags;object.byte30=96;engine.delay=(uint8_t)delay;
            assert(!ntm_course_cursor_tail(&engine,&object,&display,40,scroll,&global,&assets));
            if(flags&16) {
                assert(object.tag==65535 && !display.zero_flags[8] && engine.delay==delay);
            }else {
                if(flags&128){expected_flags&=127;status=0;expected_delay=2;}
                expected_flags|=2;
                if(!(flags&64)) {
                    expected_flags&=~2u;expected_delay=(expected_delay-1)&255;
                    if(!expected_delay){expected_flags|=66;status|=64;}
                }
                expected_flags=(expected_flags&~1u)|((flags&32)?1u:0u);
                assert(engine.frame==0 && engine.delay==expected_delay);
                assert(object.flags==expected_flags && object.byte30==status);
            }
            cases++;
        }
    }
    start(16);assert(ntm_course_cursor_tail(&engine,&object,&display,40,scroll,&global,&assets)==-1);
    start(20);object.velocity=4;assert(ntm_course_cursor_tail(&engine,&object,&display,40,scroll,&global,&assets)==-1);
    start(20);object.tag=46;object.width=16;assert(ntm_course_cursor_tail(&engine,&object,&display,40,scroll,&global,&assets)==-1);
    printf("PASS native title cursor: %u flag/delay states, three resource indices, stop/restart/freeze, deferred61-tile upload and bounded format rejection\n",cases);
    return 0;
}
