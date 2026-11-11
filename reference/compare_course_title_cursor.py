#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private original oracle for course-title object stop, geometry and tile work.

Only source addresses/control metadata are recorded here. Asset bytes and
results remain in ignored build directories. Requires the screen-probe core.
"""
import cursor_native_case_common as native
from cursor_native_case_common import *

def title_setup(course):
 init();store(0x4204,course,1);reg(6,0x7000);execute(0x20771d)
 for _ in range(24):
  pc=load(0x7000,4)
  if pc==0x2078da:return
  execute(pc)
 raise AssertionError('title initialization did not finish')

def title_assets(course):
 title_setup(course)
 index=load(0x4543,2);ap=load(load(load(0x260024,4)+index,4)&0xbfffff,4)&0xffffff
 sp=load(load(0x260028,4)+index,4);comp=load(ap+2,2);shape=load(ap+comp,2)&32767
 count=1
 while not(load(sp+shape+2*(count-1),1)&128):count+=1
 assert count<=64
 size=shape+4*count+4;source=load(load(sp+shape+2*count,4),4)
 tile_size=16*(1+max(load(sp+shape+2*count+4+2*i,2) for i in range(count)))
 ab=buf(data(ap,comp+4));sb=buf(data(sp,size));tb=buf(data(source,tile_size))
 return Assets((Animation*2)(Animation(ab,len(ab),1),Animation()),sb,tb,len(sb),len(tb)),sp,(ab,sb,tb),count

def fill_jobs(m,used):
 # Exercise an occupied queue containing valid jobs from this resource group.
 for j in range(used):
  count=m.d.zero_flags[13] or 1
  # Shape0 is the title's sole component; complete tile range remains bounded.
  count=title_count
  m.e.jobs[j]=Job(0,0,count)
  p=native.shapes+2*count
  m.w(0x6000+6*j,count);m.w(0x6001+6*j,0);m.w(0x6002+6*j,p,4)
  store(0x6000+6*j,count,1);store(0x6001+6*j,0,1);store(0x6002+6*j,p,4)
 m.d.zero_flags[10]=used;m.d.zero_words[1]=6*used
 m.w(0x6182,used);m.w(0x6180,6*used,2);store(0x6182,used,1);store(0x6180,6*used,2)

if __name__=='__main__':
 count=cases=0
 for course in range(3):
  native.assets,native.shapes,retained,title_count=title_assets(course)
  for variant in range(256):
   title_setup(course)
   put(0xa000,R.randbytes(8192));put(0x61bb,R.randbytes(0x500))
   store(0x453f,R.choice((40,0,65535,65530,160,166,256,32768)),2)
   store(0x4541,R.choice((48,0,65535,65530,152,158,256)),2)
   store(0x4549,R.randrange(16)|(128 if variant&4 else 0),1)
   store(0x454a,variant%4,1);store(0x454b,variant,1)
   store(0x4547,0,1);store(0x4548,variant,1)
   store(0x66bc,1+variant%64,1);store(0x66bb,0,1)
   m=Native()
   fill_jobs(m,(0,63,64)[variant%3])
   for tick in range(8):
    if tick==2:
     store(0x400d,load(0x400d,1)|32,1);m.global_.value|=32
    if tick==3 and not(variant&16):
     # Restart while stopped/frozen, then release freeze one update later.
     store(0x454b,load(0x454b,1)|128,1);m.c.flags|=128
    if tick==4 and not(variant&16):
     store(0x454b,load(0x454b,1)&~64,1);m.c.flags&=~64
    if tick==6:
     if variant&1:execute(0x2006cf);m.display_reset();m.compare((course,variant,tick,'reset'))
     else:store(0x454b,load(0x454b,1)|16,1);m.c.flags|=16
    m.tail();execute(0x201192);execute(0x200c7b);m.compare((course,variant,tick,'tail'))
    m.head();execute(0x20067c);execute(0x200aa6);m.compare((course,variant,tick,'head'));count+=1
   cases+=1
 print(json.dumps({'courses':3,'cases':cases,'tail_head_rounds':count,'comparisons':'all modeled nonstack RAM,8192 tile bytes,256 sprite bytes,64 palette indices','edges':'all initial flag/delay bytes; 0/63/64 queued jobs; restart, freeze release, forced refresh, clipping, flip, hide, remove and display reset'}))
