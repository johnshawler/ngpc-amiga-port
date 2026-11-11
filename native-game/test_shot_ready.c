/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_ready.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int delay(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static int ready(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
int main(void)
{
    unsigned n,b,p,cases=0;ntm_task t;
    for(n=0;n<65536;n++)for(b=0;b<2;b++)for(p=0;p<2;p++){
        uint16_t next=(uint16_t)(n-1u);int crossed=!!(next&32768u);
        memset(&t,0,sizeof(t));t.callback=delay;t.delay=(uint16_t)n;
        assert(ntm_shot_ready_tick(&t,delay,ready,(uint8_t)b,(uint8_t)(p*16))==(crossed && !b && p?2:0));
        assert(t.delay==next && t.callback==(crossed?ready:delay));cases++;
    }
    for(n=0;n<256;n++)for(p=0;p<256;p++){
        int reset=(n&48u)==48u && (p&64u);
        assert(ntm_shot_ready_input((uint8_t)n,(uint8_t)p,0)==((p&64u)?2:0));
        assert(ntm_shot_ready_input((uint8_t)n,(uint8_t)p,255)==(reset?2:0));
        assert(ntm_shot_ready_input((uint8_t)n,(uint8_t)p,1)==2);
    }
    puts("shot ready:262144 countdown/gate and196608 bounded input cases PASS");
    assert(cases==262144);return 0;
}
