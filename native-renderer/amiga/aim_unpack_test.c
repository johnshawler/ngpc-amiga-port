/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "aim_unpack.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    uint8_t out[512],before[512],source[]={2,'a','b','c',128,0,3,255,0,1};
    size_t i,n;
    assert(!aim_unpack(0,0,0,0));assert(aim_unpack(0,1,out,1)<0);
    assert(!aim_unpack(source,sizeof source,out,136));
    assert(!memcmp(out,"abcabc",6));for(i=6;i<136;i++)assert(out[i]=='c');
    memset(before,0xa5,sizeof before);
    for(n=0;n<sizeof source;n++){
        memcpy(out,before,sizeof out);assert(aim_unpack(source,n,out,136)<0);assert(!memcmp(out,before,sizeof out));
    }
    for(n=0;n<sizeof out;n++)if(n!=136){
        memcpy(out,before,sizeof out);assert(aim_unpack(source,sizeof source,out,n)<0);assert(!memcmp(out,before,sizeof out));
    }
    {uint8_t bad[][3]={{128,0,0},{128,0,1},{255,255,255}};
     for(i=0;i<3;i++){assert(aim_unpack(bad[i],3,out,130)<0);assert(!memcmp(out,before,sizeof out));}}
    assert(aim_unpack(source,sizeof source,source,136)<0);
    {uint32_t state=0x209830;uint8_t random[32];unsigned trial,k;int result;
     for(trial=0;trial<20000;trial++){
      state=state*1664525u+1013904223u;n=state%33;
      for(k=0;k<n;k++){state=state*1664525u+1013904223u;random[k]=(uint8_t)(state>>24);}
      state=state*1664525u+1013904223u;i=state%510;
      memcpy(out,before,sizeof out);result=aim_unpack(random,n,out+1,i);
      assert(out[0]==165 && out[i+1]==165);
      if(result)assert(!memcmp(out,before,sizeof out));
     }}
    puts("aim_unpack: literal, overlapping match, exact bounds, malformed and no-write failure PASS");return 0;
}
