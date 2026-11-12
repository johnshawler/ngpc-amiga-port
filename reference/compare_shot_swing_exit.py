#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Real reset/allocate/wait/finish prefixes; children never bypassed or run here."""
from shot_fire_case_common import *
import course_init_case_common as I
import os,random
class Exit(C.Structure):_fields_=[(n,P) for n in ('perfect_entry','perfect_wait','miss_entry','miss_wait')]+[('phase',U8)]
lib=C.CDLL(os.environ.get('NTM_SWING_EXIT_LIBRARY',str(ROOT/'build/swing-exit-check/native.so')))
for n in ('hit','miss','tick'):getattr(lib,'ntm_shot_swing_exit_'+n).argtypes=[P,P,P]
r=random.Random(0x209b05);cases=0
zero=I.zero_addresses
def case(pc,perfect,done,available,descendants,priority,flags):
 global cases
 setup();put(0x4000,r.randbytes(16384));aim=AM.read_aim();aim.reset_d3=perfect;aim.reset_e6=done
 d=I.Display();objects=(Cursor*32)();view=blob(data(0x8002,4));words=(U16*2)(load(0x4d80,2),load(0x4d82,2))
 for i,a in enumerate(zero):d.zero_flags[i]=load(a,1)
 d.capacity=load(0x66bc,1);d.limit=load(0x61aa,1)
 for i in range(32):d.object_ids[i]=objects[i].id=load(0x453b+64*i,1);d.object_tags[i]=objects[i].tag=load(0x453d+64*i,2);d.list[i]=load(0x4d3b+2*i,2)
 for k,base in enumerate((0x61bb,0x62fb)):
  for j in range(256):d.pools[k][j]=load(base+j,1)
 d.zero_words[:]=[load(0x4d7b,2),load(0x6180,2)];d.reset_words[:]=[load(0x5bcb,4),load(0x61ac,4)]
 nodes=(Task*8)();addresses=(0x7000,0x7040,0x7080,0x70c0,0x7100,0x7140,0x7180,0x4ee6)
 for j,n in enumerate(nodes):
  n.callback=pc;n.previous=addr(nodes[(j+7)%8]);n.next=addr(nodes[(j+1)%8]);n.owner=addr(nodes[1]);n.priority=r.randrange(256);n.result=r.randrange(256);n.flags=r.randrange(256);n.substate=r.randrange(256);n.delay=r.randrange(65536)
 nodes[0].priority=nodes[1].priority=priority;nodes[0].flags=flags
 active=[0]+list(range(2,2+descendants))+[1]
 for i,j in enumerate(active):nodes[j].previous=addr(nodes[active[(i-1)%len(active)]]);nodes[j].next=addr(nodes[active[(i+1)%len(active)]])
 for j in range(2,2+descendants):nodes[j].priority=min(priority+1+j%2,255)
 for j in range(available):nodes[5+j].next=addr(nodes[6+j]) if j+1<available else None
 tasks=TaskContext();tasks.free_head=addr(nodes[5]) if available else None;tasks.current=tasks.published=addr(nodes[0])
 intro=Context();intro.viewport=addr(view);intro.screen_words=addr(words);intro.display=addr(d)
 shared=TeeContext();shared.intro=addr(intro);shared.tasks=addr(tasks);shared.fallback=addr(nodes[7]);shared.objects=addr(objects)
 scene=SceneObjectContext();scene.shared=addr(shared);scene.aim=addr(aim)
 s=Exit(0x20c64e,0x209b29,0x2109d5,0x209b63,{0x209b05:0,0x209b4b:0,0x209b29:1,0x209b63:2}[pc])
 def serial(b):
  AM.serialize_aim(aim,b)
  for i,a in enumerate(zero):write(b,a,d.zero_flags[i])
  write(b,0x66bc,d.capacity);write(b,0x61aa,d.limit)
  for i in range(32):write(b,0x453b+64*i,objects[i].id);write(b,0x453d+64*i,objects[i].tag,2);write(b,0x4d3b+2*i,d.list[i],2)
  for k,base in enumerate((0x61bb,0x62fb)):
   b[base-0x4000:base-0x4000+256]=bytes(d.pools[k])
  for a,v in zip((0x4d7b,0x6180),d.zero_words):write(b,a,v,2)
  for a,v in zip((0x5bcb,0x61ac),d.reset_words):write(b,a,v,4)
  write(b,0x4d80,words[0],2);write(b,0x4d82,words[1],2)
  return serial_tasks(nodes,addresses,tasks,b)
 expected=bytearray(ram[:16384]);serial(expected);put(0x4000,expected);video=bytearray(data(0x8000,0x4000))
 fn=lib.ntm_shot_swing_exit_hit if pc==0x209b05 else lib.ntm_shot_swing_exit_miss if pc==0x209b4b else lib.ntm_shot_swing_exit_tick
 result=fn(C.byref(s),C.byref(nodes[0]),C.byref(scene));assert result>=0
 mapping=serial(expected);video[2:6]=bytes(view)
 reg(6,0x7000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
 for _ in range(20000):
  if get(8)==0xef00:break
  step(0);low=min(low,get(7))
 else:raise AssertionError(('budget',hex(pc),hex(get(8))))
 label=(hex(pc),perfect,done,available,descendants,priority,flags)
 assert get(6)==mapping[tasks.current],(label,'current')
 actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==expected,(label,[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:20])
 assert data(0x8000,0x4000)==video,(label,'video')
 cases+=1
for pc in (0x209b05,0x209b4b,0x209b29,0x209b63):
 for j in range(768):case(pc,(0,1,2,255)[j%4],(0,1,128,255)[(j//4)%4],j%3,(j//3)%4,(0,1,40,127,253,254,255)[(j//12)%7],j%256)
print(json.dumps({'cases':cases,'scope':'full nonstack RAM/video, exact reset and callback, active/free links, real descendant removal/allocation/finish; children never dispatched'}))
