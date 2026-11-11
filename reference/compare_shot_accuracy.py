#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Unchanged-original accuracy child, all nonstack RAM and hardware/video."""
from shot_accuracy_case_common import *
import os,random
lib=C.CDLL(os.environ['NTM_SHOT_ACCURACY_LIBRARY'])
lib.ntm_shot_accuracy_initialize.argtypes=[P,P,P]
lib.ntm_shot_accuracy_tick.argtypes=[P,P,U8,U8,U8,P,P]
R=random.Random(0x20bc19);cases=0;finish_address=C.cast(lib.ntm_task_finish,P).value
for mode in ('init','move','commit'):
 for k in range(512 if mode=='init' else 2048):
  setup();put(0x4000,R.randbytes(16384));put(0x8000,R.randbytes(16384));video=data(0x8000,16384)
  h=AM.State.from_buffer_copy(R.randbytes(C.sizeof(AM.State)));p=AM.Player(0,k%2,1-k%2,0);aim=AM.Aim.from_buffer_copy(R.randbytes(C.sizeof(AM.Aim)))
  aim.club=13 if k%7==0 else k%16
  player=h.players[p.active_player];player[1]=k%6;player[0x33]=0 if k%2 else 6;h.hole[0x70]=(k//6)%16
  held=R.randrange(256);label=(0,9,10,20,21,127,128,255)[k%8]
  words=(U16*2)(R.randrange(65536),R.randrange(65536));objects=(Cursor*32)();objects[7].y=R.randrange(65536)
  ic=Context();ic.screen_words=addr(words);tc=TeeContext();tc.intro=addr(ic);tc.objects=addr(objects)
  ctx=SceneObjectContext(addr(tc),addr(h),addr(p),addr(aim),None)
  s=Accuracy(0x20bc5b,R.randrange(65536),R.randrange(65536),R.randrange(256),R.randrange(256),0 if mode=='init' else 1)
  if mode=='commit':
   s.position=((0,1,17,18,19,20,21,37,38,127,128,255)[k%12]<<8)|R.randrange(256)
   s.lower=(0,1,5,17,19,128,255)[k%7];s.upper=(19,21,32,37,127,128,255)[k%7]
   h.hole[0x7e]=(4,100,127,128,0)[k%5];h.hole[0x7d]=(0,3,96,99,100,127,128,255)[(k//5)%8]
  lifecycle=4 if mode=='commit' else k%256
  if lifecycle==4 and mode!='commit':lifecycle=3
  nodes=(Task*4)();addresses=(0x6100,0x6000,0x6200,0x6300);mapping={addr(n):a for n,a in zip(nodes,addresses)};mapping[None]=0xffffffff
  tasks=TaskContext();tasks.current=addr(nodes[1]);tasks.free_head=addr(nodes[3]);finishmode=k%3
  for i,n in enumerate(nodes):
   n.callback=0x20bc19 if mode=='init' else 0x20bc5b;n.previous=addr(nodes[(i+3)%4]);n.next=addr(nodes[(i+1)%4]);n.owner=addr(nodes[0]);n.flags=R.randrange(256);n.priority=22;n.result=R.randrange(256);n.delay=R.randrange(65536)
  nodes[2].priority=23 if finishmode==0 else 22;nodes[1].flags=(nodes[1].flags&254)|(finishmode==1)
  def serial(b):
   b[0x149:0x189]=bytes(h.players[0]);b[0x189:0x1c9]=bytes(h.players[1]);b[0x200:0x300]=bytes(h.hole)
   write(b,0x41c9,0x4149+64*p.active_player,4)
   write(b,0x4149+64*p.active_player+0x31,0x40bf,2);write(b,0x40bf,held)
   write(b,0x41dc,aim.club);write(b,0x41dd,label);write(b,0x41d1,lifecycle);write(b,0x41d2,aim.reset_d2);write(b,0x41d3,aim.reset_d3)
   for a,n in zip(addresses,nodes):
    write(b,a,0x20132b if n.callback==finish_address else n.callback,4)
    for name,off in (('previous',4),('next',8),('owner',12)):write(b,a+off,mapping[getattr(n,name)],4)
    for name,off in (('priority',16),('result',17),('flags',18),('substate',19)):write(b,a+off,getattr(n,name))
    write(b,a+20,n.delay,2)
   write(b,0x5706,mapping[tasks.free_head],4)
   write(b,0x6016,s.position,2);write(b,0x6018,s.velocity,2);write(b,0x601a,s.upper);write(b,0x601b,s.lower)
   write(b,0x4701,objects[7].y,2)
   for j in range(2):write(b,0x4d80+2*j,words[j],2)
  b=bytearray(ram[:16384]);serial(b);put(0x4000,b)
  if mode=='init':result=lib.ntm_shot_accuracy_initialize(C.byref(s),C.byref(nodes[1]),C.byref(ctx));pc=0x20bc19
  else:result=lib.ntm_shot_accuracy_tick(C.byref(s),C.byref(ctx),lifecycle,label,held,C.byref(tasks),C.byref(nodes[1]));pc=0x20bc5b
  assert result==((1,2,3)[finishmode] if mode=='commit' else 0),(mode,k,result)
  serial(b);reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
  for _ in range(1000):
   if get(8)==0xef00:break
   step(0);low=min(low,get(7))
  else:raise AssertionError(('budget',mode,k,hex(get(8))))
  actual=bytes(ram[:16384]);b[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
  assert actual==b,(mode,k,[(hex(0x4000+i),a,v) for i,(a,v) in enumerate(zip(actual,b)) if a!=v][:20])
  assert data(0x8000,16384)==video,(mode,k,'video')
  if mode=='commit':assert get(6)==mapping[tasks.current]
  cases+=1
print(json.dumps({'cases':cases,'scope':'20bc19/20bc5b/20bcec/20bd3e/20dbd1/20132b unchanged original full nonstack RAM, task scratch and links, hit flags, marker, hardware/video; no intercepted calls'}))
