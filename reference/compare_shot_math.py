#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private original arithmetic oracle; no extracted tables or captures."""
from screen_probe import *
import os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_LIBRARY',str(ROOT/'build/shot-check/native.so')))
for name,n in [('divide',2),('multiply',2),('distance',4)]:
 f=getattr(lib,'ntm_shot_'+name);f.argtypes=[C.c_uint32]*n;f.restype=C.c_uint32
R=random.Random(200024)
def run(pc,values):
 setup()
 for i,v in enumerate(values):reg(i,v)
 reg(8,pc)
 for _ in range(4000):
  if get(8)==0xef00:return get(0)
  step(0)
 raise AssertionError(('budget',hex(get(8)),values))
edge=[0,1,2,65535,65536,65537,0x7fffffff,0x80000000,0x80000001,0xffff0000,0xfffffffe,0xffffffff]
counts={}
for name,pc in [('divide',0x20d282),('multiply',0x20d179)]:
 cases=[(a,b) for a in edge for b in edge]+[(R.getrandbits(32),R.getrandbits(32)) for _ in range(2000)]
 for a,b in cases:
  expected=run(pc,(a,b))
  if name=='multiply':expected=((get(1)&65535)<<16)|(expected>>16)
  actual=getattr(lib,'ntm_shot_'+name)(a,b)
  assert actual==expected,(name,hex(a),hex(b),hex(actual),hex(expected))
 counts[name]=len(cases)
cases=[(a,b,0,0) for a in edge for b in edge]+[(R.getrandbits(32),R.getrandbits(32),R.getrandbits(32),R.getrandbits(32)) for _ in range(3000)]
for values in cases:
 actual=lib.ntm_shot_distance(*values);expected=run(0x20d024,values)
 assert actual==expected,('distance',[hex(v) for v in values],hex(actual),hex(expected))
counts['distance']=len(cases)
print(json.dumps(counts))
