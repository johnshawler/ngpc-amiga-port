#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Unchanged-original20ace7 full nonstack RAM and real deferred stamp oracle."""
from shot_club_case_common import *
import os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_CLUB_LIBRARY',str(ROOT/'build/shot-scene-check/native.so')))
lib.ntm_shot_club_initialize.argtypes=[C.POINTER(Club),C.POINTER(Task),C.POINTER(SceneObjectContext),C.POINTER(ClubResources)]
R=random.Random(0x20ace7);cases=0
for power in range(12):
 for terrain in range(15):
  for mode in range(3):
   setup();put(0x4000,R.randbytes(16384));q=Screen();q.stamp_bytes=(0,240,256)[mode]
   d=Display.from_buffer_copy(R.randbytes(C.sizeof(Display)));objects=(Cursor*32).from_buffer_copy(R.randbytes(C.sizeof(Cursor)*32));x=(U16*32)()
   h=AM.State.from_buffer_copy(R.randbytes(C.sizeof(AM.State)));p=AM.Player(0,mode%2,1-mode%2,0);aim=AM.Aim();aim.club=R.randrange(14)
   h.players[p.active_player][1]=power%6;h.players[p.active_player][0x33]=6 if power>=6 else 0;h.players[p.active_player][3]=0
   h.players[p.active_player][0x23]=R.randrange(256);h.hole[0x70]=terrain
   if mode==1:h.hole[0x46:0x48]=h.hole[0x2e:0x30];h.hole[0x4a:0x4c]=h.hole[0x30:0x32]
   words=(U16*2)(*[R.randrange(65536) for _ in range(2)]);maps=blob(bytes(4096));saved=blob(bytes(32))
   ic=Context();ic.display=addr(d);ic.screen=addr(q);ic.screen_words=addr(words);ic.maps=addr(maps)
   tc=TeeContext();tc.intro=addr(ic);tc.objects=addr(objects);tc.x=addr(x)
   ctx=SceneObjectContext(addr(tc),addr(h),addr(p),addr(aim),addr(saved));s=Club();s.wait=0x20ad1a;s.change_flag=R.randrange(256);task=Task();task.callback=0x20ace7
   def serial(b):
    serial_display(d,b);b[0x149:0x189]=bytes(h.players[0]);b[0x189:0x1c9]=bytes(h.players[1]);b[0x200:0x300]=bytes(h.hole)
    write(b,0x41c9,0x4149+64*p.active_player,4);write(b,0x41dc,aim.club);write(b,0x41dd,s.label);write(b,0x41de,s.delta);write(b,0x41e1,s.restore_delay);write(b,0x41da,s.change_flag)
    write(b,0x4d84,q.stamp_bytes,2);write(b,0x6000,task.callback,4);write(b,0x6016,s.maximum);write(b,0x6017,s.minimum)
    write(b,0x47c1,objects[10].y,2)
    for j in range(2):write(b,0x4d80+2*j,words[j],2)
   expected=bytearray(ram[:16384]);serial(expected);put(0x4000,expected)
   before=q.stamp_bytes//8
   assert lib.ntm_shot_club_initialize(C.byref(s),C.byref(task),C.byref(ctx),C.byref(club_resources))==0
   serial(expected)
   for j in range(before,q.stamp_bytes//8):
    st=q.stamps[j];source=next(src for src,a in tee_assets if C.cast(a.data,P).value==C.cast(st.asset.data,P).value)
    write(expected,0x4d86+8*j,source,4);write(expected,0x4d8a+8*j,0x9000+st.offset,2);write(expected,0x4d8c+8*j,st.tile,2)
   reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x20ace7);low=0x6be0
   for _ in range(10000):
    if get(8)==0xef00:break
    step(0);low=min(low,get(7))
   else:raise AssertionError(('budget',hex(get(8))))
   actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
   assert actual==expected,(power,terrain,mode,[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:30])
   cases+=1
print(json.dumps({'cases':cases,'scope':'20ace7/20adcb full nonstack RAM, twelve power tables, fifteen terrains, exact stamp identities including saturated queues'}))
