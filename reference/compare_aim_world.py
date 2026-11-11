#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Unmodified201493/20151a/2015c0/20170e versus native bounded resources."""
from screen_probe import *
from aim_world_case_common import *
import random,os
lib=C.CDLL(os.environ.get('NTM_AIM_WORLD_LIBRARY',str(ROOT/'build/shot-probe/aim-world.so')))
lib.ntm_aim_world_load.argtypes=[P,P,U8,P,P,P,P,P]
R=random.Random(0x201493);keep=[]
def put(a,b):
 if 0x4000<=a and a+len(b)<=0x8000:C.memmove(C.addressof(ram.contents)+a-0x4000,bytes(b),len(b))
 else:
  for i,v in enumerate(b):store(a+i,v,1)
def data(a,n):return bytes(load(a+i,1) for i in range(n))
def diff(a,b,start):return [(hex(start+i),x,y) for i,(x,y) in enumerate(zip(a,b)) if x!=y][:20]
def run(offset,x,y,mode,layer):
 setup();put(0x4000,R.randbytes(16384));store(0x40fa,mode,1)
 for address,length in ((0x8000,0x40),(0x8100,0x19),(0x8200,512),(0x8800,256),(0x8c00,64),(0x9000,4096),(0xa000,8192)):put(address,R.randbytes(length))
 for a,v in ((0x5710,x),(0x5712,y),(0x573c,0x9000+2048*layer),(0x573e,0x8108+8*layer),(0x5740,0x8280+128*layer)):store(a,v,2)
 res=resolve_world(rom,offset,keep)
 state=WorldState.from_buffer_copy(data(0x5710,52));tiles=(U8*8192).from_buffer_copy(data(0xa000,8192));maps=(U8*4096).from_buffer_copy(data(0x9000,4096));pal=(U8*512).from_buffer_copy(data(0x8200,512));mono=(U8*24).from_buffer_copy(data(0x8100,24));bg=U8(load(0x8118,1))
 # The reference core reads SCRP1 palette0 at8108..810a but writes it
 # at8109..810b. Normalize only this test adapter; native keeps write layout.
 mono[11]=mono[10];mono[10]=mono[9];mono[9]=mono[8];mono[8]=0
 expected_ram=bytearray(ram[:16384]);expected_video=bytearray(data(0x8000,0x4000))
 rc=lib.ntm_aim_world_load(C.byref(state),C.byref(res),mode,tiles,maps,pal,mono,C.byref(bg));assert rc==0,('native',offset,x,y,mode,rc)
 expected_ram[0x1710:0x1744]=bytes(state)
 mono[8]=mono[9];mono[9]=mono[10];mono[10]=mono[11];mono[11]=0
 for a,b in ((0x8100,mono),(0x8118,bytes((bg.value,))),(0x8200,pal),(0x9000,maps),(0xa000,tiles)):expected_video[a-0x8000:a-0x8000+len(b)]=bytes(b)
 reg(0,offset);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x201493);low=0x6be0
 for n in range(10000):
  if get(8)==0xef00:break
  step(0);low=min(low,get(7))
 else:raise AssertionError(('budget',hex(get(8))))
 actual=bytes(ram[:16384]);expected_ram[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==expected_ram,('ram',offset,x,y,mode,diff(actual,expected_ram,0x4000))
 actual=data(0x8000,0x4000)
 assert actual==expected_video,('graphics',offset,x,y,mode,diff(actual,expected_video,0x8000))
 keep.clear()
cases=0
for index in range(108):
 for mode in (0,15,16,255):
  for layer in (0,1):
   run(index*18,160,240,mode,layer);cases+=1
coordinate_cases=0
for index in range(54):
 for x,y in ((0,0),(31,31),(32,32),(65520,240),(240,65520),(255,255),(256,256),(511,767)):
  run(index*18,x,y,16,index&1);coordinate_cases+=1
for index in (0,18,36,54,72,90):
 for x,y in ((32767,32767),(32768,32768),(65535,65535),(0,32768),(32768,0),(16383,49152)):
  run(index*18,x,y,255,index&1);coordinate_cases+=1
print(json.dumps({'cases':cases,'coordinate_cases':coordinate_cases,'scope':'108 original map records, both map layers, monochrome/color boundary modes; complete nonstack16KiB RAM including palette queues, complete16KiB hardware/video range'}))
