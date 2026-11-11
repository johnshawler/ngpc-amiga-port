/* SPDX-License-Identifier: GPL-2.0-or-later
 * Read-only observer around unchanged original execution functions. The loop
 * matches libretro Emulate; no original instructions or RAM are patched. */
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
typedef struct {int (*cpu)(void);bool (*timers)(void *,int);int (*z80)(void);int (*flush)(int16_t *,int);uint32_t *pc;int32_t *z80time,*soundtime;uint8_t *gpu,*ram;} Api;
int frame(Api *a,void *surface,unsigned frame,FILE *out){
 unsigned count=0,offset=14+4096+8192+256+64+512+18;bool done=false;int16_t sound[32768];
 *a->soundtime=0;
 while(!done){
  uint32_t pc=*a->pc;unsigned line=a->gpu[offset],raw=a->gpu[offset+1],latch=a->gpu[4],upper=a->ram[0x443],lower=a->ram[0x441];
  int cycles=(uint8_t)a->cpu();
  if(a->ram[0x443]!=upper || a->ram[0x441]!=lower || pc==0x20042e || pc==0x200453 || pc==0x20c620)
   fprintf(out,"{\"frame\":%u,\"pc\":%u,\"line\":%u,\"upper_before\":%u,\"upper_after\":%u,\"lower\":%u,\"raw\":%u,\"latched\":%u,\"cycles\":%d}\n",frame,pc,line,upper,a->ram[0x443],lower,raw,latch,cycles);
  done|=a->timers(surface,cycles);*a->z80time+=cycles;
  if(a->gpu[offset]!=line && a->ram[0x2fd4]==0x20 && a->ram[0x2fd5]==0xc6)
   fprintf(out,"{\"frame\":%u,\"drawn_line\":%u,\"row_scroll\":%u,\"upper\":%u,\"raw\":%u}\n",frame,line,a->gpu[4],a->ram[0x443],a->gpu[offset+1]);
  while(*a->z80time>0){int z=a->z80();if(z<0){*a->z80time=0;break;}*a->z80time-=z<<1;}
  if(++count>1000000)return -1;
 }
 a->flush(sound,32768);return (int)count;
}
