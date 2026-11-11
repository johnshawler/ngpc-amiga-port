#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exact parent power/accuracy allocation and two distinct original cancels.

Children are allocated but not dispatched. The final swing and world reload
remain explicit boundaries. Uses the existing unchanged original test core.
"""
from shot_fire_case_common import *
import os,random
class Power(C.Structure):
 _fields_=[(n,P) for n in ('meter_entry','power_delay','power_ready','accuracy_entry','accuracy_delay','accuracy_ready','cancel_rebuild','cancel_restore','cancel_return','fire_closed')]+[('phase',U8),('boundary',U8)]
class PowerContext(C.Structure):_fields_=[('scene',P),('overlay',P),('saved_meter',P)]
def new_power():return Power(0x20b70b,0x209a2e,0x209a3f,0x20bc19,0x209a77,0x209a88,0x209b9a,0x209bc1,0x209bd6,0x2099a5,0,0)
lib=C.CDLL(os.environ.get('NTM_SHOT_POWER_LIBRARY',str(ROOT/'build/shot-power-cancel-check/native.so')))
for name in ('initialize','cancel'):getattr(lib,'ntm_shot_power_'+name).argtypes=[P,P,P]
lib.ntm_shot_power_tick.argtypes=[P,P,P,U8]
r=random.Random(0x209a0a)
def video_data():return b''.join(load(a,4).to_bytes(4,'little') for a in range(0x8000,0xc000,4))
def run(pc):
 reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
 for _ in range(10000):
  if get(8) in (0xef00,0x2097f0,0x209aa2):return low,get(8)
  step(0);low=min(low,get(7))
 raise AssertionError(('budget',hex(get(8))))
def case(pc,delay=0,pressed=0,club=0,available=2,descendants=0,priority=40):
 setup();put(0x4000,r.randbytes(16384));
 for a in range(0x9000,0xa000,4):store(a,r.getrandbits(32),4)
 store(0x41c9,0x4149,4);store(0x417a,0x4100,2);store(0x4101,pressed,1)
 aim=AM.read_aim();aim.club=club;hole=AM.read_state()
 maps=blob(data(0x9000,4096));saved=blob(data(0x44ff,32));meter=blob(data(0x451f,4));view=blob(data(0x8002,4));words=(U16*2)(load(0x4d80,2),load(0x4d82,2));overlay=U8(load(0x413b,1))
 nodes=(Task*8)();addresses=(0x6000,0x6040,0x6080,0x60c0,0x6100,0x6140,0x6180,0x4ee6)
 for j,n in enumerate(nodes):
  n.callback=pc;n.previous=addr(nodes[(j+7)%8]);n.next=addr(nodes[(j+1)%8]);n.owner=addr(nodes[0]);n.priority=r.randrange(256);n.result=r.randrange(256);n.flags=r.randrange(256);n.substate=r.randrange(256);n.delay=r.randrange(65536)
 nodes[0].priority=priority;nodes[0].delay=delay;nodes[0].previous=addr(nodes[1]);nodes[1].next=addr(nodes[0]);nodes[1].priority=priority
 active=[0]+list(range(2,2+descendants))+[1]
 for i,j in enumerate(active):nodes[j].previous=addr(nodes[active[(i-1)%len(active)]]);nodes[j].next=addr(nodes[active[(i+1)%len(active)]])
 for j in range(2,2+descendants):nodes[j].priority=priority+1+j%2
 for j in range(available):nodes[5+j].next=addr(nodes[6+j]) if j+1<available else None
 tasks=TaskContext();tasks.free_head=addr(nodes[5]) if available else None;tasks.current=tasks.published=addr(nodes[0])
 intro=Context();intro.maps=addr(maps);intro.viewport=addr(view);intro.screen_words=addr(words)
 shared=TeeContext();shared.intro=addr(intro);shared.tasks=addr(tasks);shared.fallback=addr(nodes[7])
 scene=SceneObjectContext(addr(shared),addr(hole),None,addr(aim),addr(saved));ctx=PowerContext(addr(scene),addr(overlay),addr(meter));state=new_power()
 phases={0x209a2e:1,0x209a3f:2,0x209a77:3,0x209a88:4,0x209b9a:5,0x209bc1:6,0x209bd6:7}
 state.phase=phases.get(pc,0)
 def serial(b):
  AM.serialize_aim(aim,b);write(b,0x413b,overlay.value);write(b,0x4d80,words[0],2);write(b,0x4d82,words[1],2);return serial_tasks(nodes,addresses,tasks,b)
 expected=bytearray(ram[:16384]);serial(expected);put(0x4000,expected);video=bytearray(video_data())
 if pc==0x209a0a:result=lib.ntm_shot_power_initialize(C.byref(state),C.byref(nodes[0]),C.byref(ctx))
 elif pc==0x209b71:result=lib.ntm_shot_power_cancel(C.byref(state),C.byref(nodes[0]),C.byref(ctx))
 else:result=lib.ntm_shot_power_tick(C.byref(state),C.byref(nodes[0]),C.byref(ctx),pressed)
 mapping=serial(expected);video[2:6]=bytes(view);video[0x1000:0x2000]=bytes(maps)
 low,end=run(pc);label=(hex(pc),delay,pressed,club,available,descendants,priority)
 assert result==(0 if end==0xef00 else 2),(label,result,hex(end))
 assert state.boundary=={0xef00:0,0x2097f0:1,0x209aa2:2}[end],(label,state.boundary,hex(end))
 assert get(6)==mapping[tasks.current],(label,'current',hex(get(6)),mapping[tasks.current])
 actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==expected,(label,[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:20])
 actual=video_data();assert actual==video,(label,'video',[(hex(0x8000+i),a,b) for i,(a,b) in enumerate(zip(actual,video)) if a!=b][:20])
 if state.phase in (8,9):
  snapshot=bytes(state),bytes(nodes),bytes(maps),bytes(aim)
  assert lib.ntm_shot_power_tick(C.byref(state),C.byref(nodes[0]),C.byref(ctx),pressed)==-1
  assert snapshot==(bytes(state),bytes(nodes),bytes(maps),bytes(aim))
counts={}
for pc in (0x209a0a,0x209b71,0x209b9a,0x209bc1,0x209bd6):
 count=0
 for j in range(96):case(pc,available=j%3,descendants=(j//3)%4 if pc==0x209b71 else 0,priority=(j//12)*31);count+=1
 counts[hex(pc)]=count
for pc in (0x209a2e,0x209a3f,0x209a77,0x209a88):
 count=0
 for delay in (0,1,3,4,32767,32768,32769,65535):
  for pressed in range(256):
   for club in (0,13):case(pc,delay,pressed,club,available=pressed%3,priority=255 if pressed&1 else 0);count+=1
 counts[hex(pc)]=count
print(json.dumps({'cases':sum(counts.values()),'entries':counts,'scope':'all nonstack RAM and hardware/video, callback/scratch/free-active links, real allocations and descendant removals; every pressed byte and signed delays; boundaries before world reload2097f0 and swing209aa2; children never dispatched'}))
