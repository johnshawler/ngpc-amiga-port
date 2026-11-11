/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "fire_stream.h"
#include "aim_unpack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL line %u: %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct {uint8_t *bytes;size_t size,at,limit,fail_at;unsigned pass,fail_pass,rewind_fail,change_pass,oversized;} memory;
static uint32_t hash(const uint8_t *p,size_t n){uint32_t h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
static void be(uint8_t *p,uint32_t n){p[0]=(uint8_t)(n>>24);p[1]=(uint8_t)(n>>16);p[2]=(uint8_t)(n>>8);p[3]=(uint8_t)n;}
static long read_memory(void *ctx,uint8_t *p,size_t n)
{
 memory *m=ctx;
 if(m->oversized)return (long)n+1;
 if(m->pass==m->fail_pass && m->at>=m->fail_at)return -1;
 if(n>m->size-m->at)n=m->size-m->at;
 if(m->limit && n>m->limit)n=m->limit;
 memcpy(p,m->bytes+m->at,n);
 if(m->pass==m->change_pass && m->at<=21 && m->at+n>21)p[21-m->at]^=1;
 m->at+=n;return (long)n;
}
static int rewind_memory(void *ctx){memory *m=ctx;m->pass++;if(m->rewind_fail==m->pass)return -1;m->at=0;return 0;}
static int run(memory *m,const fire_pack_spec *spec,uint8_t *out,size_t n)
{fire_stream io={m,read_memory,rewind_memory};m->pass=0;return fire_stream_unpack(&io,spec,out,n);}
static void guard(const uint8_t *p,unsigned n,uint8_t expected){unsigned i;for(i=0;i<n;i++)CHECK(p[i]==expected);}
int main(int argc,char **argv)
{
 uint8_t good[26]={'N','T','M','F','P','K','1',0},bad[27],out[40],before[40];unsigned i,j;
 fire_pack_spec spec={6,6,0};memory m;
 /* literal ABC, then length3/distance3 => ABCABC. */
 good[20]=2;good[21]='A';good[22]='B';good[23]='C';good[24]=128;good[25]=0;
 /* Need the second distance byte: packed length7. */
 {uint8_t sample[27];memcpy(sample,good,26);sample[26]=3;spec.packed_bytes=7;spec.checksum=hash(sample+20,7);
 be(sample+8,spec.raw_bytes);be(sample+12,spec.packed_bytes);be(sample+16,spec.checksum);
 for(i=0;i<=8;i++){
  memset(&m,0,sizeof m);m.bytes=sample;m.size=27;m.limit=i;memset(out,0xa5,sizeof out);
  CHECK(run(&m,&spec,out+4,6)==0);CHECK(!memcmp(out+4,"ABCABC",6));guard(out,4,0xa5);guard(out+10,30,0xa5);
 }
 for(i=0;i<27;i++){
  memset(&m,0,sizeof m);m.bytes=sample;m.size=i;memset(out,0xa5,sizeof out);memcpy(before,out,sizeof out);
  CHECK(run(&m,&spec,out+4,6)<0);CHECK(!memcmp(out,before,sizeof out));
 }
 for(i=0;i<27;i++)for(j=0;j<8;j++){
  memcpy(bad,sample,27);bad[i]^=(uint8_t)(1u<<j);memset(&m,0,sizeof m);m.bytes=bad;m.size=27;memset(out,0xa5,sizeof out);
  CHECK(run(&m,&spec,out+4,6)<0);guard(out,sizeof out,0xa5);
 }
 for(i=1;i<=2;i++){
  memset(&m,0,sizeof m);m.bytes=sample;m.size=27;m.rewind_fail=i;memset(out,0xa5,sizeof out);
  CHECK(run(&m,&spec,out+4,6)==FIRE_PACK_IO);guard(out+4,6,i==1?0xa5:0);
 }
 for(i=1;i<=2;i++)for(j=0;j<28;j++){
  memset(&m,0,sizeof m);m.bytes=sample;m.size=27;m.fail_pass=i;m.fail_at=j;m.limit=1;memset(out,0xa5,sizeof out);
  CHECK(run(&m,&spec,out+4,6)==FIRE_PACK_IO);guard(out,4,0xa5);guard(out+4,6,i==1?0xa5:0);guard(out+10,30,0xa5);
 }
 memset(&m,0,sizeof m);m.bytes=sample;m.size=27;m.change_pass=2;memset(out,0xa5,sizeof out);
 CHECK(run(&m,&spec,out+4,6)==FIRE_PACK_CHECKSUM);guard(out+4,6,0);
 memset(&m,0,sizeof m);m.bytes=sample;m.size=27;m.oversized=1;memset(out,0xa5,sizeof out);
 CHECK(run(&m,&spec,out+4,6)==FIRE_PACK_FORMAT);guard(out,sizeof out,0xa5);
 CHECK(run(&m,&spec,out,5)==FIRE_PACK_ARGUMENT);CHECK(fire_stream_unpack(NULL,&spec,out,6)==FIRE_PACK_ARGUMENT);
 /* Well-checksummed, structurally invalid streams must still reject. */
 for(i=0;i<4;i++){
  fire_pack_spec invalid=spec;memcpy(bad,sample,27);
  if(i==0)bad[26]=0;else if(i==1)bad[26]=4;else if(i==2)bad[24]=129;else bad[20]=127;
  invalid.checksum=hash(bad+20,7);be(bad+16,invalid.checksum);
  memset(&m,0,sizeof m);m.bytes=bad;m.size=27;memset(out,0xa5,sizeof out);
  CHECK(run(&m,&invalid,out+4,6)==FIRE_PACK_FORMAT);guard(out,sizeof out,0xa5);
 }
 }
 {
  uint32_t state=0x81237651u;uint8_t fuzz[85],expected[128],decoded[128];unsigned case_index;
  for(case_index=0;case_index<20000;case_index++){
   fire_pack_spec f;size_t count,out_size;int legacy,result;
   state=state*1664525u+1013904223u;count=1+(state%64);state=state*1664525u+1013904223u;out_size=1+(state%128);
   memcpy(fuzz,"NTMFPK1",8);
   for(i=0;i<count;i++){state=state*1664525u+1013904223u;fuzz[20+i]=(uint8_t)(state>>24);}
   f.raw_bytes=(uint32_t)out_size;f.packed_bytes=(uint32_t)count;f.checksum=hash(fuzz+20,count);
   be(fuzz+8,f.raw_bytes);be(fuzz+12,f.packed_bytes);be(fuzz+16,f.checksum);
   memset(&m,0,sizeof m);m.bytes=fuzz;m.size=20+count;m.limit=1+case_index%17;memset(decoded,0xa5,sizeof decoded);
   result=run(&m,&f,decoded,out_size);legacy=aim_unpack(fuzz+20,count,expected,out_size);
   CHECK((result==0)==(legacy==0));if(!result)CHECK(!memcmp(decoded,expected,out_size));else guard(decoded,sizeof decoded,0xa5);
   /* A correctly decoded payload with any trailing byte is still invalid. */
   if(!result){m.size++;memset(decoded,0xa5,sizeof decoded);CHECK(run(&m,&f,decoded,out_size)==FIRE_PACK_FORMAT);guard(decoded,sizeof decoded,0xa5);}
  }
 }
 if(argc==3){FILE *f;long n,raw_n;uint8_t *data,*raw,*decoded;fire_pack_spec real;fire_stream io;
  f=fopen(argv[1],"rb");CHECK(f!=NULL);CHECK(!fseek(f,0,SEEK_END));n=ftell(f);CHECK(n>=20);rewind(f);data=malloc((size_t)n);CHECK(data!=NULL);CHECK(fread(data,1,(size_t)n,f)==(size_t)n);fclose(f);
  f=fopen(argv[2],"rb");CHECK(f!=NULL);CHECK(!fseek(f,0,SEEK_END));raw_n=ftell(f);CHECK(raw_n>0);rewind(f);raw=malloc((size_t)raw_n);decoded=malloc((size_t)raw_n);CHECK(raw&&decoded);CHECK(fread(raw,1,(size_t)raw_n,f)==(size_t)raw_n);fclose(f);
  real.raw_bytes=(uint32_t)raw_n;real.packed_bytes=(uint32_t)n-20;real.checksum=hash(data+20,(size_t)n-20);
  memset(&m,0,sizeof m);m.bytes=data;m.size=(size_t)n;io.context=&m;io.read=read_memory;io.rewind=rewind_memory;
  CHECK(!fire_stream_unpack(&io,&real,decoded,(size_t)raw_n));CHECK(!memcmp(decoded,raw,(size_t)raw_n));free(data);free(raw);free(decoded);
 }
 printf("fire_stream: %u checks passed\n",checks);return 0;
}
