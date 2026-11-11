#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Isolated original comparisons. User-supplied assets remain private.
from screen_probe import *
import random,json,hashlib
R=random.Random(0x202c77);U8=C.c_uint8;U16=C.c_uint16
class Job(C.Structure):_fields_=[('kind',U8),('pending_index',U8),('offset',U16),('count',U16),('color',U16),('control_offset',U16),('offsets',C.POINTER(U8)),('colors',C.POINTER(U8)),('offsets_size',C.c_size_t),('colors_size',C.c_size_t)]
class Copy(C.Structure):_fields_=[('offset',U16),('words',U16)]
class Palette(C.Structure):_fields_=[('slots',U16*8),('status',U16),('copy_bytes',U16),('jobs',Job*8),('copies',Copy*8),('staging',U8*512),('palette',U8*512),('pending',U8*2),('background',U8),('window_control',U8)]
native=C.CDLL(str(ROOT/'build/course-init-check/native_palette.so'))
native.ntm_course_palette_enqueue.argtypes=[C.POINTER(Palette),C.POINTER(Job)]
native.ntm_course_palette_tail.argtypes=[C.POINTER(Palette)];native.ntm_course_palette_head.argtypes=[C.POINTER(Palette)]
def data(a,n):return bytes(load(a+i,1) for i in range(n))
def buf(v):return (U8*len(v)).from_buffer_copy(v)
group_count=load(0x21a045,1);group_source=load(0x21a047,4)
assert 0<group_count<=64
indices=buf(data(group_source,group_count*2))
color_size=max(indices[2*i]+256*indices[2*i+1] for i in range(group_count))+8
colors=buf(data(load(0x26001c,4),color_size))
group=Job(1,0,load(0x21a046,1)*8,group_count,0,0,indices,colors,len(indices),len(colors))
single=Job(2,load(0x21a053+5,1),480+load(0x21a053,1),0,load(0x21a053+1,2),load(0x21a053+3,2),None,None,0,0)
def execute(pc):
 reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc)
 for i in range(20000):
  if get(8)==0xef00:return
  step(0)
 raise RuntimeError(('limit',hex(get(8))))
def setram(a,v):C.memmove(C.cast(C.addressof(ram.contents)+a-0x4000,C.c_void_p),bytes(v),len(v))
def serialize(s,b):
 def w(a,v,n=1):b[a-0x4000:a-0x4000+n]=int(v).to_bytes(n,'little')
 for i in range(8):w(0x66bf+2*i,s.slots[i],2);w(0x67cf+4*i,s.copies[i].offset,2);w(0x67d1+4*i,s.copies[i].words,2)
 w(0x66bd,s.status,2);w(0x67ef,s.copy_bytes,2);b[0x27f1:0x29f1]=bytes(s.staging);b[0x29f1:0x29f3]=bytes(s.pending)
 return b
def compare(s,expected=None):
 assert list(s.slots)==[load(0x66bf+2*i,2) for i in range(8)]
 assert s.status==load(0x66bd,2);assert s.copy_bytes==load(0x67ef,2)
 assert bytes(s.staging)==data(0x67f1,512);assert bytes(s.palette)==data(0x8200,512)
 assert bytes(s.pending)==data(0x69f1,2)
 assert (s.background,s.window_control)==(load(0x8118,1),load(0x8012,1)),(s.background,load(0x8118,1),s.window_control,load(0x8012,1))
 for i in range(8):assert (s.copies[i].offset,s.copies[i].words)==(load(0x67cf+4*i,2),load(0x67d1+4*i,2))
 if expected is not None:
  actual=bytes(ram[:16384]);assert actual[:0x2bc0]==expected[:0x2bc0] and actual[0x2be4:]==expected[0x2be4:],[(hex(0x4000+i),x,y) for i,(x,y) in enumerate(zip(actual,expected)) if x!=y and not 0x2bc0<=i<0x2be4][:12]
