#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# User-supplied original stays private. Run tools/check-course-screen.sh.
import sys,random,json,hashlib
from screen_probe import *
R=random.Random(209603)
U8=C.c_uint8;U16=C.c_uint16
class Asset(C.Structure):_fields_=[('data',C.POINTER(U8)),('size',C.c_size_t),('tiles',C.POINTER(U8)),('tiles_size',C.c_size_t)]
class Stamp(C.Structure):_fields_=[('asset',Asset),('offset',U16),('tile',U16)]
native=C.CDLL(str(ROOT/'build/course-screen-check/native_screen.so'))
native.ntm_course_screen_stamp.argtypes=[C.POINTER(Stamp),C.POINTER(U8),C.POINTER(U8)];native.ntm_course_screen_stamp.restype=C.c_int

def run(pc):
 reg(7,0x6be0);store(0x6be0,0xef00,4)
 reg(8,pc)
 for i in range(50000):
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
  for j in range(100):
   v=load(q,2);q+=2
   if v==65535:break
  else:raise RuntimeError('row too wide')
 return q-src

def compare(src,base,base_size,offset,tile,reason):
 tiles=bytes(R.randrange(256) for _ in range(8192));maps=bytes(R.randrange(256) for _ in range(4096))
 setmem(0xa000,tiles);setmem(0x9000,maps)
 a=blob(data(src,maplen(src)));b=blob(data(base,base_size));t=blob(tiles);m=blob(maps)
 stamp=Stamp(Asset(a,len(a),b,len(b)),offset,tile)
 reg(0,tile);reg(4,offset+0x9000);reg(5,src);run(0x200581)
 result=native.ntm_course_screen_stamp(C.byref(stamp),t,m)
 assert result==0,(reason,'native invalid')
 assert bytes(t)==data(0xa000,8192),(reason,'tiles')
 assert bytes(m)==data(0x9000,4096),(reason,'maps')
checks={}
# Authenticated private assets, copied only into process memory.
n=0
for s in range(3):
 for trial in range(5):
  setup();table=load(load(0x260020,4)+24,4)&0xffffff;src=load(table+4*(s+2),4);base=load(src+2,4)
  maxoff=max(load(src+6+2*i,2) for i in range(load(src,2)))
  compare(src,base,maxoff+16,0x302+12*s,301,('original asset',s,trial));n+=1
checks['private_course_assets']=n
# Synthetic bounded nonempty normal 16-bit maps; variable row widths and palette.
n=0
for trial in range(256):
 setup();src=0x6000;base=0x6400;count=R.randrange(1,12);rows=R.randrange(1,10);tile=R.randrange(512-count)|R.randrange(32)*512
 offsets=[R.randrange(32)*16 for _ in range(count)];payload=bytes(R.randrange(256) for _ in range(512))
 store(src,count,2);store(src+2,base,4);setmem(base,payload)
 for i,off in enumerate(offsets):store(src+6+2*i,off,2)
 q=src+6+2*count;store(q,rows|128,1);q+=1
 for y in range(rows):
  row=[R.randrange(65535) for _ in range(R.randrange(1,14))]
  for v in row+[65535]:store(q,v,2);q+=2
 compare(src,base,len(payload),R.randrange(64)*2,tile,('synthetic asset',trial));n+=1
checks['synthetic_normal16_assets']=n
print(json.dumps({'status':'PASS','checks':checks,'native_source_sha256':hashlib.sha256((ROOT/'native-game/course_screen.c').read_bytes()).hexdigest()},sort_keys=True))
