#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compare unchanged20a38a/20a473 with native HUD, including queue saturation."""
from shot_hud_case_common import *
import os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_HUD_LIBRARY',str(ROOT/'build/shot-hud-check/native.so')))
lib.ntm_shot_hud_initialize.argtypes=[C.POINTER(HudContext),C.POINTER(HudResources),U32,U8,U8,U8]
lib.ntm_shot_hud_clear.argtypes=[P]
R=random.Random(0x20a38a);cases=0
for wind in range(16):
 for direction in range(16):
  for occupied in (0,31,32):
   setup();put(0x4000,R.randbytes(16384));maps=blob(R.randbytes(4096));tiles=blob(R.randbytes(8192));put(0x9000,bytes(maps));put(0xa000,bytes(tiles))
   distance=R.randrange(1<<32);store(0x4268,distance,4);store(0x426e,wind,1);store(0x426f,direction,1);store(0x4276,4,1);store(0x4d84,8*occupied,2)
   q=Screen();q.stamp_bytes=8*occupied;text=U16(load(0x400f,2));scratch=U32(load(0x5bcb,4));saved=blob(data(0x451f,4))
   c=HudContext(addr(q),addr(maps),addr(tiles),addr(text),addr(scratch),addr(saved));expected=bytearray(ram[:16384])
   assert lib.ntm_shot_hud_initialize(C.byref(c),C.byref(hud_resources),distance,wind,direction,4)==0
   expected[0x51f:0x523]=bytes(saved);write(expected,0x400f,text.value,2);write(expected,0x5bcb,scratch.value,4);write(expected,0x4d84,q.stamp_bytes,2)
   for j in range(occupied,q.stamp_bytes//8):
    st=q.stamps[j];source=next(src for src,a in tee_assets if addr(a.data.contents)==addr(st.asset.data.contents))
    write(expected,0x4d86+8*j,source,4);write(expected,0x4d8a+8*j,0x9000+st.offset,2);write(expected,0x4d8c+8*j,st.tile,2)
   reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x20a38a);minimum=0x6be0
   for instruction in range(100000):
    if get(8)==0xef00:break
    step(0);minimum=min(minimum,get(7))
   else:raise AssertionError(('budget',hex(get(8))))
   actual=bytes(ram[:16384]);assert minimum>=0x6b80
   expected[minimum-0x4000:0x2be4]=actual[minimum-0x4000:0x2be4]
   assert actual==expected,('ram',wind,direction,occupied,[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:20])
   assert data(0x9000,4096)==bytes(maps),('maps',wind,direction,occupied)
   assert data(0xa000,8192)==bytes(tiles),('tiles',wind,direction,occupied)
   assert lib.ntm_shot_hud_clear(maps)==0
   reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x20a473)
   for instruction in range(1000):
    if get(8)==0xef00:break
    step(0)
   else:raise AssertionError('clear budget')
   assert data(0x9000,4096)==bytes(maps)
   cases+=1
print(json.dumps({'result':'PASS','cases':cases,'scope':'all16 wind values/directions with empty/one-free/full queues; complete nonstack RAM, maps, tiles, and subsequent clear'}))
