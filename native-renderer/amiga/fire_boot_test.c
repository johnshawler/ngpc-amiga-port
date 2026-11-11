/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "fire_boot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,fail,opened,closed,libs,unlibs,allocated,errors,reads,rewinds,pass;
static size_t at;
static uint8_t file[22]={'N','T','M','F','P','K','1',0,0,0,0,1,0,0,0,2,0,0,0,0,0,65},display[170000];
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
void *fire_os_open_library(void){libs++;return fail==1?NULL:(void *)file;}
void fire_os_close_library(void *p){CHECK(p==file);unlibs++;}
long fire_os_open(const char *p){CHECK(!strcmp(p,opened?"intro.pool":"fire.pool"));opened++;at=0;pass=0;return (fail==2 && opened==1)||(fail==3 && opened==2)?0:(long)opened;}
long fire_os_close(long handle){CHECK(handle==1 || handle==2);closed++;return fail==4?0:1;}
long fire_os_read(long handle,uint8_t *p,size_t n){CHECK(handle==1 || handle==2);reads++;if(fail==5)return -1;if(fail==6 && pass==2)return -1;if(n>sizeof file-at)n=sizeof file-at;memcpy(p,file+at,n);at+=n;return (long)n;}
long fire_os_rewind(long handle){CHECK(handle==1 || handle==2);rewinds++;pass++;at=0;return fail==7?-1:0;}
long fire_os_write_error(const char *p,size_t n){CHECK(p&&n);errors++;return (long)n;}
uint32_t fire_os_avail(void){return allocated?200000:370000;}
uint8_t *fire_os_alloc(uint32_t n){CHECK(n==sizeof display);allocated++;return fail==8?NULL:display;}
void fire_os_free(uint8_t *p,uint32_t n){(void)p;(void)n;abort();}
int main(void){unsigned i;fire_pack_spec spec={1,2,0};uint8_t a=0,b=0;uint32_t memory[2];uint32_t h=(2166136261u^0)*16777619u;h=(h^65)*16777619u;spec.checksum=h;file[16]=(uint8_t)(h>>24);file[17]=(uint8_t)(h>>16);file[18]=(uint8_t)(h>>8);file[19]=(uint8_t)h;
 for(i=0;i<=8;i++){
  uint8_t *result;fail=i;opened=closed=libs=unlibs=allocated=errors=reads=rewinds=0;
  result=fire_boot_prepare(&spec,&a,1,&spec,&b,1,memory);
  CHECK((result!=NULL)==(i==0));CHECK(!fire_dos_base);CHECK(libs==1);CHECK(unlibs==(i==1?0:1));
  CHECK(closed==opened-((i==2||i==3)?1:0));CHECK(errors==(i==0||i==1?0:1));
  if(i==0){CHECK(a==65&&b==65);CHECK(memory[0]==370000&&memory[1]==200000);CHECK(allocated==1);}
  else if(i!=8)CHECK(allocated==0);
 }
 printf("fire_boot: %u checks passed, library/open/read/rewind/close/allocation failures cleaned before takeover\n",checks);return 0;
}
