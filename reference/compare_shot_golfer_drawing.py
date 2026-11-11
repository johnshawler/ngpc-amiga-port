#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Unchanged full20a5aa initial drawing, no patched helper or synthetic child."""
from shot_golfer_drawing_case_common import *
import os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_GOLFER_DRAWING_LIBRARY',str(ROOT/'build/shot-golfer-drawing-check/native.so')))
lib.ntm_shot_golfer_draw_initialize.argtypes=[P,P,P,P,P,P]
lib.ntm_shot_golfer_project_feature.argtypes=[U16,U16,P,P]
lib.ntm_course_screen_stamp_queue.argtypes=[P,P,P]
r=random.Random(0x20a5aa);cases=0;drains=0;registered=0;variants={};project_cases=0
def video_snapshot():return b''.join(load(a,4).to_bytes(4,'little') for a in range(0x8000,0xc000,4))
def run(pc,expected,video):
 reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
 for _ in range(100000):
  if get(8)==0xef00:break
  step(0);low=min(low,get(7))
 else:raise AssertionError(('budget',cases,hex(pc),hex(get(8))))
 actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==expected,(cases,hex(pc),'RAM',[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:30])
 actual=video_snapshot()
 assert actual==video,(cases,hex(pc),'video',[(hex(0x8000+i),a,b) for i,(a,b) in enumerate(zip(actual,video)) if a!=b][:30])
def helper(pc,value):
 reg(0,value);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc)
 for _ in range(1000):
  if get(8)==0xef00:return get(0)
  step(0)
 raise AssertionError('direction budget')
