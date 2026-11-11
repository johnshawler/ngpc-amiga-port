#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original camera termination removes real descendants and recycles task."""
from shot_camera_case_common import *
import os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_CAMERA_LIBRARY',str(ROOT/'build/shot-probe/camera.so')))
lib.ntm_shot_camera_active.argtypes=[P,P,P,P,P,U8]
R=random.Random(0x20ca2c);count=0
for mode in range(1,256):
 for descendants in range(4):
  setup();put(0x4000,R.randbytes(16384));aim=AM.read_aim();aim.reset_d1=mode
  nodes=(Task*7)();addresses=(0x6100,0x6000,0x6200,0x6300,0x6400,0x6500,0x6600)
  mapping={addr(n):a for n,a in zip(nodes,addresses)};mapping[None]=0xffffffff
  def old(p):return mapping[p]
  for i,n in enumerate(nodes):
   n.callback=0x20c8a5;n.previous=addr(nodes[(i+6)%7]);n.next=addr(nodes[(i+1)%7]);n.owner=addr(nodes[0])
   n.priority=R.randrange(256);n.result=R.randrange(256);n.flags=R.randrange(256);n.substate=R.randrange(256);n.delay=R.randrange(65536)
  nodes[1].priority=R.randrange(255)
  for i in range(2,2+descendants):nodes[i].priority=R.randrange(nodes[1].priority+1,256)
  nodes[2+descendants].priority=nodes[1].priority
  tasks=TaskContext();tasks.current=addr(nodes[1]);tasks.published=addr(nodes[1]);tasks.free_head=addr(nodes[6]);nodes[6].next=None
  state=CameraState(R.randrange(65536),R.randrange(65536));ctx=MarkerContext();ctx.aim=addr(aim)
  def serial(b):
   AM.serialize_aim(aim,b)
   for a,n in zip(addresses,nodes):
    write(b,a,n.callback,4)
    for name,off in (('previous',4),('next',8),('owner',12)):write(b,a+off,old(getattr(n,name)),4)
    for name,off in (('priority',16),('result',17),('flags',18),('substate',19)):write(b,a+off,getattr(n,name))
    write(b,a+20,n.delay,2)
   write(b,0x5706,old(tasks.free_head),4)
  expected=bytearray(ram[:16384]);serial(expected);put(0x4000,expected)
  assert lib.ntm_shot_camera_active(C.byref(state),C.byref(nodes[1]),C.byref(tasks),C.byref(ctx),None,0)==0
  serial(expected);reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x20c8a5);low=0x6be0
  for _ in range(1000):
   if get(8)==0xef00:break
   step(0);low=min(low,get(7))
  else:raise AssertionError(('budget',mode,descendants))
  actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
  assert actual==expected,(mode,descendants,[(hex(i+0x4000),x,y) for i,(x,y) in enumerate(zip(actual,expected)) if x!=y][:20])
  assert get(6)==old(tasks.current),(mode,descendants,'current');count+=1
print(json.dumps({'termination_cases':count,'scope':'all nonzero lifecycle values,zero to three descendants,mixed task flags,full nonstack RAM and free/active links'}))
