/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "aim_unpack.h"
int aim_unpack(const uint8_t *source,size_t input,uint8_t *target,size_t output)
{
    size_t at=0,out=0,n,d,i;unsigned code;
    if((input && !source)||(output && !target))return -1;
    /* Subtractions avoid overflow and make hostile streams bounded. */
    if(input && output && ((uintptr_t)source<=(uintptr_t)target ?
       (uintptr_t)target-(uintptr_t)source<input :
       (uintptr_t)source-(uintptr_t)target<output))return -1;
    while(at<input){
        code=source[at++];n=(code&127)+(code&128?3:1);
        if(n>output-out)return -1;
        if(code&128){
            if(input-at<2)return -1;
            d=((size_t)source[at]<<8)|source[at+1];at+=2;
            if(!d || d>out)return -1;
        }else{if(n>input-at)return -1;at+=n;}
        out+=n;
    }
    if(out!=output)return -1;
    at=out=0;
    while(at<input){
        code=source[at++];n=(code&127)+(code&128?3:1);
        if(code&128){
            d=((size_t)source[at]<<8)|source[at+1];at+=2;
            for(i=0;i<n;i++){target[out]=target[out-d];out++;}
        }else{for(i=0;i<n;i++)target[out++]=source[at++];}
    }
    return 0;
}
