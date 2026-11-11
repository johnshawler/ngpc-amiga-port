#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private original oracle; no cartridge bytes or derived assets embedded."""
from cursor_case_common import *
import ctypes as C, hashlib, os
U8=C.c_uint8;U16=C.c_uint16;U32=C.c_uint32
class Display(C.Structure):_fields_=[('zero_flags',U8*16),('capacity',U8),('limit',U8),('object_ids',U8*32),('pools',(U8*256)*2),('zero_words',U16*2),('object_tags',U16*32),('list',U16*32),('reset_words',U32*2)]
class Cursor(C.Structure):_fields_=[(n,U16) for n in ('tag','y','width','velocity','tile','link')]+[(n,U8) for n in ('id','second','byte14','byte15','flags','byte30')]
class Animation(C.Structure):_fields_=[('data',C.POINTER(U8)),('size',C.c_size_t),('frames',U8)]
class Assets(C.Structure):_fields_=[('animations',Animation*40),('shapes',C.POINTER(U8)),('tiles',C.POINTER(U8)),('shapes_size',C.c_size_t),('tiles_size',C.c_size_t)]
class Job(C.Structure):_fields_=[('shapes',C.POINTER(U8)),('tiles',C.POINTER(U8)),('shapes_size',C.c_size_t),('tiles_size',C.c_size_t),('shape_offset',U16),('tile',U16),('count',U8)]
class Engine(C.Structure):_fields_=[('frame',U8*32),('delay',U8*32),('list_dirty_high',U8),('uploaded_bank',U8),('bounds',(U16*4)*32),('bank1',(U8*256)*2),('palettes',((U8*64)*2)*2),('jobs',Job*64)]
lib=C.CDLL(os.environ.get('NTM_TEE_OBJECT_LIBRARY',str(ROOT/'build/tee-object-check/native.so')))
lib.ntm_tee_objects_tail.argtypes=[C.POINTER(Engine),C.POINTER(Cursor),C.POINTER(Display),C.POINTER(U16),C.POINTER(U16),C.POINTER(U8),C.POINTER(C.POINTER(Assets))]
lib.ntm_tee_objects_head.argtypes=[C.POINTER(Engine),C.POINTER(Display),C.POINTER(U8),C.POINTER(U8),C.POINTER(U8)]
lib.ntm_tee_object_register.argtypes=[C.POINTER(Display),C.POINTER(Cursor)]
lib.ntm_tee_object_remove.argtypes=[C.POINTER(Display),C.POINTER(Cursor)]
lib.ntm_tee_objects_reset.argtypes=[C.POINTER(Display),C.POINTER(Cursor)]
zero_addresses=[0x66bb,0x61ba,0x61b4,0x61b5,0x61b7,0x61b8,0x4d80,0x4d82,0x4d7d,0x4d7e,0x6182,0x619d,0x61a9,0x61a6,0x61a7,0x61a8]
curmap=[('tag',2,2),('y',6,2),('width',8,2),('velocity',10,2),('tile',18,2),('link',28,2),('id',0,1),('second',1,1),('byte14',14,1),('byte15',15,1),('flags',16,1),('byte30',30,1)]
def buf(b):return (U8*len(b)).from_buffer_copy(b)
# Resolve the resource selected by the tee descriptors, with exact reachable
# animation/component/shape bounds; original embedded pointers stay private.
resource_index=load(0x2197e1,2)
anim_root=load(load(0x260024,4)+resource_index,4)&0xbfffff
shape_root=load(load(0x260028,4)+resource_index,4)
animations=[];animation_buffers=[];shape_end=0;tile_end=0;tile_root=None
for v in range(0,32,4):
 a=load(anim_root+v,4)&0xffffff;frame=0;end=0
 while True:
  component=load(a+frame+2,2)
  for k in range(2):
   raw=load(a+component,2);shape=raw&32767;end=max(end,component+4)
   if load(shape_root+shape+1,1)==255:shape_end=max(shape_end,shape+2)
   else:
    n=0
    while True:
     n+=1
     if load(shape_root+shape+2*n-2,1)&128:break
     assert n<64
    shape_end=max(shape_end,shape+4*n+4)
    tr=load(load(shape_root+shape+2*n,4),4)
    assert tile_root in (None,tr);tile_root=tr
    tile_end=max(tile_end,16*(max(load(shape_root+shape+2*n+4+2*j,2) for j in range(n))+1))
   if raw&32768:break
   component+=4
  else:raise AssertionError('more than two components')
  frame+=4
  if load(a+frame-3,1)&128:break
  assert frame<256
 b=buf(data(a,max(end,frame)));animation_buffers.append(b);animations.append(Animation(b,len(b),frame//4))
shape_buffer=buf(data(shape_root,shape_end));tile_buffer=buf(data(tile_root,tile_end))
assets=Assets((Animation*40)(*animations),shape_buffer,tile_buffer,shape_end,tile_end)
asset_refs=(C.POINTER(Assets)*32)(*[C.pointer(assets) for _ in range(32)])
class Native(Model):
 def __init__(self):
  super().__init__();self.e=Engine();self.d=Display();self.c=(Cursor*32)();self.x=(U16*32)();self.global_=U8(self.r(0x400d));self.ts=buf(self.tiles);self.ss=buf(self.sprites);self.ps=buf(self.pal)
  e,d=self.e,self.d
  for i,a in enumerate(zero_addresses):d.zero_flags[i]=self.r(a)
  d.capacity=self.r(0x66bc);d.limit=self.r(0x61aa)
  for i in range(32):
   p=0x453b+64*i;d.object_ids[i]=self.r(p);d.object_tags[i]=self.r(p+2,2);d.list[i]=self.r(0x4d3b+2*i,2);self.x[i]=self.r(p+4,2)
   for name,off,n in curmap:setattr(self.c[i],name,self.r(p+off,n))
   e.frame[i]=self.r(p+12);e.delay[i]=self.r(p+13)
   for j in range(4):e.bounds[i][j]=self.r(p+20+2*j,2)
  d.zero_words[0]=self.r(0x4d7b,2);d.zero_words[1]=self.r(0x6180,2);d.reset_words[0]=self.r(0x5bcb,4);d.reset_words[1]=self.r(0x61ac,4);e.list_dirty_high=self.r(0x4d7f)
  for bank,base in ((0,0x61bb),(1,0x643b)):
   for plane in range(2):
    arr=d.pools[plane] if bank==0 else e.bank1[plane]
    for j in range(256):arr[j]=self.r(base+plane*0x140+j)
    for j in range(64):e.palettes[bank][plane][j]=self.r(base+plane*0x140+256+j)
  for i in range(d.zero_flags[10]):
   n=self.r(0x6000+6*i);p=self.r(0x6002+6*i,4)
   e.jobs[i]=Job(shape_buffer,tile_buffer,shape_end,tile_end,(p&0xffffff)-shape_root-2*n,self.r(0x6001+6*i)|(p>>24)*256,n)
 def serial(self,oldjobs=None,head=False):
  e,d=self.e,self.d
  for i,a in enumerate(zero_addresses):self.w(a,d.zero_flags[i])
  self.w(0x66bc,d.capacity);self.w(0x61aa,d.limit)
  for i in range(32):
   p=0x453b+64*i;self.w(p,d.object_ids[i]);self.w(p+2,d.object_tags[i],2);self.w(0x4d3b+2*i,d.list[i],2);self.w(p+4,self.x[i],2)
   for name,off,n in curmap:self.w(p+off,getattr(self.c[i],name),n)
   self.w(p+12,e.frame[i]);self.w(p+13,e.delay[i])
   for j in range(4):self.w(p+20+2*j,e.bounds[i][j],2)
  self.w(0x4d7f,e.list_dirty_high);self.w(0x4d7b,d.zero_words[0],2);self.w(0x6180,d.zero_words[1],2);self.w(0x5bcb,d.reset_words[0],4);self.w(0x61ac,d.reset_words[1],4)
  for bank,base in ((0,0x61bb),(1,0x643b)):
   for plane in range(2):
    arr=d.pools[plane] if bank==0 else e.bank1[plane]
    for j in range(256):self.w(base+plane*0x140+j,arr[j])
    for j in range(64):self.w(base+plane*0x140+256+j,e.palettes[bank][plane][j])
  self.w(0x400d,self.global_.value)
  if oldjobs is not None:
   for i in range(oldjobs,d.zero_flags[10]):
    j=e.jobs[i];self.w(0x6000+6*i,j.count);self.w(0x6001+6*i,j.tile);self.w(0x6002+6*i,(shape_root+j.shape_offset+2*j.count)|((j.tile>>8)<<24),4)
  if head:self.w(0x619e,0x61aa,4);self.w(0x61a2,(0x61bb if e.uploaded_bank==0 else 0x643b)+0x140,4)
  self.tiles=bytearray(self.ts);self.sprites=bytes(self.ss);self.pal=bytes(self.ps)
 def tail(self):
  old=self.d.zero_flags[10];scroll=(U16*2)(self.r(0x4d80,2),self.r(0x4d82,2))
  r=lib.ntm_tee_objects_tail(C.byref(self.e),self.c,C.byref(self.d),self.x,scroll,C.byref(self.global_),asset_refs)
  assert r==0,('native tail error',r);self.serial(old)
 def head(self):
  assert lib.ntm_tee_objects_head(C.byref(self.e),C.byref(self.d),self.ts,self.ss,self.ps)==0;self.serial(head=True)
 def remove(self,i):
  result=lib.ntm_tee_object_remove(C.byref(self.d),C.byref(self.c[i]));self.serial();return result
 def reset(self):
  assert lib.ntm_tee_objects_reset(C.byref(self.d),self.c)==0;self.serial()
 def register(self,i):
  result=lib.ntm_tee_object_register(C.byref(self.d),C.byref(self.c[i]));self.serial();return result

def initialize(count=4):
 setup();store(0x40fa,16,1);put(0x453b,R.randbytes(32*64));put(0x61bb,R.randbytes(0x500));put(0xa000,R.randbytes(8192));put(0x8800,R.randbytes(256));put(0x8c00,R.randbytes(64));execute(0x2006cf);store(0x400d,16,1)
 for i in range(count):
  p=0x453b+64*i;tag=(30,29,31,32)[i%4];vel={29:0,30:12,31:4,32:16}[tag]
  for off,v,n in [(1,0,1),(2,tag,2),(4,96 if i==0 else 56,2),(6,8 if i==0 else 24,2),(8,52,2),(10,vel,2),(14,0,1),(15,3,1),(16,128,1),(18,(400,432,448,464)[i%4],2),(30,0,1)]:store(p+off,v,n)

def compare_round(m,label,head=True):
 m.tail();execute(0x201192);execute(0x200c7b);m.compare((label,'tail'))
 if head:m.head();execute(0x20067c);execute(0x200aa6);m.compare((label,'head'))

if __name__=='__main__':
 rounds=0;cases=0
 for case in range(128):
  count=(3,4,8,32)[case%4];initialize(count)
  for i in range(count):
   p=0x453b+64*i;store(p+1,R.choice((0,1,2,3,255)),1);store(p+4,R.choice((0,8,56,104,65535,65530,160,166,256)),2);store(p+6,R.choice((0,24,152,158,65535,65530)),2)
   store(p+14,R.randrange(16)|(128 if case&4 else 0),1);store(p+15,(case*37+i*13)&255,1);store(p+16,128|((i%4)<<2)|(32 if case&8 and i%4!=3 else 0)|(64 if case&16 else 0),1)
   if i%4==2:store(p+10,4+(case%2)*4,2)
   if i%4==3:store(p+10,16+(case%4)*4,2)
  m=Native()
  for i in range(count):
   assert m.register(i)==0;reg(4,0x453b+64*i);execute(0x2007ca);m.compare((case,i,'register'));assert (get(0)&255)==0
  start=case%64;store(0x66bb,start,1);store(0x66bc,64-start,1);m.d.zero_flags[0]=start;m.d.capacity=64-start
  for tick in range(40):
   if tick in (5,16,24):
    i=case%count;p=0x453b+64*i;x=R.choice((8,56,104,65535,160));store(p+4,x,2);m.x[i]=x
    pri=R.randrange(256);store(p+1,pri,1);m.c[i].second=pri
   if tick==7:store(0x400d,load(0x400d,1)|32,1);m.global_.value|=32
   if tick==12:
    i=case%count;p=0x453b+64*i;store(p+16,128|(case&12),1);m.c[i].flags=128|(case&12)
   if tick==33:
    for i in range(count):
     if (i+case)%3==0:store(0x453b+64*i+16,load(0x453b+64*i+16,1)|16,1);m.c[i].flags|=16
   if tick==37:
    for i in range(count):store(0x453b+64*i+16,load(0x453b+64*i+16,1)|16,1);m.c[i].flags|=16
   compare_round(m,(case,tick),head=tick%3!=1);rounds+=1
  cases+=1
 # Every occupancy: FIFO component jobs drop individually, including a queue
 # with room for only the first component of the optional badge.
 for q in range(65):
  initialize();
  for i in range(4):reg(4,0x453b+64*i);execute(0x2007ca)
  for j in range(q):store(0x6000+6*j,1,1);store(0x6001+6*j,432&255,1);store(0x6002+6*j,(shape_root+2)|(1<<24),4)
  store(0x6182,q,1);store(0x6180,6*q,2);m=Native()
  for tick in range(3):compare_round(m,('queue',q,tick));rounds+=1
  cases+=1
 for removed in range(32):
  initialize(32)
  for i in range(32):
   store(0x453c+64*i,i%4,1);reg(4,0x453b+64*i);execute(0x2007ca)
  m=Native();assert m.register(0)==1;reg(4,0x453b);execute(0x2007ca);m.compare(('full',removed));assert (get(0)&255)==1
  compare_round(m,('before-remove',removed));rounds+=1
  assert m.remove(removed)==0;reg(4,0x453b+64*removed);execute(0x200824);m.compare(('explicit-remove',removed));assert (get(0)&255)==0
  assert m.remove(removed)==1;reg(4,0x453b+64*removed);execute(0x200824);m.compare(('remove-again',removed));assert (get(0)&255)==1
  compare_round(m,('after-remove',removed));rounds+=1
  m.reset();execute(0x2006cf);m.compare(('display-reset',removed))
  compare_round(m,('after-reset',removed));rounds+=1;cases+=1
 for bank in range(8):
  initialize(4)
  for i in range(4):reg(4,0x453b+64*i);execute(0x2007ca)
  store(0x61ba,bank,1);m=Native()
  for phase in range(2):
   m.head();execute(0x20067c);execute(0x200aa6);m.compare(('early-head',bank,phase))
  compare_round(m,('early-head-tail',bank));rounds+=1
  for phase in range(2):
   m.head();execute(0x20067c);execute(0x200aa6);m.compare(('repeat-head',bank,phase))
  cases+=1
 # Exactly the inclusive viewport boundary and its immediate neighbors,
 # accounting for tag29's signed (-1,-7) component offset.
 for sx in (-8,-7,-1,0,158,159,160):
  for sy in (-8,-7,-1,0,150,151,152):
   initialize(1);p=0x453b;store(p+2,29,2);store(p+10,0,2);store(p+4,(sx+1)&65535,2);store(p+6,(sy+7)&65535,2)
   reg(4,p);execute(0x2007ca);m=Native();compare_round(m,('clip',sx,sy));rounds+=1;cases+=1
 print(json.dumps({'cases':cases,'tail_head_rounds':rounds,'comparisons':'all nonstack RAM; all32 records/list; independent per-object animation/bounds; both sprite banks/palettes; FIFO tile jobs;8192 tiles;256 sprite bytes;64 palette indices','includes':'tags29..32,8 selections,empty shapes,2 components,32 object capacity,stable priority changes,swapped removal,queue occupancy0..64,double tails,clipping,flips,freeze/restart/hide','c_source_sha256':hashlib.sha256((ROOT/'native-game/tee_objects.c').read_bytes()).hexdigest()}))
