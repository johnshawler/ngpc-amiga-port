#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Unchanged21081d nearby feature collection, view filter, nearest selection."""
from shot_golfer_features_case_common import *
import os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_GOLFER_FEATURES_LIBRARY',str(ROOT/'build/shot-golfer-features-check/native.so')))
lib.ntm_shot_golfer_features_select.argtypes=[P,P,P]
r=random.Random(0x21081d);cases=0;visible=0;full=0;empty=0
def video_snapshot():return b''.join(load(a,4).to_bytes(4,'little') for a in range(0x8000,0xc000,4))
def helper(pc,regs):
 for j,v in enumerate(regs):reg(j,v)
 reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc)
 for _ in range(10000):
  if get(8)==0xef00:return
  step(0)
 raise AssertionError(('helper budget',hex(pc),hex(get(8))))
def check(h,res,source):
 global cases,visible,full,empty
 s=GolferFeatures.from_buffer_copy(r.randbytes(C.sizeof(GolferFeatures)))
 h.hole[0x36:0x3a]=source.to_bytes(4,'little')
 def serial(b):
  b[0x200:0x300]=bytes(h.hole);b[0x34e:0x39f]=bytes(s.nearby);b[0x39f:0x3f0]=bytes(s.view)
  for n,a,size in [('origin_x',0x4103,2),('origin_y',0x4105,2),('extent',0x4107,2),('lateral_bias',0x433b,4),('depth_bias',0x433f,4),('lateral_scratch',0x4343,4),('depth_scratch',0x4347,4)]:
   b[a-0x4000:a-0x4000+size]=getattr(s,n).to_bytes(size,'little')
 expected=bytearray(ram[:16384]);serial(expected);put(0x4000,expected);video=video_snapshot()
 rc=lib.ntm_shot_golfer_features_select(C.byref(s),C.byref(h),C.byref(res));assert rc==0,(cases,'native',rc)
 serial(expected);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x21081d);low=0x6be0
 for _ in range(100000):
  if get(8)==0xef00:break
  step(0);low=min(low,get(7))
 else:raise AssertionError(('budget',cases,hex(get(8))))
 actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==expected,(cases,'RAM',[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:30])
 assert video_snapshot()==video,(cases,'video')
 if s.view[0]!=255:visible+=1
 if s.nearby[0]==255:empty+=1
 if all(s.nearby[5*i]!=255 for i in range(16)):full+=1
 cases+=1
for case in range(768):
 setup();put(0x4000,r.randbytes(16384));h=State.from_buffer_copy(r.randbytes(C.sizeof(State)))
 center_x=(0,99,100,32767,32768,65535)[case%6];center_y=(0,99,100,32767,32768,65535)[(case//6)%6]
 if case>=384:center_x=r.randrange(65536);center_y=r.randrange(65536)
 for off,v in [(0x44,(center_x<<16)|r.randrange(65536)),(0x48,(center_y<<16)|r.randrange(65536))]:h.hole[off:off+4]=v.to_bytes(4,'little')
 if case<384:
  angle=(case*173)&4095;helper(0x20d676,[angle]);dx=get(0);helper(0x20d633,[angle]);dy=get(0)
 else:dx=r.getrandbits(32);dy=r.getrandbits(32)
 h.hole[0xd4:0xd8]=dx.to_bytes(4,'little');h.hole[0xd8:0xdc]=dy.to_bytes(4,'little')
 feature=bytearray()
 for j in range(case%34):
  ox=r.randrange(-100,101) if case%4==0 else (-101,-100,-99,0,99,100,101)[r.randrange(7)]
  oy=r.randrange(-100,101) if case%4==0 else (-101,-100,-99,0,99,100,101)[r.randrange(7)]
  feature+=bytes([j])+((center_x+ox)&65535).to_bytes(2,'little')+((center_y+oy)&65535).to_bytes(2,'little')
 feature+=b'\xff';array=(U8*len(feature)).from_buffer_copy(feature);put(0x7000,feature)
 check(h,GolferFeatureResources(C.addressof(array),len(array),C.addressof(aim_resources)),0x7000)
for index,(res,source) in enumerate(feature_resources):
 points=[(load(source+i+1,2),load(source+i+3,2)) for i in range(0,res.features_size-1,5)]
 for direction in range(8):
  setup();put(0x4000,r.randbytes(16384));h=State.from_buffer_copy(r.randbytes(C.sizeof(State)))
  x,y=points[(index+direction)%len(points)] if points else (500,500)
  for off,v in [(0x44,(((x-30)&65535)<<16)|r.randrange(65536)),(0x48,(((y-30)&65535)<<16)|r.randrange(65536))]:h.hole[off:off+4]=v.to_bytes(4,'little')
  helper(0x20d676,[direction*512]);h.hole[0xd4:0xd8]=get(0).to_bytes(4,'little')
  helper(0x20d633,[direction*512]);h.hole[0xd8:0xdc]=get(0).to_bytes(4,'little')
  check(h,res,source)
print(json.dumps({'feature_selections':cases,'actual_hole_lists':len(feature_resources),'visible_selections':visible,'empty_nearby_lists':empty,'capacity16_lists':full,'scope':'complete nonstack RAM and hardware/video;rectangle edges,wrapped/fractional positions,full-width directions,stale scratch,actual54 hole lists'}))
