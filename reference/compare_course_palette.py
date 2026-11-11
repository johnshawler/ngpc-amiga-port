#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Isolated original comparisons. User-supplied assets remain private.
from course_palette_case_common import *
count=0;full=0
for case in range(1024):
 setup();setram(0x4000,R.randbytes(16384));store(0x40fa,16+case%240,1)
 s=Palette();perm=list(range(8));R.shuffle(perm);n=case%9
 for i,index in enumerate(perm):
  kind=R.randrange(2);code=0x124 if kind==0 else 0x128;job=group if kind==0 else single
  s.slots[i]=(0x400 if kind==0 else 0x2000)|index*4 if i<n else 0xff00|index*4
  s.jobs[index]=job
  a=0x66cf+32*index
  store(a,4 if kind==0 else 32,2);store(a+2,code,2);store(a+8,job.offset,2)
  if kind==0:store(a+10,48,2);store(a+12,0x21a073,4)
  else:store(a+12,job.color,2);store(a+14,job.control_offset,2);store(a+26,job.pending_index,1)
  store(0x66bf+2*i,s.slots[i],2)
 s.status=65535;store(0x66bd,s.status,2)
 for i,v in enumerate(R.randbytes(512)):s.staging[i]=v;store(0x67f1+i,v,1)
 for i,v in enumerate(R.randbytes(512)):s.palette[i]=v;store(0x8200+i,v,1)
 store(0x8118,R.randrange(256),1);store(0x8012,R.randrange(256),1)
 s.background=load(0x8118,1);s.window_control=load(0x8012,1)
 for i,mask in enumerate((0xc7,0x87)):
  s.pending[i]=255 if case%3 else R.randrange(256)&mask;store(0x69f1+i,s.pending[i],1)
 available=8-min(8,n+2);s.copy_bytes=4*R.randrange(available+1);store(0x67ef,s.copy_bytes,2)
 for i in range(8):
  offset=2*R.randrange(256);words=R.randrange(1,(512-offset)//2+1)
  s.copies[i]=Copy(offset,words);store(0x67cf+4*i,offset,2);store(0x67d1+4*i,words,2)
 for code,job in ((0x124,group),(0x128,single)):
  reg(0,code);execute(0x2037a8);r=native.ntm_course_palette_enqueue(C.byref(s),C.byref(job));assert r==(get(9)&1);full+=r;compare(s)
 for repeat in range(2):
  expected=bytearray(ram[:16384]);r=native.ntm_course_palette_tail(C.byref(s));assert r==0;serialize(s,expected);execute(0x202c77);compare(s,expected)
  expected=bytearray(ram[:16384]);r=native.ntm_course_palette_head(C.byref(s));assert r==0;serialize(s,expected);execute(0x202bce);compare(s,expected)
 count+=1
print(json.dumps({'cases':count,'full_enqueues':full,'tail_head_rounds':count*2,'native_sha256':hashlib.sha256((ROOT/'native-game/course_palette.c').read_bytes()).hexdigest()}))
