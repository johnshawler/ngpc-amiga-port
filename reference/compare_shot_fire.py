#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original209919 parent prefix, waits and gate; children are not dispatched."""
from shot_fire_case_common import *
import os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_FIRE_LIBRARY',str(ROOT/'build/shot-fire-check/native.so')))
lib.ntm_shot_fire_initialize.argtypes=[P,P,P,P];lib.ntm_shot_fire_tick.argtypes=[P,P,P,U8]
r=random.Random(0x209919)
# Word-sized reads expose the same original hardware/video byte image, with no
# core changes. Check byte/word equivalence before using the faster snapshot.
def video_data():return b''.join(load(a,4).to_bytes(4,'little') for a in range(0x8000,0xc000,4))
assert video_data()==data(0x8000,0x4000)
def run(pc,stops=()):
 reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
 for _ in range(10000):
  if get(8)==0xef00 or get(8) in stops:return low,get(8)
  step(0);low=min(low,get(7))
 raise AssertionError(('budget',hex(get(8))))
def check(expected,video,low,label):
 actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==expected,(label,[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:20])
 actual=video_data()
 assert actual==video,(label,'video',[(hex(0x8000+i),a,b) for i,(a,b) in enumerate(zip(actual,video)) if a!=b][:20])
entries=0
for case in range(576):
 setup();put(0x4000,r.randbytes(16384));put(0x9000,r.randbytes(4096));store(0x40fa,16+case%240,1)
 d=Display.from_buffer_copy(r.randbytes(C.sizeof(Display)));objects=(Cursor*32).from_buffer_copy(r.randbytes(C.sizeof(Cursor)*32));x=(U16*32)(*[r.randrange(65536) for _ in range(32)])
 mask=case//36;occupied=case%9;available=(case//9)%4
 ids=[j for j in range(32) if j not in (0,2,5,17) or mask&(1<<(0,2,5,17).index(j))];r.shuffle(ids)
 d.zero_flags[8]=len(ids);d.zero_words[0]=2*len(ids)
 for j in range(32):
  d.list[j]=ids[j]+256*r.randrange(256) if j<len(ids) else r.randrange(65536)
  objects[j].id=d.object_ids[j]=j;objects[j].tag=d.object_tags[j]=r.randrange(256) if j in ids else 65535;objects[j].byte30&=254
 palette=Palette.from_buffer_copy(r.randbytes(C.sizeof(Palette)))
 perm=list(range(8));r.shuffle(perm)
 for j in range(8):palette.slots[j]=4*perm[j]+(0x400 if j<occupied else 0xff00)
 # Only the single requested palette is resolved; pre-existing queued records
 # and their unconsumed original bytes are preserved, not drained here.
 maps=blob(data(0x9000,4096));view=blob(data(0x8002,4));flags=U8(load(0x400d,1));overlay=U8(load(0x413b,1));aim=AM.read_aim()
 nodes=(Task*7)();addresses=(0x6000,0x6040,0x6080,0x5800,0x5840,0x5880,0x4ee6)
 for j,n in enumerate(nodes):
  n.callback=0x2098ff;n.previous=addr(nodes[(j+6)%7]);n.next=addr(nodes[(j+1)%7]);n.owner=addr(nodes[0]);n.priority=r.randrange(256);n.result=r.randrange(256);n.flags=r.randrange(256);n.substate=r.randrange(256);n.delay=r.randrange(65536)
 nodes[0].next=addr(nodes[1]);nodes[1].previous=addr(nodes[0])
 for j in range(available):nodes[j+3].next=addr(nodes[j+4]) if j+1<available else None
 tasks=TaskContext();tasks.free_head=addr(nodes[3]) if available else None;tasks.current=tasks.published=addr(nodes[0])
 ic=Context();ic.display=addr(d);ic.palette=addr(palette);ic.maps=addr(maps);ic.viewport=addr(view);ic.flags=addr(flags)
 tc=TeeContext();tc.intro=addr(ic);tc.objects=addr(objects);tc.x=addr(x);tc.tasks=addr(tasks);tc.fallback=addr(nodes[6])
 ctx=FireContext(addr(tc),addr(aim),addr(overlay));state=new_fire()
 def serial(b):
  serial_display(d,b);serial_objects(objects,x,b);serial_palette(palette,b);AM.serialize_aim(aim,b)
  write(b,0x400d,flags.value);write(b,0x413b,overlay.value);serial_tasks(nodes,addresses,tasks,b)
 expected=bytearray(ram[:16384]);serial(expected);put(0x4000,expected);video=bytearray(video_data())
 assert lib.ntm_shot_fire_initialize(C.byref(state),C.byref(nodes[0]),C.byref(ctx),C.byref(fire_resources))==0
 serial(expected)
 if occupied<8:write_job(expected,perm[occupied],palette.jobs[perm[occupied]],(fire_palette_command,fire_palette_descriptor))
 video[2:6]=bytes(view);video[0x1000:0x2000]=bytes(maps)
 low,end=run(0x209919);assert end==0xef00;check(expected,video,low,('entry',case));entries+=1
print(json.dumps({'entry_cases':entries,'scope':'full nonstack RAM and hardware/video; object-list permutations,0..3 free tasks,all palette queue limits; genuine allocations,children not dispatched'}),flush=True)
ticks=0
for phase,pc in ((1,0x2099a5),(2,0x2099e1),(3,0x2099f2)):
 for delay in [0,1,2,3,4,32767,32768,32769,65535]+[r.randrange(65536) for _ in range(8)]:
  for pressed in range(256):
   setup();put(0x4000,r.randbytes(16384));state=new_fire();state.phase=phase;task=Task();task.callback=pc;task.delay=delay
   aim=AM.read_aim();view=blob(data(0x8002,4));overlay=U8(load(0x413b,1));ic=Context();ic.viewport=addr(view);tc=TeeContext();tc.intro=addr(ic);ctx=FireContext(addr(tc),addr(aim),addr(overlay))
   store(0x41c9,0x4149,4);store(0x417a,0x4100,2);store(0x4101,pressed,1);store(0x6000,pc,4);store(0x6014,delay,2)
   expected=bytearray(ram[:16384]);video=bytearray(video_data())
   result=lib.ntm_shot_fire_tick(C.byref(state),C.byref(task),C.byref(ctx),pressed)
   write(expected,0x6000,task.callback,4);write(expected,0x6014,task.delay,2);write(expected,0x413b,overlay.value);video[2:6]=bytes(view)
   low,end=run(pc,(0x209a0a,0x209b71));assert result==(0 if end==0xef00 else 2)
   assert state.boundary==({0xef00:0,0x209a0a:1,0x209b71:2}[end]);check(expected,video,low,('tick',phase,delay,pressed));ticks+=1
print(json.dumps({'parent_ticks':ticks,'scope':'full nonstack RAM/hardware/video; all edge bytes, signed delay boundaries; stop before power209a0a/cancel209b71 without effects'}))
