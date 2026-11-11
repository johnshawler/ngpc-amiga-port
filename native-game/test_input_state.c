/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "input_state.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int signed_byte(unsigned x){return x<128?(int)x:(int)x-256;}
static void oracle(uint8_t s[16],unsigned input)
{
    unsigned old=s[0],i;s[4]=(uint8_t)old;s[0]=(uint8_t)input;
    s[1]=(uint8_t)(input^(input&old));s[2]=s[3]=0;
    for(i=0;i<8;i++) {
        unsigned b=1u<<i;
        if((old&b) && (input&b)) {
            s[8+i]=s[8+i]==255?128:(uint8_t)(s[8+i]+1);
            if(!(s[8+i]&s[5]))s[2]|=(uint8_t)b;
            if(signed_byte(s[8+i]&s[5])>=signed_byte(s[6]))s[3]|=(uint8_t)b;
        }else s[8+i]=0;
    }
}
static void check(uint8_t s[16],unsigned input)
{
    uint8_t a[16],b[16];memcpy(a,s,16);memcpy(b,s,16);
    assert(!ntm_input_update(a,(uint8_t)input));oracle(b,input);assert(!memcmp(a,b,16));
}
int main(void)
{
    uint8_t s[16];unsigned a,b,c,i;unsigned long cases=0;
    for(i=0;i<16;i++)s[i]=(uint8_t)(i*17);
    s[5]=7;s[6]=32;
    for(a=0;a<256;a++)for(b=0;b<256;b++) {s[0]=(uint8_t)a;check(s,b);cases++;}
    s[0]=255;
    for(a=0;a<256;a++)for(b=0;b<256;b++)for(c=0;c<256;c++) {
        s[5]=(uint8_t)a;s[6]=(uint8_t)b;
        for(i=0;i<8;i++)s[8+i]=(uint8_t)(c+i*29);
        check(s,255);cases++;
    }
    assert(ntm_input_update(0,0)==-1);
    printf("PASS native input: %lu state/configuration cases\n",cases);return 0;
}
