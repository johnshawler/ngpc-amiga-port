#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original first-shot tag5 object traversal, deferred graphics and sprite order."""
import compare_tee_objects as O
from aim_object_assets import resolve
from compare_tee_objects import C,U8,Animation,Assets,Native,setup,store,load,put,reg,execute,data,R,buf
cases=rounds=0
for course in range(3):
 for hole in range(18):
  r=resolve(load,data,course,hole);O.shape_root=r['shape_address'];O.tile_root=r['tile_address'];O.shape_end=len(r['shapes']);O.tile_end=len(r['tiles'])
  O.shape_buffer=buf(r['shapes']);O.tile_buffer=buf(r['tiles']);buffers=[];animations=(Animation*40)()
  for velocity,(b,n) in r['animations'].items():
      a=buf(b);buffers.append(a);animations[velocity//4]=Animation(a,len(a),n)
  O.assets=Assets(animations,O.shape_buffer,O.tile_buffer,O.shape_end,O.tile_end)
  O.asset_refs=(C.POINTER(Assets)*32)(*[C.pointer(O.assets) for _ in range(32)])
  for velocity in (4*hole,):
   for flags in (128,132,136,140,192):
    for x,y in ((64,80),(64,65536-80),(0,0),(159,151)):
     O.initialize(0);p=0x463b
     for off,value,n in ((1,255,1),(2,5,2),(4,x,2),(6,y,2),(8,68+4*course,2),(10,velocity,2),(14,0,1),(15,3,1),(16,flags,1),(18,156,2),(30,0,1)):store(p+off,value,n)
     m=Native();assert m.register(4)==0;reg(4,p);execute(0x2007ca);m.compare(('register',velocity,flags,x,y))
     # Registration always sets restart, preserving caller freeze and flip bits.
     for tick in range(20):
      if tick==8:store(p+16,128|(flags&12),1);m.c[4].flags=128|(flags&12)
      if tick==16:store(p+16,m.c[4].flags|16,1);m.c[4].flags|=16
      O.compare_round(m,(velocity,flags,x,y,tick),head=tick%3!=1);rounds+=1
     cases+=1
print(O.json.dumps({'cases':cases,'tail_head_rounds':rounds,'indices':[68,72,76],'tag':5,'holes':18,'scope':'full nonstack RAM,32 object records,list,bounds,queues,both sprite banks and palettes,8192 tiles,256 hardware sprites,64 sprite indices'}))
