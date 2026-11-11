#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Private original comparison; no cartridge bytes or captured assets embedded.
from cursor_native_case_common import *
if __name__=='__main__':
 count=0
 for case in range(128):
  init();put(0xa000,R.randbytes(8192));put(0x61bb,R.randbytes(0x500))
  store(0x453f,R.choice((8,56,104,0,65535,65530,160,166,256,32768)),2);store(0x4541,R.choice((28,0,65535,65530,152,158,256)),2)
  store(0x4549,R.randrange(16)|(128 if case&4 else 0),1);store(0x454a,case%4,1);store(0x454b,128|((case%4)<<2)|(32 if case&8 else 0),1)
  store(0x66bc,1+case%64,1);store(0x66bb,0,1)
  m=Native()
  for tick in range(51):
   if tick in (8,17,24):
    x=R.choice((8,56,104,65535,160));store(0x453f,x,2);m.w(0x453f,x,2)
   if tick==9:store(0x400d,load(0x400d,1)|32,1);m.global_.value|=32
   if tick==12:store(0x4545,4,2);m.c.velocity=4;store(0x454b,load(0x454b,1)|128,1);m.c.flags|=128
   if tick==36:
    if case&1:execute(0x2006cf);m.display_reset();m.compare((case,tick,'reset'))
    else:store(0x454b,load(0x454b,1)|16,1);m.c.flags|=16
   m.tail();execute(0x201192);execute(0x200c7b);m.compare((case,tick,'tail'))
   m.head();execute(0x20067c);execute(0x200aa6);m.compare((case,tick,'head'));count+=1
 print(json.dumps({'cases':128,'tail_head_rounds':count,'c_source_sha256':hashlib.sha256((ROOT/'native-game/course_cursor.c').read_bytes()).hexdigest(),'comparisons':'all nonstack RAM,8192 tile bytes,256 sprite bytes,64 palette indices'}))
