#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exact2098ee/2098ff ready gate and bounded200472 admission, no ROM bytes."""
from tee_setup_case_common import *
import os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_READY_LIBRARY',str(ROOT/'build/shot-ready-check/native.so')))
lib.ntm_shot_ready_tick.argtypes=[C.POINTER(Task),P,P,U8,U8]
lib.ntm_shot_ready_input.argtypes=[U8,U8,U8]
r=random.Random(0x2098ee);ticks=inputs=0

def run(pc,stop,expected,label):
 reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
 for _ in range(100):
  if get(8) in (0xef00,stop):break
  step(0);low=min(low,get(7))
 else:raise AssertionError(('budget',label,hex(get(8))))
 actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==expected,(label,[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:10])
 return get(8)

bindings=0
lib.ntm_shot_ready_bind_players.argtypes=[P]
for _ in range(256):
 setup();put(0x4000,r.randbytes(16384));store(0x4148,0,1);store(0x4144,0,1)
 players=(U8*128).from_buffer_copy(data(0x4149,128));expected=bytearray(ram[:16384])
 assert lib.ntm_shot_ready_bind_players(players)==0
 expected[0x149:0x1c9]=bytes(players)
 assert run(0x20d68c,0,expected,('bindings',_))==0xef00;bindings+=1
for pc in (0x2098ee,0x2098ff):
 for count in [0,1,2,3,4,32767,32768,32769,65535]+[r.randrange(65536) for _ in range(256)]:
  for busy in (0,1,255):
   for pressed in (0,16,32,255):
    setup();put(0x4000,r.randbytes(16384));store(0x4416,busy,1);store(0x41c9,0x4149,4);store(0x417a,0x6100,2);store(0x6101,pressed,1)
    t=Task();t.callback=pc;t.delay=count;store(0x6000,pc,4);store(0x6014,count,2);expected=bytearray(ram[:16384])
    result=lib.ntm_shot_ready_tick(C.byref(t),0x2098ee,0x2098ff,busy,pressed)
    write(expected,0x6000,t.callback,4);write(expected,0x6014,t.delay,2)
    end=run(pc,0x209919,expected,(pc,count,busy,pressed));assert result==(2 if end==0x209919 else 0)
    ticks+=1
for overlay in (0,128,255):
 for held in range(256):
  for pressed in (0,16,32,64,80,112,255):
   setup();put(0x4000,r.randbytes(16384));store(0x413b,overlay,1);store(0x413c,0x204786,4);store(0x4140,0x20485a,4)
   store(0x4148,0,1);store(0x40bf,held,1);store(0x40c0,pressed,1);expected=bytearray(ram[:16384])
   result=lib.ntm_shot_ready_input(held,pressed,overlay)
   stop=0x200049 if held&48==48 and pressed&64 else 0x204786
   end=run(0x200472,stop,expected,(overlay,held,pressed))
   assert result==(0 if end==0xef00 else 2)
   if not result:assert get(9)&1==0
   inputs+=1
print(json.dumps({'player_bindings':bindings,'ready_ticks':ticks,'input_admission':inputs,'scope':'full nonstack RAM; fire stops before209919; information overlay/reset stops before handlers; no swing or overlay effects substituted'}))
