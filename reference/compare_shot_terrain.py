#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Unchanged original terrain queries versus bounded native state and resources."""
from screen_probe import *
import random,os
U8=C.c_uint8;U16=C.c_uint16;P=C.c_void_p
class Map(C.Structure):_fields_=[('data',P),('size',C.c_size_t)]
class Resources(C.Structure):_fields_=[('large',Map),('close',Map),('masks',P),('shapes',P),('masks_size',C.c_size_t),('shapes_size',C.c_size_t)]
class Result(C.Structure):_fields_=[('x',U16),('y',U16),('detail',U16),('terrain',U8)]
lib=C.CDLL(os.environ.get('NTM_TERRAIN_LIBRARY',str(ROOT/'build/shot-probe/terrain.so')))
lib.ntm_shot_terrain.argtypes=[P,U8,U16,U16,C.POINTER(Resources),C.POINTER(Result)]
R=random.Random(0x20fb18);keep=[]
def span(b):
 a=(U8*len(b)).from_buffer_copy(b);keep.append(a);return C.addressof(a)
def put(a,b):C.memmove(C.addressof(ram.contents)+a-0x4000,bytes(b),len(b))
def data(a,n):return bytes(load(a+i,1) for i in range(n))
def mapdata(seed):
 r=random.Random(seed);b=bytearray(128)
 for row in range(64):
  b[row*2:row*2+2]=len(b).to_bytes(2,'little')
  for col in range(8):b+=bytes((8,r.randrange(256),r.randrange(256)))
 return b
large=mapdata(1);close=mapdata(2);assert len(large)<=2048
res=Resources(Map(span(large),len(large)),Map(span(close),len(close)),span(data(0x223104,8)),span(data(0x22310c,2048)),8,2048)
cases=0
for status in (0,1,254,255):
 for x,y in [(x,y) for x in (0,1,9,10,19,20,21,31,55,56,75,76,100,111,112,119,120,121,175,176,199,200,311,312,350,351,352,65535) for y in (0,1,7,8,19,20,21,55,56,75,76,100,119,120,121,175,176,199,200,311,312,350,351,352,65535)]+[(R.randrange(400),R.randrange(400)) for _ in range(1000)]:
  if status==255 and not(100<=x<348 and 100<=y<352):continue
  setup();put(0x4000,R.randbytes(16384));put(0x7000,large);put(0x7800,close)
  store(0x5730,0x7000,4);store(0x4240,0x7800,4);store(0x41db,status,1)
  for a,v in ((0x422a,200),(0x422c,200),(0x422e,20),(0x4230,20),(0x423c,100),(0x423e,100)):store(a,v,2)
  hole=(U8*256).from_buffer_copy(data(0x4200,256));out=Result(load(0x5bcb,2),load(0x5bcd,2),load(0x4411,2),91)
  expected=bytearray(ram[:16384]);rc=lib.ntm_shot_terrain(hole,status,x,y,C.byref(res),C.byref(out));assert rc==0,(status,x,y,rc)
  for a,v in ((0x5bcb,out.x),(0x5bcd,out.y),(0x4411,out.detail)):expected[a-0x4000:a-0x4000+2]=v.to_bytes(2,'little')
  reg(0,x);reg(1,y);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x20fb18);low=0x6be0
  for i in range(1000):
   if get(8)==0xef00:break
   step(0);low=min(low,get(7))
  else:raise AssertionError(('budget',hex(get(8))))
  actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
  assert (get(0)&255)==out.terrain,(status,x,y,'terrain',get(0)&255,out.terrain)
  assert actual==expected,(status,x,y,[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:15])
  cases+=1
report={'cases':cases,'scope':'original20fb18 including hazard, large/close row RLE, per-pixel terrain, complete nonstack RAM; early state preservation and coordinate boundaries'}
classification_cases=0
for code in range(256):
 for index in (0,1,127,128,255):
  for x in range(8):
   for y in range(8):
    setup();reg(0,x);reg(1,y);reg(2,code+256*index);store(0x4411,0xa5a5,2);reg(8,0x20fbbd)
    keep[0][129]=code;keep[0][130]=index
    out=Result(90,91,0xa5a5,99)
    assert lib.ntm_shot_terrain(hole,0,x,y,C.byref(res),C.byref(out))==0
    for i in range(100):
     if get(8)==0xef00:break
     step(0)
    else:raise AssertionError('classification budget')
    assert (get(0)&255,load(0x4411,2))==(out.terrain,out.detail),(code,index,x,y)
    classification_cases+=1
report['classification_cases']=classification_cases
print(json.dumps(report))
