#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Real tag51 sprite resources against unchanged original object engine."""
import os,json
os.environ['NTM_TEE_OBJECT_LIBRARY']=os.environ['NTM_SHOT_GOLFER_LIBRARY']
import compare_tee_objects as O
from shot_golfer_object_assets import resolve
from compare_tee_objects import C,Animation,Native,store,load,reg,execute,data,buf
class Assets(C.Structure):
 _fields_=[('animations',Animation*40),('shapes',C.POINTER(O.U8)),('tiles',C.POINTER(O.U8)),('shapes_size',C.c_size_t),('tiles_size',C.c_size_t)]
r=resolve(load,data)
selections=sorted(r['animations'])
O.shape_root=r['shape_address'];O.tile_root=r['tile_address'];O.shape_end=len(r['shapes']);O.tile_end=len(r['tiles'])
O.shape_buffer=buf(r['shapes']);O.tile_buffer=buf(r['tiles']);buffers=[];animations=(Animation*40)()
for velocity,(raw,frames) in r['animations'].items():
 b=buf(raw);buffers.append(b);animations[velocity//4]=Animation(b,len(b),frames)
O.assets=Assets(animations,O.shape_buffer,O.tile_buffer,O.shape_end,O.tile_end)
O.asset_refs=(C.POINTER(Assets)*32)(*[C.pointer(O.assets) for _ in range(32)])
O.lib.ntm_tee_objects_tail.argtypes[-1]=C.POINTER(C.POINTER(Assets))
# Assigning a list entry does not update ctypes' cached conversion table.
O.lib.ntm_tee_objects_tail.argtypes=list(O.lib.ntm_tee_objects_tail.argtypes)
rounds=0
for velocity in selections:
 for flags in (128,192,196,200,204):
  for x,y in ((40,88),(65535,65535),(159,151)):
   O.initialize(0);p=0x467b
   for off,value,size in ((1,0,1),(2,51,2),(4,x,2),(6,y,2),(8,60,2),(10,velocity,2),(14,0,1),(15,2,1),(16,flags,1),(18,206,2),(30,0,1)):store(p+off,value,size)
   m=Native();assert m.register(5)==0;reg(4,p);execute(0x2007ca);m.compare(('golfer-pin-register',velocity))
   for tick in range(3):O.compare_round(m,(velocity,flags,x,y,tick));rounds+=1
print(json.dumps({'rounds':rounds,'scope':'all9 real tag51 initializer/distance selections; original registration/tail/head, frozen/flips/clipping; full RAM/video/resource identity'}))