def check(h,res,source):
 global cases,drains,registered
 h.hole[0x36:0x3a]=source.to_bytes(4,'little');golfer_drawing_resources.features=res
 d=Display.from_buffer_copy(r.randbytes(C.sizeof(Display)));objects=(Cursor*32).from_buffer_copy(r.randbytes(C.sizeof(Cursor)*32));x=(U16*32)(*[r.randrange(65536) for _ in range(32)])
 full=cases%3==0;ids=[j for j in range(32) if j!=18 or full];r.shuffle(ids);d.zero_flags[8]=len(ids);d.zero_words[0]=2*len(ids)
 for j in range(32):
  d.list[j]=ids[j] if j<len(ids) else r.randrange(65536);objects[j].id=d.object_ids[j]=j;objects[j].tag=d.object_tags[j]
 screen=Screen();screen.stamp_bytes=(0,248,256)[cases%3];before=screen.stamp_bytes//8
 words=(U16*2)(r.randrange(65536),r.randrange(65536));video=bytearray(video_snapshot());maps=blob(video[0x1000:0x2000]);tiles=blob(video[0x2000:0x4000]);saved=blob(r.randbytes(32))
 ic=Context();ic.display=addr(d);ic.screen=addr(screen);ic.maps=addr(maps);ic.screen_words=addr(words)
 tc=TeeContext();tc.intro=addr(ic);tc.objects=addr(objects);tc.x=addr(x);tc.tiles=addr(tiles)
 scene=SceneObjectContext(addr(tc),addr(h),None,None,addr(saved));g=Golfer.from_buffer_copy(r.randbytes(C.sizeof(Golfer)))
 g.drawing=0x20a5aa;g.phase=1;g.surface=cases%15;g.task_bytes[:]=[2*(cases%4),(1,0,255)[cases%3],5,(0,4,8,12,252)[cases%5],2*((cases//4)%4),1,7,(0,252)[cases%2]]
 s=GolferDrawing.from_buffer_copy(r.randbytes(C.sizeof(GolferDrawing)));s.phase=0;task=Task();task.callback=g.drawing
 def serial(b):
  serial_display(d,b);serial_objects(objects,x,b);b[0x200:0x300]=bytes(h.hole);b[0x4ff:0x51f]=bytes(saved)
  b[0x2016:0x201e]=bytes(g.task_bytes);write(b,0x6000,task.callback,4);write(b,0x4534,g.surface)
  write(b,0x4535,s.center_x,2);write(b,0x4537,s.half_width,2);write(b,0x4d84,screen.stamp_bytes,2)
  for j in range(2):write(b,0x4d80+2*j,words[j],2)
  b[0x34e:0x39f]=bytes(s.features.nearby);b[0x39f:0x3f0]=bytes(s.features.view)
  for n,a,size in [('origin_x',0x4103,2),('origin_y',0x4105,2),('extent',0x4107,2),('lateral_bias',0x433b,4),('depth_bias',0x433f,4),('lateral_scratch',0x4343,4),('depth_scratch',0x4347,4)]:write(b,a,getattr(s.features,n),size)
 expected=bytearray(ram[:16384]);serial(expected);put(0x4000,expected)
 rc=lib.ntm_shot_golfer_draw_initialize(C.byref(s),C.byref(g),C.byref(task),C.byref(scene),C.byref(golfer_drawing_resources),0x20a5c0)
 assert rc==0,(cases,'native',rc);assert s.phase==1 and task.callback==0x20a5c0
 serial(expected);video[0x1000:0x2000]=bytes(maps);video[0x2000:0x4000]=bytes(tiles)
 for j in range(before,screen.stamp_bytes//8):
  st=screen.stamps[j];src=golfer_animation_sources[C.cast(st.asset.data,P).value]
  write(expected,0x4d86+8*j,src,4);write(expected,0x4d8a+8*j,0x9000+st.offset,2);write(expected,0x4d8c+8*j,st.tile,2)
 run(0x20a5aa,expected,video)
 if objects[18].tag==52:registered+=1
 key='none' if s.features.view[0]==255 else str(next(load(0x21b975+i+2,2) for i in range(0,20,4) if word_view(s.features.view,3)<=load(0x21b975+i,2)))
 variants[key]=variants.get(key,0)+1;cases+=1
 if before==0 and screen.stamp_bytes:
  assert lib.ntm_course_screen_stamp_queue(C.byref(screen),tiles,maps)==0
  write(expected,0x4d84,screen.stamp_bytes,2);video[0x1000:0x2000]=bytes(maps);video[0x2000:0x4000]=bytes(tiles)
  run(0x20054f,expected,video);drains+=1
def word_view(b,n):return b[n]+256*b[n+1]
for case in range(768):
 setup();put(0x4000,r.randbytes(16384));put(0x9000,r.randbytes(4096));h=AM.State.from_buffer_copy(r.randbytes(C.sizeof(AM.State)))
 x0,y0=200,200;distance=(0,1,2,55,56,57,71,72,73,87,88,89,100)[case%13];lateral=(-100,-80,-56,-32,0,32,56,80,100)[(case//13)%9]
 angle=(0,512,1024,1536,2048,2560,3072,3584)[(case//117)%8]
 for off,v in [(0x44,x0<<16),(0x48,y0<<16),(0xd4,helper(0x20d676,angle)),(0xd8,helper(0x20d633,angle))]:h.hole[off:off+4]=v.to_bytes(4,'little')
 feature=bytes([case%255])+(x0+distance).to_bytes(2,'little')+(y0+lateral).to_bytes(2,'little')+b'\xff'
 a=blob(feature);put(0x7000,feature);check(h,GolferFeatureResources(addr(a),len(a),addr(AM.aim_resources)),0x7000)
for index,(res,source) in enumerate(feature_resources):
 points=[(load(source+i+1,2),load(source+i+3,2)) for i in range(0,res.features_size-1,5)]
 for direction in range(8):
  setup();put(0x4000,r.randbytes(16384));put(0x9000,r.randbytes(4096));h=AM.State.from_buffer_copy(r.randbytes(C.sizeof(AM.State)))
  x0,y0=points[(index+direction)%len(points)] if points else (500,500)
  for off,v in [(0x44,(((x0-30)&65535)<<16)|r.randrange(65536)),(0x48,(((y0-30)&65535)<<16)|r.randrange(65536)),(0xd4,helper(0x20d676,direction*512)),(0xd8,helper(0x20d633,direction*512))]:h.hole[off:off+4]=v.to_bytes(4,'little')
  check(h,res,source)
for lateral,depth in [(r.randrange(65536),r.randrange(65536)) for _ in range(2048)]+[(a,b) for a in (0,1,32767,32768,65535) for b in (0,1,15,16,32767,32768,65520,65535)]:
 setup();put(0x4000,r.randbytes(16384));store(0x43a0,lateral,2);store(0x43a2,depth,2)
 expected=bytearray(ram[:16384]);video=video_snapshot();center=U16();scratch=U32()
 assert lib.ntm_shot_golfer_project_feature(lateral,depth,C.byref(center),C.byref(scratch))==0
 write(expected,0x5bcb,scratch.value,4);run(0x20fa2f,expected,video);assert (get(0)&65535)==center.value;project_cases+=1
print(json.dumps({'drawing_callbacks':cases,'immediate_green_variants':variants,'tag52_registrations':registered,'drawing_head_drains':drains,'feature_projection_cases':project_cases,'actual_hole_lists':len(feature_resources),'scope':'full nonstack RAM/video;real immediate art,feature selection,clipped row copy,pin/tree,save-map,callback and same-call animation'}))
