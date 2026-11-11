/* SPDX-License-Identifier: GPL-2.0-or-later. Native state-level implementation. */
#include "input_state.h"
int ntm_input_update(uint8_t s[16],uint8_t sample)
{
    unsigned i,held,periodic=0,compared=0;
    if(!s)return -1;
    held=s[0]&sample;s[4]=s[0];s[1]=(uint8_t)(sample&~s[0]);s[0]=sample;
    for(i=0;i<8;i++) {
        unsigned flag=1u<<i;
        if(!(held&flag))s[8+i]=0;
        else {
            unsigned count=(s[8+i]+1u)&255u,masked;
            if(!count)count=128;
            s[8+i]=(uint8_t)count;masked=count&s[5];
            if(!masked)periodic|=flag;
            /* Preserve the original SIGNED compare of the MASKED count.
             * Biasing both bytes avoids implementation-defined signed casts. */
            if((masked^128u)>=(s[6]^128u))compared|=flag;
        }
    }
    s[2]=(uint8_t)periodic;s[3]=(uint8_t)compared;return 0;
}
