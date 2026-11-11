#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Private original comparison; no cartridge bytes or captured assets embedded.
from cursor_case_common import *
import ctypes as C,hashlib
U8=C.c_uint8;U16=C.c_uint16;U32=C.c_uint32
class Display(C.Structure):_fields_=[('zero_flags',U8*16),('capacity',U8),('limit',U8),('object_ids',U8*32),('pools',(U8*256)*2),('zero_words',U16*2),('object_tags',U16*32),('list',U16*32),('reset_words',U32*2)]
class Cursor(C.Structure):_fields_=[(n,U16) for n in ('tag','y','width','velocity','tile','link')]+[(n,U8) for n in ('id','second','byte14','byte15','flags','byte30')]
class Animation(C.Structure):_fields_=[('data',C.POINTER(U8)),('size',C.c_size_t),('frames',U8)]
class Assets(C.Structure):_fields_=[('animations',Animation*2),('shapes',C.POINTER(U8)),('tiles',C.POINTER(U8)),('shapes_size',C.c_size_t),('tiles_size',C.c_size_t)]
class Job(C.Structure):_fields_=[('shape_offset',U16),('tile',U16),('count',U8)]
class Engine(C.Structure):_fields_=[('frame',U8),('delay',U8),('list_dirty_high',U8),('uploaded_bank',U8),('bounds',U16*4),('bank1',(U8*256)*2),('palettes',((U8*64)*2)*2),('jobs',Job*64)]
lib=C.CDLL(str(ROOT/'build/cursor-check/native_cursor_final.so'));lib.ntm_course_cursor_tail.argtypes=[C.POINTER(Engine),C.POINTER(Cursor),C.POINTER(Display),U16,C.POINTER(U16),C.POINTER(U8),C.POINTER(Assets)];lib.ntm_course_cursor_head.argtypes=[C.POINTER(Engine),C.POINTER(Display),C.POINTER(U8),C.POINTER(U8),C.POINTER(U8),C.POINTER(Assets)];lib.ntm_display_reset.argtypes=[C.POINTER(Display)]
zero_addresses=[0x66bb,0x61ba,0x61b4,0x61b5,0x61b7,0x61b8,0x4d80,0x4d82,0x4d7d,0x4d7e,0x6182,0x619d,0x61a9,0x61a6,0x61a7,0x61a8]
curmap=[('tag',0x453d,2),('y',0x4541,2),('width',0x4543,2),('velocity',0x4545,2),('tile',0x454d,2),('link',0x4557,2),('id',0x453b,1),('second',0x453c,1),('byte14',0x4549,1),('byte15',0x454a,1),('flags',0x454b,1),('byte30',0x4559,1)]
def buf(b):return (U8*len(b)).from_buffer_copy(b)
a0=buf(data(anim,40));aroot=load(load(0x260024,4)+16,4)&0xbfffff;a1=buf(data(load(aroot+4,4)&0xffffff,40));shapeb=buf(data(shapes,688));tileptr=load(load(shapes+108+56,4),4);tileb=buf(data(tileptr,352));assets=Assets((Animation*2)(Animation(a0,40,5),Animation(a1,40,5)),shapeb,tileb,688,352)
class Native(Model):
 def __init__(self):
  super().__init__();self.e=Engine();self.d=Display();self.c=Cursor();self.global_=U8(self.r(0x400d));self.ts=buf(self.tiles);self.ss=buf(self.sprites);self.ps=buf(self.pal)
  e,d,c=self.e,self.d,self.c
  for i,a in enumerate(zero_addresses):d.zero_flags[i]=self.r(a)
  d.capacity=self.r(0x66bc);d.limit=self.r(0x61aa)
  for i in range(32):d.object_ids[i]=self.r(0x453b+64*i);d.object_tags[i]=self.r(0x453d+64*i,2);d.list[i]=self.r(0x4d3b+2*i,2)
  d.zero_words[0]=self.r(0x4d7b,2);d.zero_words[1]=self.r(0x6180,2);d.reset_words[0]=self.r(0x5bcb,4);d.reset_words[1]=self.r(0x61ac,4)
  for name,a,n in curmap:setattr(c,name,self.r(a,n))
  e.frame=self.r(0x4547);e.delay=self.r(0x4548);e.list_dirty_high=self.r(0x4d7f)
  for i in range(4):e.bounds[i]=self.r(0x454f+2*i,2)
  for bank,base in ((0,0x61bb),(1,0x643b)):
   for plane in range(2):
    arr=d.pools[plane] if bank==0 else e.bank1[plane]
    for i in range(256):arr[i]=self.r(base+plane*0x140+i)
    for i in range(64):e.palettes[bank][plane][i]=self.r(base+plane*0x140+256+i)
  for i in range(d.zero_flags[10]):
   n=self.r(0x6000+6*i);p=self.r(0x6002+6*i,4);e.jobs[i]=Job((p&0xffffff)-shapes-2*n,self.r(0x6001+6*i)|(p>>24)*256,n)
 def serial(self,newjob=None,head=False):
  e,d,c=self.e,self.d,self.c
  for i,a in enumerate(zero_addresses):self.w(a,d.zero_flags[i])
  self.w(0x66bc,d.capacity);self.w(0x61aa,d.limit)
  for i in range(32):self.w(0x453b+64*i,d.object_ids[i]);self.w(0x453d+64*i,d.object_tags[i],2);self.w(0x4d3b+2*i,d.list[i],2)
  self.w(0x4d7b,d.zero_words[0],2);self.w(0x6180,d.zero_words[1],2);self.w(0x5bcb,d.reset_words[0],4);self.w(0x61ac,d.reset_words[1],4)
  for name,a,n in curmap:self.w(a,getattr(c,name),n)
  self.w(0x4547,e.frame);self.w(0x4548,e.delay);self.w(0x4d7f,e.list_dirty_high)
  for i in range(4):self.w(0x454f+2*i,e.bounds[i],2)
  for bank,base in ((0,0x61bb),(1,0x643b)):
   for plane in range(2):
    arr=d.pools[plane] if bank==0 else e.bank1[plane]
    for i in range(256):self.w(base+plane*0x140+i,arr[i])
    for i in range(64):self.w(base+plane*0x140+256+i,e.palettes[bank][plane][i])
  self.w(0x400d,self.global_.value)
  if newjob is not None:
   j=e.jobs[newjob];self.w(0x6000+6*newjob,j.count);self.w(0x6001+6*newjob,j.tile);self.w(0x6002+6*newjob,(shapes+j.shape_offset+2*j.count)|((j.tile>>8)<<24),4)
  if head:self.w(0x619e,0x61aa,4);self.w(0x61a2,(0x61bb if e.uploaded_bank==0 else 0x643b)+0x140,4)
  self.tiles=bytearray(self.ts);self.sprites=bytes(self.ss);self.pal=bytes(self.ps)
 def tail(self):
  old=self.d.zero_flags[10];scroll=(U16*2)(self.r(0x4d80,2),self.r(0x4d82,2));r=lib.ntm_course_cursor_tail(C.byref(self.e),C.byref(self.c),C.byref(self.d),self.r(0x453f,2),scroll,C.byref(self.global_),C.byref(assets));assert r==0,('native tail error',r);self.serial(old if self.d.zero_flags[10]>old else None)
 def head(self):
  r=lib.ntm_course_cursor_head(C.byref(self.e),C.byref(self.d),self.ts,self.ss,self.ps,C.byref(assets));assert r==0,('native head error',r);self.serial(head=True)
 def display_reset(self):
  assert lib.ntm_display_reset(C.byref(self.d))==0
  self.c.tag=self.d.object_tags[0];self.c.id=self.d.object_ids[0];self.serial()
