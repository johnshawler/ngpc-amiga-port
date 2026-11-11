#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Unchanged original20c8a5 and201608 against native camera and map scrolling."""
from shot_camera_case_common import *
import os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_CAMERA_LIBRARY',str(ROOT/'build/shot-probe/camera.so')))
lib.ntm_shot_camera_active.argtypes=[P,P,P,P,P,U8]
lib.ntm_shot_camera_scroll.argtypes=[P,P,P,P]
R=random.Random(0x20c8a5)
def run(pc,budget=100000):
 reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
 for _ in range(budget):
  if get(8)==0xef00:return low
  step(0);low=min(low,get(7))
 raise AssertionError(('budget',hex(get(8))))
def diff(a,b,base):return [(hex(base+i),x,y) for i,(x,y) in enumerate(zip(a,b)) if x!=y][:20]
def check(expected,low,label):
 actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==expected,(label,diff(actual,expected,0x4000))
active_cases=0
for mode in (0,2,4):
 for case in range(1024):
  setup();put(0x4000,R.randbytes(16384))
  objects=(Cursor*32).from_buffer_copy(R.randbytes(C.sizeof(Cursor)*32));x=(U16*32)(*[R.randrange(65536) for _ in range(32)])
  h=AM.State.from_buffer_copy(R.randbytes(C.sizeof(AM.State)));aim=AM.read_aim();aim.reset_d1=0;aim.reset_4416=mode
  words=(U16*2)(*[R.randrange(65536) for _ in range(2)]);viewport=blob(bytes(load(0x8002+i,1) for i in range(4)))
  ic=Context();ic.screen_words=addr(words);ic.viewport=addr(viewport)
  tc=TeeContext();tc.intro=addr(ic);tc.objects=addr(objects);tc.x=addr(x)
  dirty=U8(case&255);ctx=MarkerContext(addr(tc),addr(h),addr(aim),addr(dirty),case&255,case&255)
  state=CameraState(R.randrange(65536),R.randrange(65536));task=Task();task.callback=0x20c8a5;tasks=TaskContext()
  if case%7==0:state.saved_x=aim.origin_x;state.saved_y=aim.origin_y
  pressed=(case//2)&255
  def serial(b):
   for j,o in enumerate(objects):
    for n,off in [('tag',2),('y',6),('width',8),('velocity',10),('tile',18),('link',28)]:write(b,0x453b+64*j+off,getattr(o,n),2)
    for n,off in [('id',0),('second',1),('byte14',14),('byte15',15),('flags',16),('byte30',30)]:write(b,0x453b+64*j+off,getattr(o,n))
    write(b,0x453f+64*j,x[j],2)
   b[0x200:0x300]=bytes(h.hole);AM.serialize_aim(aim,b);write(b,0x41da,dirty.value)
   for j in range(2):write(b,0x4d80+2*j,words[j],2)
   write(b,0x4417,state.saved_x,2);write(b,0x4419,state.saved_y,2)
   write(b,0x41c9,0x4149,4);write(b,0x417a,0x4100,4);write(b,0x4100,ctx.held);write(b,0x4101,pressed);write(b,0x6000,task.callback,4)
  expected=bytearray(ram[:16384]);serial(expected);put(0x4000,expected);video=bytearray(data(0x8000,0x4000))
  assert lib.ntm_shot_camera_active(C.byref(state),C.byref(task),C.byref(tasks),C.byref(ctx),C.byref(marker_resources),pressed)==0
  serial(expected);video[2:6]=bytes(viewport)
  low=run(0x20c8a5);check(expected,low,('camera',mode,case))
  actual=data(0x8000,0x4000);assert actual==video,('camera video',mode,case,diff(actual,video,0x8000))
  active_cases+=1
print(json.dumps({'active_cases':active_cases,'scope':'full nonstack RAM and hardware/video; all0/2/4 states,all held/edge bytes,wrapped signed bounds,real miniature/gauge refresh'}),flush=True)
scroll_cases=0;keep=[]
for index in range(108):
 res=resolve_world(rom,index*18,keep)
 coordinates=[(64,64,0,0),(64,64,31,31),(64,64,32,32),(64,64,-32,-32),(64,64,-33,-33),(64,64,150,-150),(64,64,-150,150),(65504,0,32,-33)]
 if index%18==0:
  coordinates += [(32736,32736,32,32),(32768,32768,-33,-33),(65504,65504,32,32),(0,0,-33,-33),(0,64,32768,0),(32768,64,-32768,0),(0,64,32767,0),(0,64,-32767,0)]
 for case,(x0,y0,dx,dy) in enumerate(coordinates):
  setup();put(0x4000,R.randbytes(16384));maps=blob(R.randbytes(4096))
  for i,v in enumerate(maps):store(0x9000+i,v,1)
  state=WorldState.from_buffer_copy(data(0x5710,52));b=bytearray(state.bytes)
  for off,v,n in [(0,(x0+dx)&65535,2),(2,(y0+dy)&65535,2),(4,x0,2),(6,y0,2),(8,res.layout_tag,4),(12,res.blocks_tag,4),(16,res.width,1),(17,res.height,1),(44,0x9000+2048*(case&1),2)]:b[off:off+n]=v.to_bytes(n,'little')
  state=WorldState.from_buffer_copy(b);put(0x5710,b);scroll=(U8*2)(load(0x8034,1),load(0x8035,1))
  expected=bytearray(ram[:16384]);video=bytearray(data(0x8000,0x4000))
  assert lib.ntm_shot_camera_scroll(C.byref(state),C.byref(res),maps,scroll)==0,('native scroll',index,case)
  expected[0x1710:0x1744]=bytes(state);video[0x1000:0x2000]=bytes(maps);video[0x34:0x36]=bytes(scroll)
  low=run(0x201608);check(expected,low,('scroll',index,case))
  actual=data(0x8000,0x4000);assert actual==video,('scroll video',index,case,diff(actual,video,0x8000))
  scroll_cases+=1
 keep.clear()
print(json.dumps({'scroll_cases':scroll_cases,'scope':'all108 map records,both layers,32-pixel asymmetry,diagonals,signed coordinate and difference extremes;full nonstack RAM and hardware/video'}))
