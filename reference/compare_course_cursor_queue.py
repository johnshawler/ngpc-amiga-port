#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Private original comparison; no cartridge bytes or captured assets embedded.
from cursor_native_case_common import *
count=0
for case in range(130):
 init();mode=case%2*4;store(0x4545,mode,2);store(0x454b,128|(64 if case%3==0 else 0),1)
 n=case%65;src=shapes+(load(anim+20,2)&0x7fff)+56
 for j in range(n):store(0x6000+6*j,28,1);store(0x6001+6*j,450&255,1);store(0x6002+6*j,src|(1<<24),4)
 store(0x6182,n,1);store(0x6180,n*6,2)
 st=case%64;store(0x66bb,st,1);store(0x66bc,64-st,1)
 m=Native()
 for tick in range(3):
  m.tail();execute(0x201192);execute(0x200c7b);m.compare((case,tick,'tail'))
  m.head();execute(0x20067c);execute(0x200aa6);m.compare((case,tick,'head'));count+=1
print(json.dumps({'cases':130,'tail_head_rounds':count,'includes':'all existing tile queue occupancies0..64 including full drop; normal/accept animation; timer pause bit64; sprite start0..63 and bounded capacity'}))
