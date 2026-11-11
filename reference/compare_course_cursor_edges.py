#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private original regression for inclusive legacy sprite clipping edges."""
import cursor_native_case_common as native
from cursor_native_case_common import *

def resolve(index,velocity):
 ap=load((load(load(0x260024,4)+index,4)&0xbfffff)+velocity,4)&0xffffff
 sp=load(load(0x260028,4)+index,4);frames=0;ae=0;se=0;te=0;tp=None
 while True:
  cp=load(ap+frames*4+2,2);shape=load(ap+cp,2)&32767;n=1
  while not load(sp+shape+2*(n-1),1)&128:n+=1;assert n<=64
  ae=max(ae,cp+4);se=max(se,shape+4*n+4);p=load(load(sp+shape+2*n,4),4);assert tp in (None,p);tp=p
  te=max(te,16*(1+max(load(sp+shape+2*n+4+2*j,2) for j in range(n))));frames+=1
  if load(ap+4*frames-3,1)&128:break
  assert frames<=64
 ab=buf(data(ap,max(ae,4*frames)));sb=buf(data(sp,se));tb=buf(data(tp,te))
 selected=Animation(ab,len(ab),frames);animations=(Animation*2)()
 animations[velocity//4 if index==16 else 0]=selected
 return Assets(animations,sb,tb,len(sb),len(tb)),sp,(ab,sb,tb),ap

if __name__=='__main__':
 cases=0
 for tag,index,velocity in ((46,16,0),(46,16,4),(38,20,0),(38,24,0),(38,28,0),(33,32,0)):
  native.assets,native.shapes,retained,ap=resolve(index,velocity)
  cp=load(ap+2,2);shape=load(ap+cp,2)&32767
  dx=signed(load(ap+cp+2,1));dy=signed(load(ap+cp+3,1));xy=load(native.shapes+shape+1,1)
  for flip in (0,4,8,12):
   for sx in (-8,-7,-1,0,158,159,160):
    for sy in (-8,-7,-1,0,150,151,152):
     init();store(0x453d,tag,2);store(0x4543,index,2);store(0x4545,velocity,2);store(0x454b,192|flip,1)
     x=sx-(-dx if flip&8 else dx)-(-8*(xy&15)-8 if flip&8 else 8*(xy&15))
     y=sy-(-dy if flip&4 else dy)-(-8*(xy>>4)-8 if flip&4 else 8*(xy>>4))
     store(0x453f,x&65535,2);store(0x4541,y&65535,2);put(0xa000,R.randbytes(8192));put(0x61bb,R.randbytes(0x500));m=Native()
     m.tail();execute(0x201192);execute(0x200c7b);m.compare((tag,index,velocity,flip,sx,sy,'tail'))
     m.head();execute(0x20067c);execute(0x200aa6);m.compare((tag,index,velocity,flip,sx,sy,'head'));cases+=1
 print(json.dumps({'cases':cases,'resource_indices':[16,20,24,28,32],'edges':'first sprite x=-8,-7,-1,0,158,159,160;y=-8,-7,-1,0,150,151,152;all4 flips;both selector animations','comparisons':'all modeled nonstack RAM,8192 tile bytes,256 sprite bytes,64 palette indices','c_source_sha256':hashlib.sha256((ROOT/'native-game/course_cursor.c').read_bytes()).hexdigest()}))
