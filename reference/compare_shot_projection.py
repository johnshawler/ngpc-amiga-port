#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Unchanged20f98a including signed arithmetic/helpers and full nonstack RAM."""
from screen_probe import *
import os,random
U32=C.c_uint32;U16=C.c_uint16;P=C.c_void_p
class Projection(C.Structure):_fields_=[('x',U16),('y',U16),('depth',U32),('lateral_scratch',U32),('depth_scratch',U32)]
lib=C.CDLL(os.environ.get('NTM_SHOT_PROJECTION_LIBRARY',str(ROOT/'build/shot-projection-check/native.so')))
lib.ntm_shot_project.argtypes=[U32,U32,U32,U32,U32,U32,P]
r=random.Random(0x20f98a);cases=0
values=[0,1,65535,65536,0x7fffffff,0x80000000,0xffffffff]
inputs=[tuple(r.getrandbits(32) for _ in range(6)) for _ in range(2048)]
inputs += [(x,y,0,0,dx,dy) for x in values for y in values for dx,dy in ((0,0),(65536,0),(0,65536),(0xffff0000,0),(0,0xffff0000),(0x80000000,0x7fffffff))]
for case,(x,y,ox,oy,dx,dy) in enumerate(inputs):
 setup();raw=r.randbytes(16384);C.memmove(C.addressof(ram.contents),raw,len(raw));reg(0,x);reg(1,y)
 for a,v in ((0x4250,ox),(0x4254,oy),(0x4333,dx),(0x4337,dy)):store(a,v,4)
 expected=bytearray(ram[:16384]);out=Projection();assert lib.ntm_shot_project(x,y,ox,oy,dx,dy,C.byref(out))==0
 for a,v in ((0x5bcb,out.lateral_scratch),(0x5bcf,out.depth_scratch)):expected[a-0x4000:a-0x4000+4]=v.to_bytes(4,'little')
 reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x20f98a);low=0x6be0
 for _ in range(10000):
  if get(8)==0xef00:break
  step(0);low=min(low,get(7))
 else:raise AssertionError(('budget',case,hex(get(8))))
 actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert (get(0)&65535,get(1)&65535,get(2))==(out.x,out.y,out.depth),(case,'registers',(get(0)&65535,get(1)&65535,get(2)),(out.x,out.y,out.depth))
 assert actual==expected,(case,[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:15]);cases+=1
print(json.dumps({'projection_cases':cases,'scope':'all nonstack RAM and projected outputs; full-width random and signed/zero/division boundary inputs'}))
