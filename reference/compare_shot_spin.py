#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original20b604 initializer and bounded20b62d full nonstack RAM oracle."""
from shot_spin_case_common import *
import os,random
lib=C.CDLL(os.environ['NTM_SHOT_SPIN_LIBRARY'])
lib.ntm_shot_spin_initialize.argtypes=[C.POINTER(Spin),C.POINTER(Task),C.POINTER(AM.State),C.POINTER(AM.Player),C.POINTER(SpinResources)]
lib.ntm_shot_spin_tick.argtypes=[C.POINTER(Spin),C.POINTER(AM.State),U8,U8,U8,P,P]
R=random.Random(0x20b604);cases=0

def check(init,power,bank,lifecycle=1,gate=1,pressed=0):
 global cases
 setup();put(0x4000,R.randbytes(16384))
 h=AM.State.from_buffer_copy(R.randbytes(C.sizeof(AM.State)));p=AM.Player(0,bank,1-bank,0)
 h.players[bank][1]=power%6;h.players[bank][0x33]=6 if power>=6 else 0
 s=Spin();s.wait=0x20b62d;s.maximum=R.randrange(256);s.minimum=R.randrange(256);s.current=h.hole[0x76];s.phase=0 if init else 1
 if not init and lifecycle!=2:h.hole[0x77]=0
 task=Task();task.callback=0x20b604 if init else 0x20b62d
 b=bytearray(ram[:16384])
 def serial():
  b[0x149:0x189]=bytes(h.players[0]);b[0x189:0x1c9]=bytes(h.players[1]);b[0x200:0x300]=bytes(h.hole)
  write(b,0x41c9,0x4149+64*bank,4);write(b,0x41d1,lifecycle);write(b,0x41e1,gate);write(b,0x40c0,pressed);h.players[bank][0x31]=0xbf;h.players[bank][0x32]=0x40; b[0x149+64*bank:0x189+64*bank]=bytes(h.players[bank])
  write(b,0x6000,task.callback,4);write(b,0x6016,s.minimum);write(b,0x6017,s.maximum);write(b,0x6018,s.current)
 serial();put(0x4000,b)
 result=lib.ntm_shot_spin_initialize(C.byref(s),C.byref(task),C.byref(h),C.byref(p),C.byref(spin_resources)) if init else lib.ntm_shot_spin_tick(C.byref(s),C.byref(h),lifecycle,gate,pressed,None,None)
 assert result==0
 serial();reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x20b604 if init else 0x20b62d);low=0x6be0
 for _ in range(1000):
  if get(8)==0xef00:break
  step(0);low=min(low,get(7))
 else:raise AssertionError(('budget',hex(get(8))))
 actual=bytes(ram[:16384]);b[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==b,(init,power,bank,lifecycle,[(hex(0x4000+i),a,v) for i,(a,v) in enumerate(zip(actual,b)) if a!=v][:20])
 cases+=1
for power in range(12):
 for bank in range(2):
  for repeat in range(16):check(True,power,bank)
for lifecycle in range(256):
 if lifecycle!=3:
  for bank in range(2):check(False,bank,bank,lifecycle)
for pressed in range(256):
 if (pressed&12) in (0,12):
  for bank in range(2):check(False,bank,bank,1,0,pressed)
# Exercise the unchanged20132b finish path, including defer, unlink and recycle.
finish_address=C.cast(lib.ntm_task_finish,P).value
for mode in range(3):
 for flags in range(256):
  setup();put(0x4000,R.randbytes(16384));h=AM.State();s=Spin();s.phase=1
  nodes=(Task*4)();addresses=(0x6100,0x6000,0x6200,0x6300)
  mapping={addr(n):a for n,a in zip(nodes,addresses)};mapping[None]=0xffffffff
  tasks=TaskContext();tasks.current=addr(nodes[1]);tasks.free_head=addr(nodes[3])
  for i,n in enumerate(nodes):
   n.callback=0x20b62d;n.previous=addr(nodes[(i+3)%4]);n.next=addr(nodes[(i+1)%4]);n.owner=addr(nodes[0]);n.flags=flags;n.priority=22
  nodes[2].priority=23 if mode==0 else 22
  nodes[1].flags=(flags&254)|(mode==1)
  def serial_finish(b):
   write(b,0x41d1,3)
   for a,n in zip(addresses,nodes):
    write(b,a,0x20132b if n.callback==finish_address else n.callback,4)
    for name,off in (('previous',4),('next',8),('owner',12)):write(b,a+off,mapping[getattr(n,name)],4)
    for name,off in (('priority',16),('result',17),('flags',18),('substate',19)):write(b,a+off,getattr(n,name))
    write(b,a+20,n.delay,2)
   write(b,0x5706,mapping[tasks.free_head],4)
  b=bytearray(ram[:16384]);serial_finish(b);put(0x4000,b)
  assert lib.ntm_shot_spin_tick(C.byref(s),C.byref(h),3,0,0,C.byref(tasks),C.byref(nodes[1]))==(1,2,3)[mode]
  serial_finish(b);reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x20b62d);low=0x6be0
  for _ in range(1000):
   if get(8)==0xef00:break
   step(0);low=min(low,get(7))
  else:raise AssertionError('finish budget')
  actual=bytes(ram[:16384]);b[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
  assert actual==b,(mode,flags,[(hex(0x4000+i),a,v) for i,(a,v) in enumerate(zip(actual,b)) if a!=v][:20])
  assert get(6)==mapping[tasks.current]
  cases+=1
print(json.dumps({'cases':cases,'scope':'20b604 and bounded20b62d plus20132b unchanged original full nonstack RAM, callback and untouched queue/resource identities; no intercepted calls'}))
