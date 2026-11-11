/* SPDX-License-Identifier: GPL-2.0-or-later. Isolated test controls only.
 * No original instructions or assets; never linked into the Amiga runtime. */
#include <string.h>
extern uint8_t CPUExRAM[16384];
__attribute__((visibility("default"))) void retro_reference_life_reset(void)
{memset(CPUExRAM,0,16384);memset(gprBank,0,sizeof gprBank);memset(gpr,0,sizeof gpr);sr=0;statusRFP=0;f_dash=0;rErr=0;}
__attribute__((visibility("default"))) void retro_reference_life_reg(unsigned n,unsigned v)
{if(n<8)regL(n)=v;else if(n==8)pc=v;else if(n==9)sr=v;}
__attribute__((visibility("default"))) unsigned retro_reference_life_getreg(unsigned n)
{return n<8?regL(n):n==8?pc:sr;}
__attribute__((visibility("default"))) void retro_reference_life_store(unsigned a,unsigned v,unsigned n)
{if(n==1)storeB(a,v);else if(n==2)storeW(a,v);else storeL(a,v);}
__attribute__((visibility("default"))) unsigned retro_reference_life_load(unsigned a,unsigned n)
{return n==1?loadB(a):n==2?loadW(a):loadL(a);}
__attribute__((visibility("default"))) void retro_reference_life_step(unsigned unused)
{(void)unused;TLCS900h_interpret();}
