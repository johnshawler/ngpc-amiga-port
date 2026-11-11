#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original animated group/background palettes, real ordering and upload."""
from course_palette_case_common import *
import os
P=C.POINTER(U8)
class AnimatedJob(C.Structure):
 _fields_=[(n,U8) for n in ('kind','speed','pending_index')]+[(n,U16) for n in ('offset','count','source_color','target_color','control_offset')]+[(n,P) for n in ('source','target','blend')]+[(n,C.c_size_t) for n in ('source_size','target_size','blend_size')]
class Animated(C.Structure):_fields_=[('jobs',AnimatedJob*8),('fraction',U8*8),('level',U8*8)]
lib=C.CDLL(os.environ.get('NTM_SHOT_PALETTE_LIBRARY',str(ROOT/'build/shot-probe/palette.so')))
lib.ntm_shot_palette_enqueue.argtypes=[C.POINTER(Palette),C.POINTER(Animated),C.POINTER(AnimatedJob)]
lib.ntm_shot_palette_tail.argtypes=[C.POINTER(Palette),C.POINTER(Animated)]
lib.ntm_course_palette_head.argtypes=[C.POINTER(Palette)]
lib.ntm_course_palette_enqueue.argtypes=[C.POINTER(Palette),C.POINTER(Job)]
keep=[]
def colors(table,count):return b''.join(data(load(0x26001c,4)+load(table+2*i,2),8) for i in range(count))
blend=buf(data(0x2031ad-16,512));source=buf(colors(0x21a6bd,16));target=buf(colors(0x21addd,16));keep.extend((blend,source,target))
groups=AnimatedJob(24,16,0,256,16,0,0,0,source,target,blend,len(source),len(target),512)
background=AnimatedJob(40,16,0,486,0,0x5ab,0,0x118,None,None,blend,0,0,512)
cases=rounds=0
for case in range(128):
 setup();setram(0x4000,R.randbytes(16384));store(0x40fa,16,1)
 s=Palette();a=Animated();permutation=list(range(8));R.shuffle(permutation)
 for i,k in enumerate(permutation):s.slots[i]=0xff00+4*k;store(0x66bf+2*i,s.slots[i],2)
 for i in range(8):
  s.copies[i]=Copy(2*i,1);store(0x67cf+4*i,2*i,2);store(0x67d1+4*i,1,2)
 s.status=load(0x66bd,2);s.copy_bytes=0;store(0x67ef,0,2)
 s.staging[:]=data(0x67f1,512);s.palette[:]=data(0x8200,512)
 s.pending[:]=[255,255];store(0x69f1,255,1);store(0x69f2,255,1)
 s.background=load(0x8118,1);s.window_control=load(0x8012,1)
 jobs=[(0x6c,groups),(0x70,background)];R.shuffle(jobs)
 for i,(code,job) in enumerate(jobs):
  index=permutation[i];reg(0,code);execute(0x2037a8)
  j=AnimatedJob.from_buffer_copy(job);j.speed=(0,1,7,15,16,17,31,127,255)[case%9]
  if j.kind==40:
   j.source_color=R.randrange(4096);j.target_color=R.randrange(4096)
   j.control_offset=0x12 if case&2 else 0x118;j.pending_index=1 if case&2 else 0
   store(0x66cf+32*index+12,j.target_color,2);store(0x66cf+32*index+18,j.source_color,2)
   store(0x66cf+32*index+14,j.control_offset,2);store(0x66cf+32*index+26,j.pending_index,1)
  store(0x66cf+32*index+6,j.speed,1)
  assert lib.ntm_shot_palette_enqueue(C.byref(s),C.byref(a),C.byref(j))==0
  compare(s)
 if case&1:
  reg(0,0x124);execute(0x2037a8);assert lib.ntm_course_palette_enqueue(C.byref(s),C.byref(group))==0;compare(s)
 for tick in range(40):
  expected=bytearray(ram[:16384]);assert lib.ntm_shot_palette_tail(C.byref(s),C.byref(a))==0;serialize(s,expected)
  for k in permutation[:2]:expected[0x26cf+32*k+7]=a.fraction[k];expected[0x26cf+32*k+16]=a.level[k]
  execute(0x202c77);compare(s,expected)
  expected=bytearray(ram[:16384]);assert lib.ntm_course_palette_head(C.byref(s))==0;serialize(s,expected);execute(0x202bce);compare(s,expected);rounds+=1
 cases+=1
print(json.dumps({'cases':cases,'tail_head_rounds':rounds,'scope':'original kinds24/40, shuffled allocation, 9 step speeds, mixed legacy groups, complete nonstack RAM and all palette/control uploads'}))
