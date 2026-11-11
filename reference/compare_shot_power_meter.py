#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Unchanged original power-meter RAM, hardware, tiles and task oracle."""
from shot_power_meter_case_common import *
lib=C.CDLL(os.environ['NTM_SHOT_POWER_METER_LIBRARY'])
lib.ntm_shot_power_meter_initialize.argtypes=[P,P,P,P]
lib.ntm_shot_power_meter_tick.argtypes=[P,P,P,U8,P,P]
lib.ntm_shot_power_meter_advance.argtypes=[P,P,P,P]
R=random.Random(0x20b70b);cases=0

def run(pc,b):
 reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
 for _ in range(10000):
  if get(8)==0xef00:break
  step(0);low=min(low,get(7))
 else:raise AssertionError(('budget',hex(get(8))))
 actual=bytes(ram[:16384]);b[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==b,(hex(pc),[(hex(0x4000+i),a,v) for i,(a,v) in enumerate(zip(actual,b)) if a!=v][:20])

for mode in ('init','advance','tick'):
 for k in range(512 if mode=='init' else 2048):
  setup();put(0x4000,R.randbytes(16384));put(0x8000,R.randbytes(16384))
  hardware=data(0x8000,4096)
  h=AM.State.from_buffer_copy(R.randbytes(C.sizeof(AM.State)));p=AM.Player(0,k%2,1-k%2,0)
  player=h.players[p.active_player]
  # All wrapped speed lookup indices, character2 hold, and ordinary characters.
  player[1]=2 if k%2 else k%6
  player[0x33]=(k//6)%2*6 if mode=='init' else R.randrange(256)
  player[0x23]=k%10
  h.hole[0x70]=k%7;player[3]=k%4
  aim=AM.Aim();aim.club=k%16
  objects=(Cursor*32).from_buffer_copy(R.randbytes(C.sizeof(Cursor)*32));words=(U16*2)(R.randrange(65536),R.randrange(65536))
  maps=blob(data(0x9000,4096));tiles=blob(data(0xa000,8192));ic=Context();ic.maps=addr(maps);ic.screen_words=addr(words)
  tc=TeeContext();tc.intro=addr(ic);tc.tiles=addr(tiles);tc.objects=addr(objects)
  ctx=SceneObjectContext(addr(tc),addr(h),addr(p),addr(aim),None)
  s=Meter(0x20b74b,R.randrange(65536),R.randrange(65536),R.randrange(256),R.randrange(256),R.randrange(256),0 if mode=='init' else 1)
  if mode=='tick':
   s.position=256+(k%63)*256;s.velocity=(40,120,256,500,65416,65280)[k%6];s.upper=62;s.lower=1;s.hold=(1,2,5,0)[k%4];h.hole[0x7d]=k%121
  t=Task();t.callback=0x20b70b if mode=='init' else 0x20b74b
  def serial(b):
   b[0x149:0x189]=bytes(h.players[0]);b[0x189:0x1c9]=bytes(h.players[1]);b[0x200:0x300]=bytes(h.hole)
   write(b,0x41c9,0x4149+64*p.active_player,4);write(b,0x41dc,aim.club);write(b,0x41d1,2)
   write(b,0x6000,t.callback,4);write(b,0x6016,s.position,2);write(b,0x6018,s.velocity,2)
   for at,name in [(0x601a,'upper'),(0x601b,'lower'),(0x601c,'hold')]:write(b,at,getattr(s,name))
   write(b,0x47c1,objects[10].y,2)
   for j in range(2):write(b,0x4d80+2*j,words[j],2)
  b=bytearray(ram[:16384]);serial(b);put(0x4000,b)
  if mode=='init':result=lib.ntm_shot_power_meter_initialize(C.byref(s),C.byref(t),C.byref(ctx),C.byref(r));pc=0x20b70b
  elif mode=='advance':result=lib.ntm_shot_power_meter_advance(C.byref(s),C.byref(h),C.byref(p),C.byref(r));pc=0x20b7c8
  else:result=lib.ntm_shot_power_meter_tick(C.byref(s),C.byref(ctx),C.byref(r),2,None,C.byref(t));pc=0x20b74b
  assert result==0,(mode,k,result)
  serial(b);run(pc,b)
  assert data(0x8000,4096)==hardware,(mode,k,'hardware')
  assert data(0x9000,4096)==bytes(maps),(mode,k,'maps')
  assert data(0xa000,8192)==bytes(tiles),(mode,k,'tiles')
  cases+=1
# Full task finishing path, not substituted helpers; every lifecycle except2.
finish_address=C.cast(lib.ntm_task_finish,P).value
for lifecycle in range(256):
 if lifecycle==2:continue
 for mode in range(3):
  setup();put(0x4000,R.randbytes(16384));put(0x8000,R.randbytes(16384));video=data(0x8000,16384)
  h=AM.State();s=Meter();s.phase=1;ctx=SceneObjectContext();ctx.hole=addr(h)
  nodes=(Task*4)();addresses=(0x6100,0x6000,0x6200,0x6300)
  mapping={addr(n):a for n,a in zip(nodes,addresses)};mapping[None]=0xffffffff
  tasks=TaskContext();tasks.current=addr(nodes[1]);tasks.free_head=addr(nodes[3])
  for i,n in enumerate(nodes):
   n.callback=0x20b74b;n.previous=addr(nodes[(i+3)%4]);n.next=addr(nodes[(i+1)%4]);n.owner=addr(nodes[0]);n.flags=R.randrange(256);n.priority=22
  nodes[2].priority=23 if mode==0 else 22;nodes[1].flags=(nodes[1].flags&254)|(mode==1)
  def finish_serial(b):
   write(b,0x41d1,lifecycle)
   for a,n in zip(addresses,nodes):
    write(b,a,0x20132b if n.callback==finish_address else n.callback,4)
    for name,off in (('previous',4),('next',8),('owner',12)):write(b,a+off,mapping[getattr(n,name)],4)
    for name,off in (('priority',16),('result',17),('flags',18),('substate',19)):write(b,a+off,getattr(n,name))
    write(b,a+20,n.delay,2)
   write(b,0x5706,mapping[tasks.free_head],4)
  b=bytearray(ram[:16384]);finish_serial(b);put(0x4000,b)
  assert lib.ntm_shot_power_meter_tick(C.byref(s),C.byref(ctx),None,lifecycle,C.byref(tasks),C.byref(nodes[1]))==(1,2,3)[mode]
  finish_serial(b);run(0x20b74b,b);assert get(6)==mapping[tasks.current]
  assert data(0x8000,16384)==video;cases+=1
print(json.dumps({'cases':cases,'scope':'20b70b,20b7c8,20b74b full nonstack RAM, scratch, hardware/video, native callback and genuine lifecycle links; unchanged original, no intercepted calls'}))
