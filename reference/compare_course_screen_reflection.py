#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original200581/20054f, including real20ab15 scenery pair; no ROM edits."""
from screen_probe import *
import os,random
R=random.Random(0x200581);U8=C.c_uint8;U16=C.c_uint16
class Asset(C.Structure):_fields_=[('data',C.POINTER(U8)),('size',C.c_size_t),('tiles',C.POINTER(U8)),('tiles_size',C.c_size_t)]
class Stamp(C.Structure):_fields_=[('asset',Asset),('offset',U16),('tile',U16)]
class Fill(C.Structure):_fields_=[('offset',U16),('fill',U16),('width',U8),('height',U8)]
class Screen(C.Structure):_fields_=[('fill_status',U8*8),('fills',Fill*8),('stamp_bytes',U16),('stamps',Stamp*32),('cached_selection',U16),('cursor_x',U16),('displayed_selection',U8)]
lib=C.CDLL(os.environ['NTM_SCREEN_REFLECTION_LIBRARY'])
lib.ntm_course_screen_stamp.argtypes=[C.POINTER(Stamp),C.POINTER(U8),C.POINTER(U8)]
lib.ntm_course_screen_stamp_queue.argtypes=[C.POINTER(Screen),C.POINTER(U8),C.POINTER(U8)]
def put(a,v):
 for i,b in enumerate(v):store(a+i,b,1)
def read(a,n):return b''.join(load(i,1).to_bytes(1,'little') for i in range(a,a+n))
def video():return b''.join(load(i,4).to_bytes(4,'little') for i in range(0x8000,0xc000,4))
def blob(v):return (U8*len(v)).from_buffer_copy(v)
def run(pc):
 reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
 for _ in range(100000):
  if get(8)==0xef00:return low
  step(0);low=min(low,get(7))
 raise AssertionError(('budget',hex(get(8))))
def resource(src):
 n=load(src,2);base=load(src+2,4);p=src+6+2*n;rows=load(p,1);p+=1;size=2 if rows&128 else 1;sent=(1<<(8*size))-1
 for _ in range(rows&127):
  for _ in range(2049):
   v=load(p,size);p+=size
   if v==sent:break
  else:raise AssertionError('unbounded private row')
 a=blob(read(src,p-src));b=blob(read(base,max(load(src+6+2*i,2) for i in range(n))+16))
 return Asset(a,len(a),b,len(b)),(a,b)
def check(jobs,queued=False,pair=False):
 before=bytearray(ram[:16384]);v=video();t=blob(v[0x2000:]);m=blob(v[0x1000:0x2000]);keep=[];s=Screen();s.stamp_bytes=8*len(jobs)
 for i,(src,offset,tile) in enumerate(jobs):
  asset,refs=resource(src);keep.append(refs);s.stamps[i]=Stamp(asset,offset,tile)
 if queued:
  assert lib.ntm_course_screen_stamp_queue(C.byref(s),t,m)==0 and s.stamp_bytes==0
  before[0xd84:0xd86]=b'\0\0';low=run(0x20054f)
 else:
  for i in range(len(jobs)):assert lib.ntm_course_screen_stamp(C.byref(s.stamps[i]),t,m)==0
  if pair:low=run(0x20ab15)
  else:
   src,offset,tile=jobs[0];reg(0,tile);reg(4,0x9000+offset);reg(5,src);low=run(0x200581)
 actual=bytes(ram[:16384]);before[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==before,('RAM',[(hex(i+0x4000),a,b) for i,(a,b) in enumerate(zip(actual,before)) if a!=b][:12])
 assert video()==v[:0x1000]+bytes(m)+bytes(t),'complete hardware/video'
counts={'synthetic8':0,'synthetic16':0,'private_rim_pairs':0}
for wide in (False,True):
 for trial in range(256):
  setup();put(0x4000,R.randbytes(16384));put(0x9000,R.randbytes(12288))
  count=1+trial%11;tile=((trial*149)&0x3e00)|R.randrange(512-count+1)|((trial%4)<<14)
  src=0x6000;base=0x6800;payload=R.randbytes(512);put(base,payload)
  a=bytearray(count.to_bytes(2,'little')+base.to_bytes(4,'little'))
  for _ in range(count):a+=(R.randrange(32)*16).to_bytes(2,'little')
  a+=bytes([(128 if wide else 0)|5]);sent=65535 if wide else 255
  for y in range(5):
   for _ in range(1+R.randrange(19)):a+=R.randrange(sent).to_bytes(2 if wide else 1,'little')
   a+=sent.to_bytes(2 if wide else 1,'little')
  put(src,a);check([(src,128+2*(trial%8),tile)]);counts['synthetic16' if wide else 'synthetic8']+=1
# Genuine immediate20ab15 pair, plus the same assets in a FIFO queue.
for trial in range(32):
 for queued in (False,True):
  setup();put(0x4000,R.randbytes(16384));put(0x9000,R.randbytes(12288))
  table=load(load(0x260020,4)+0x6c,4)&0xffffff;src=load(table+0x88,4)
  jobs=[(src,0x1c4,0x1f5),(src,0x1d8,0x81f5)]
  if queued:
   store(0x4d84,16,2)
   for i,(source,offset,tile) in enumerate(jobs):
    store(0x4d86+8*i,source,4);store(0x4d8a+8*i,0x9000+offset,2);store(0x4d8c+8*i,tile,2)
  check(jobs,queued,not queued);counts['private_rim_pairs']+=1
print(json.dumps({'status':'PASS','checks':counts,'scope':'all nonstack RAM and complete8000:bfff hardware/video; original200581 and genuine20ab15 immediate pair plus20054f FIFO; no patched ROM/core'}))
