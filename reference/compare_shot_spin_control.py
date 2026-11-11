#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original20b62d complete spin adjustment, timer, redraw and queue oracle."""
from shot_spin_case_common import *
import os,random
lib=C.CDLL(os.environ['NTM_SHOT_SPIN_LIBRARY'])
lib.ntm_shot_spin_control.argtypes=[C.POINTER(Spin),C.POINTER(SceneObjectContext),C.POINTER(SpinDrawResources),U8,U8,U8,P,P]
lib.ntm_course_screen_stamp_queue.argtypes=[C.POINTER(Screen),P,P]
R=random.Random(0x20b64a);cases=0;sounds=[]
SOUND=C.CFUNCTYPE(None,P,U8,U16,U16)
@SOUND
def sound(p,service,command,parameter):sounds.append((service,command,parameter))
source_by_asset={C.cast(a.data,P).value:src for src,a in tee_assets}
def check(spin=4,pressed=0,timer=0,gate=0,life=1,queued=0,minimum=0,maximum=8,cached=None):
 global cases
 setup();put(0x4000,R.randbytes(16384));sounds.clear()
 h=AM.State.from_buffer_copy(R.randbytes(C.sizeof(AM.State)));p=AM.Player(0,cases%2,1-cases%2,0)
 h.players[p.active_player][0x31]=0xbf;h.players[p.active_player][0x32]=0x40
 h.hole[0x76]=spin;h.hole[0x77]=timer
 s=Spin();s.wait=0x20b62d;s.minimum=minimum;s.maximum=maximum;s.current=spin if cached is None else cached;s.phase=1
 q=Screen();q.stamp_bytes=8*queued
 for j in range(32):q.stamps[j]=Stamp(spin_draw_resources.indicators[0],0x448,0xd6)
 maps=blob(R.randbytes(4096));tiles=blob(R.randbytes(8192));saved=blob(R.randbytes(32));ic=Context();ic.screen=addr(q);ic.maps=addr(maps)
 tc=TeeContext();tc.intro=addr(ic);tc.sound_request=C.cast(sound,P).value
 ctx=SceneObjectContext(addr(tc),addr(h),addr(p),None,addr(saved));b=bytearray(ram[:16384])
 def serial():
  b[0x149:0x189]=bytes(h.players[0]);b[0x189:0x1c9]=bytes(h.players[1]);b[0x200:0x300]=bytes(h.hole)
  write(b,0x41c9,0x4149+64*p.active_player,4);write(b,0x41d1,life);write(b,0x41e1,gate);write(b,0x40c0,pressed)
  write(b,0x6000,0x20b62d,4);write(b,0x6016,s.minimum);write(b,0x6017,s.maximum);write(b,0x6018,s.current);b[0x4ff:0x51f]=bytes(saved)
  write(b,0x4d84,q.stamp_bytes,2)
  for j in range(q.stamp_bytes//8):
   st=q.stamps[j];write(b,0x4d86+8*j,source_by_asset[C.cast(st.asset.data,P).value],4);write(b,0x4d8a+8*j,0x9000+st.offset,2);write(b,0x4d8c+8*j,st.tile,2)
 def run(pc):
  osounds=[];reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
  for _ in range(10000):
   pc=get(8)
   if pc==0xef00:break
   if pc==0x2501d1:
    osounds.append((2,get(0)&65535,get(1)&255));sp=get(7);reg(8,load(sp,4));reg(7,sp+4);continue
   step(0);low=min(low,get(7))
  else:raise AssertionError(('budget',hex(get(8))))
  return low,osounds
 serial();put(0x4000,b);put(0x9000,maps);put(0xa000,tiles)
 assert lib.ntm_shot_spin_control(C.byref(s),C.byref(ctx),C.byref(spin_draw_resources),life,gate,pressed,None,None)==0
 serial();low,osounds=run(0x20b62d);actual=bytes(ram[:16384]);b[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 label=(spin,pressed,timer,gate,life,queued,minimum,maximum,cached)
 assert actual==b,(label,[(hex(0x4000+i),a,v) for i,(a,v) in enumerate(zip(actual,b)) if a!=v][:20])
 assert sounds==osounds,(label,sounds,osounds)
 assert bytes(maps)==data(0x9000,4096),(label,'maps')
 assert bytes(tiles)==data(0xa000,8192),(label,'tiles')
 assert lib.ntm_course_screen_stamp_queue(C.byref(q),tiles,maps)==0
 serial();low,osounds=run(0x20054f);actual=bytes(ram[:16384]);b[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==b,(label,'drain RAM')
 assert bytes(maps)==data(0x9000,4096),(label,'drain maps')
 assert bytes(tiles)==data(0xa000,8192),(label,'drain tiles');cases+=1
for spin in range(9):
 for pressed in (0,4,8,12):
  for queued in (0,29,30,31,32):check(spin,pressed,queued=queued)
for timer in range(256):
 for pressed in (0,4,8):check(timer=timer,pressed=pressed)
for life in range(256):
 if life!=3:
  for gate in (0,1,255):check(life=life,gate=gate,pressed=4,timer=1)
for minimum in range(9):
 for maximum in range(minimum,9):
  for spin in (minimum,maximum):
   for pressed in (4,8):check(spin,pressed,minimum=minimum,maximum=maximum)
for pressed in range(256):check(pressed=pressed)
for spin in range(9):
 for queued in (0,30,31,32):check(spin=spin,cached=255,queued=queued,timer=1)
print(json.dumps({'cases':cases,'scope':'full nonstack RAM, exact queued asset identities, all timers and input bytes, map restoration, sound requests and queue drain pixels; unchanged original except external audio service interception'}))
