#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original marker transition, dirty-gated miniature refresh and termination."""
from shot_fire_case_common import *
import os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_FIRE_LIBRARY',str(ROOT/'build/shot-fire-check/native.so')))
lib.ntm_shot_fire_markers_tick.argtypes=[P,P,P,P,P]
r=random.Random(0x20b1d6);cases=0
for phase in (1,2):
 for case in range(1024):
  setup();put(0x4000,r.randbytes(16384));objects=(Cursor*32).from_buffer_copy(r.randbytes(C.sizeof(Cursor)*32));x=(U16*32)(*[r.randrange(65536) for _ in range(32)])
  h=AM.State.from_buffer_copy(r.randbytes(C.sizeof(AM.State)));aim=AM.read_aim();aim.reset_d1=1 if phase==1 else case%256
  aim.angle=r.randrange(65536);h.hole[0x6a:0x6c]=r.choice([0,48,49,159,160,161,32767,32768,65535]).to_bytes(2,'little')
  words=(U16*2)(*[r.randrange(65536) for _ in range(2)]);d=Display();ic=Context();ic.screen_words=addr(words);ic.display=addr(d)
  tc=TeeContext();tc.intro=addr(ic);tc.objects=addr(objects);tc.x=addr(x);dirty=U8(0 if case%3==0 else case%256)
  ctx=MarkerContext(addr(tc),addr(h),addr(aim),addr(dirty),r.randrange(256),0 if phase==1 else r.randrange(256))
  nodes=(Task*4)();addresses=(0x6000,0x6040,0x6080,0x5800)
  for j,n in enumerate(nodes):
   n.callback=0x20af8a if phase==1 else 0x20afa5;n.previous=addr(nodes[(j+3)%4]);n.next=addr(nodes[(j+1)%4]);n.owner=addr(nodes[2]);n.priority=r.randrange(256);n.result=r.randrange(256);n.flags=r.randrange(256);n.substate=r.randrange(256);n.delay=r.randrange(65536)
  nodes[3].next=None;tasks=TaskContext();tasks.current=tasks.published=addr(nodes[0]);tasks.free_head=addr(nodes[3])
  state=Markers(0x20af8a,0x20afa5,4,phase,512)
  def serial(b):
   serial_objects(objects,x,b);b[0x200:0x300]=bytes(h.hole);AM.serialize_aim(aim,b);write(b,0x41da,dirty.value);write(b,0x41db,ctx.player_status)
   for j in range(2):write(b,0x4d80+2*j,words[j],2)
   mapping=serial_tasks(nodes,addresses,tasks,b)
   return mapping
  expected=bytearray(ram[:16384]);serial(expected);put(0x4000,expected);video=data(0x8000,0x4000)
  assert lib.ntm_shot_fire_markers_tick(C.byref(state),C.byref(nodes[0]),C.byref(tasks),C.byref(ctx),C.byref(marker_resources))==0
  for n in nodes:
   if n.callback==C.cast(lib.ntm_task_finish,P).value:n.callback=0x20132b
  mapping=serial(expected);reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x20af8a if phase==1 else 0x20afa5);low=0x6be0
  for _ in range(10000):
   if get(8)==0xef00:break
   step(0);low=min(low,get(7))
  else:raise AssertionError(('budget',phase,case,hex(get(8))))
  actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
  assert actual==expected,(phase,case,[(hex(i+0x4000),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:20])
  assert get(6)==mapping[tasks.current] and data(0x8000,0x4000)==video
  cases+=1
print(json.dumps({'marker_cases':cases,'scope':'full nonstack RAM,video,task cursor and active/free links; same-call transition,all lifecycle bytes,dirty refresh,wrapped angles/lengths; main marker and angle remain unchanged'}))
