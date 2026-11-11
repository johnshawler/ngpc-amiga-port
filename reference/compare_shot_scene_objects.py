#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exact unchanged-original209f5c/20a170/20a214/20ab9f comparison."""
from shot_scene_objects_case_common import *
import os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_SCENE_OBJECT_LIBRARY',str(ROOT/'build/shot-scene-check/native.so')))
R=random.Random(0x209830);cases=0
entries=[('ball',0x209f5c),('pin',0x20a170),('gauges',0x20a214),('save_map',0x20ab9f)]
for name,pc in entries:
 fn=getattr(lib,'ntm_shot_scene_'+name);fn.argtypes=[C.POINTER(SceneObjectContext)]+([] if name=='save_map' else [C.POINTER(SceneObjectResources)])
 for case in range(256):
  setup();put(0x4000,R.randbytes(16384));put(0x9000,R.randbytes(4096))
  d=Display.from_buffer_copy(R.randbytes(C.sizeof(Display)));objects=(Cursor*32).from_buffer_copy(R.randbytes(C.sizeof(Cursor)*32));x=(U16*32)(*[R.randrange(65536) for _ in range(32)])
  h=AM.State.from_buffer_copy(R.randbytes(C.sizeof(AM.State)));p=AM.Player(0,case%2,1-case%2,0);aim=AM.Aim();aim.club=case%16
  words=(U16*2)(*[R.randrange(65536) for _ in range(2)]);maps=blob(data(0x9000,4096));saved=blob(data(0x44ff,32))
  slots={'ball':(0,1,17),'pin':(5,6),'gauges':(7,8,9,10),'save_map':()}[name]
  available=[j for j in range(32) if j not in slots]
  count=0 if case%3==0 else len(available) if case%3==1 else 32
  d.zero_flags[8]=count;d.zero_words[0]=2*count;d.capacity=34;d.zero_flags[0]=0
  for j in range(count):d.list[j]=j if count==32 else available[j]
  for j,o in enumerate(objects):o.id=d.object_ids[j]=j;o.tag=d.object_tags[j]
  # Exercise absent, completed, on-tee and off-tee other-player paths.
  other=h.players[p.other_player];other[1]=255 if case%4==0 else case%6;other[0x22]=int(case%4==1)
  if case%4==2:
   other[6:8]=h.hole[0x2e:0x30];other[10:12]=h.hole[0x30:0x32]
  ic=Context();ic.display=addr(d);ic.screen_words=addr(words);ic.maps=addr(maps)
  tc=TeeContext();tc.intro=addr(ic);tc.objects=addr(objects);tc.x=addr(x)
  ctx=SceneObjectContext(addr(tc),addr(h),addr(p),addr(aim),addr(saved))
  def serial(b):
   serial_display(d,b)
   for j,o in enumerate(objects):
    for n,off in [('tag',2),('y',6),('width',8),('velocity',10),('tile',18),('link',28)]:write(b,0x453b+64*j+off,getattr(o,n),2)
    for n,off in [('id',0),('second',1),('byte14',14),('byte15',15),('flags',16),('byte30',30)]:write(b,0x453b+64*j+off,getattr(o,n))
    write(b,0x453f+64*j,x[j],2)
   b[0x149:0x189]=bytes(h.players[0]);b[0x189:0x1c9]=bytes(h.players[1]);b[0x200:0x300]=bytes(h.hole)
   write(b,0x41c9,0x4149+64*p.active_player,4);write(b,0x41cd,0x4149+64*p.other_player,4);write(b,0x41dc,aim.club)
   for j in range(2):write(b,0x4d80+2*j,words[j],2)
   b[0x4ff:0x51f]=bytes(saved)
  expected=bytearray(ram[:16384]);serial(expected);put(0x4000,expected)
  result=fn(C.byref(ctx),*(() if name=='save_map' else (C.byref(scene_object_resources),)))
  assert result==0,(name,case,'native',result)
  serial(expected)
  reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
  for _ in range(10000):
   if get(8)==0xef00:break
   step(0);low=min(low,get(7))
  else:raise AssertionError(('budget',name,hex(get(8))))
  actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
  assert actual==expected,(name,case,[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:20])
  cases+=1
print(json.dumps({'cases':cases,'scope':'four scene producers full nonstack RAM, saturated registration, other-player tee branches, signed coordinates and HUD power'}))
