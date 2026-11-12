#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Unchanged original swing child: whole nonstack RAM/video, queues and sounds."""
from shot_trajectory_swing_case_common import *
import os,random
lib=C.CDLL(os.environ['NTM_SHOT_TRAJECTORY_SWING_LIBRARY'])
for name in ('enter','tick'):getattr(lib,'ntm_shot_trajectory_swing_'+name).argtypes=[P,P,P,P,P,P,U8,U8]
finish=C.cast(lib.ntm_task_finish,P).value
R=random.Random(0x20b43f);cases=0;sounds=0
for k in range(6144):
 init=k<3072;setup();put(0x4000,R.randbytes(16384));put(0x8000,R.randbytes(16384));video=data(0x8000,16384)
 g=k%6;category=k//6%3;label=k%23;suppress=k%2;club=13 if k//2%2 else 4
 h=AM.State();h.hole[0x70]=k%15;h.hole[0x7d]=k%256
 aim=AM.Aim();aim.club=club;aim.reset_d2=k//4%2
 objects=(Cursor*32).from_buffer_copy(R.randbytes(C.sizeof(Cursor)*32));x=(U16*32)();q=Screen();q.stamp_bytes=(0,248,256)[k%3]
 ic=Context();ic.screen=addr(q);tc=TeeContext();tc.intro=addr(ic);tc.objects=addr(objects)
 requests=[]
 cb=C.CFUNCTYPE(None,P,U8,U16,U16)(lambda _,a,b,c:requests.append((a,b,c)))
 tc.sound_request=C.cast(cb,P);ctx=SceneObjectContext(addr(tc),addr(h),None,addr(aim),None)
 s=TrajectorySwing(0x20b47f,(1,2,0,255)[k//16%4],0,0 if init else 1)
 p=Trajectory(0x20b432,4*load(0x21ee8d+g,1),4*load(0x21ee93+g,1),60*category,k//32%16,3+k%2,1)
 nodes=(Task*4)();addresses=(0x6100,0x6000,0x6200,0x6300);mapping={addr(n):a for n,a in zip(nodes,addresses)};mapping[None]=0xffffffff
 tasks=TaskContext();tasks.current=addr(nodes[1]);tasks.free_head=addr(nodes[3]);mode=k//128%3
 for i,n in enumerate(nodes):
  n.callback=0x20b43f if init else 0x20b47f;n.previous=addr(nodes[(i+3)%4]);n.next=addr(nodes[(i+1)%4]);n.owner=addr(nodes[0]);n.priority=22;n.flags=R.randrange(256)
 nodes[2].priority=23 if mode==0 else 22;nodes[1].flags=(nodes[1].flags&254)|(mode==1)
 def serial(b):
  b[0x200:0x300]=bytes(h.hole);write(b,0x41dc,aim.club);write(b,0x41dd,label);write(b,0x41d2,aim.reset_d2);write(b,0x447e,suppress)
  serial_objects(objects,x,b)
  for a,n in zip(addresses,nodes):
   write(b,a,0x20132b if n.callback==finish else n.callback,4)
   for name,off in (('previous',4),('next',8),('owner',12)):write(b,a+off,mapping[getattr(n,name)],4)
   for name,off in (('priority',16),('result',17),('flags',18),('substate',19)):write(b,a+off,getattr(n,name))
   write(b,a+20,n.delay,2)
  write(b,0x5706,mapping[tasks.free_head],4)
  for at,value,size in ((0x6016,p.counter,1),(0x6017,s.timer,1),(0x6018,p.hud_base,2),(0x601a,p.sprite_base,2),(0x601c,p.hud_delta,2),(0x601e,p.last_label,1),(0x601f,s.result,1),(0x4d84,q.stamp_bytes,2)):write(b,at,value,size)
 b=bytearray(ram[:16384]);serial(b);put(0x4000,b);before=q.stamp_bytes//8
 result=getattr(lib,'ntm_shot_trajectory_swing_'+('enter' if init else 'tick'))(C.byref(s),C.byref(p),C.byref(tasks),C.byref(nodes[1]),C.byref(ctx),C.byref(swing_resources),suppress,label)
 assert result>=0,(k,result);serial(b)
 for j in range(before,q.stamp_bytes//8):
  st=q.stamps[j];source=next(src for src,a in tee_assets if C.cast(a.data,P).value==C.cast(st.asset.data,P).value)
  write(b,0x4d86+8*j,source,4);write(b,0x4d8a+8*j,0x9000+st.offset,2);write(b,0x4d8c+8*j,st.tile,2)
 reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x20b43f if init else 0x20b47f);low=0x6be0;actual_requests=[]
 for _ in range(10000):
  pc=get(8)
  if pc==0xef00:break
  if pc in (load(0x250020,4),load(0x25001c,4)):
   actual_requests.append((0 if pc==load(0x250020,4) else 2,get(0)&65535,0 if pc==load(0x250020,4) else get(1)&255));sp=get(7);reg(8,load(sp,4));reg(7,sp+4);continue
  step(0);low=min(low,get(7))
 else:raise AssertionError(('budget',k,hex(get(8))))
 actual=bytes(ram[:16384]);b[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==b,(k,[(hex(0x4000+i),a,v) for i,(a,v) in enumerate(zip(actual,b)) if a!=v][:20])
 assert video==data(0x8000,16384),(k,'video')
 assert actual_requests==requests,(k,actual_requests,requests)
 assert get(6)==mapping[tasks.current],(k,'current')
 cases+=1;sounds+=len(requests)
print(json.dumps({'cases':cases,'sound_requests':sounds,'scope':'unchanged20b43f/20b47f/20b50b full nonstack RAM, video, queue identities, lifecycle and sound request boundary'}))
