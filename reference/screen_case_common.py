#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# User-supplied original stays private. Run tools/check-course-screen.sh.
import sys,random,json,hashlib
from screen_probe import *
R=random.Random(0x200150);U8=C.c_uint8;U16=C.c_uint16
class Asset(C.Structure):_fields_=[('data',C.POINTER(U8)),('size',C.c_size_t),('tiles',C.POINTER(U8)),('tiles_size',C.c_size_t)]
class Stamp(C.Structure):_fields_=[('asset',Asset),('offset',U16),('tile',U16)]
class Fill(C.Structure):_fields_=[('offset',U16),('fill',U16),('width',U8),('height',U8)]
class Screen(C.Structure):_fields_=[('fill_status',U8*8),('fills',Fill*8),('stamp_bytes',U16),('stamps',Stamp*32),('cached_selection',U16),('cursor_x',U16),('displayed_selection',U8)]
native=C.CDLL(str(ROOT/'build/course-screen-check/native_screen.so'))
native.ntm_course_screen_tick.argtypes=[C.POINTER(Screen),U16,C.POINTER(Asset)]
native.ntm_course_screen_fill_queue.argtypes=[C.POINTER(Screen),C.POINTER(U8)]
native.ntm_course_screen_stamp_queue.argtypes=[C.POINTER(Screen),C.POINTER(U8),C.POINTER(U8)]
def run(pc):
 reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc)
 for i in range(100000):
  if get(8)==0xef00:return
  step(0)
 raise RuntimeError(('limit',hex(get(8))))
def data(a,n):return bytes(load(a+i,1) for i in range(n))
def setmem(a,v):
 for i,c in enumerate(v):store(a+i,c,1)
def blob(v):return (U8*len(v)).from_buffer_copy(v)
def maplen(src):
 n=load(src,2);q=src+6+2*n;rows=load(q,1)&127;q+=1
 for y in range(rows):
  while load(q,2)!=65535:q+=2
  q+=2
 return q-src
assets=(Asset*3)();retained=[];sources=[]
for s in range(3):
 table=load(load(0x260020,4)+24,4)&0xffffff;src=load(table+4*(s+2),4);base=load(src+2,4);maxoff=max(load(src+6+2*i,2) for i in range(load(src,2)))
 a=blob(data(src,maplen(src)));b=blob(data(base,maxoff+16));retained.extend([a,b]);assets[s]=Asset(a,len(a),b,len(b));sources.append(src)
