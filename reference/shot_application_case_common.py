#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private resolved inputs for the first native shot/banner path."""
from shot_banner_case_common import *
import compare_tee_objects as O
from shot_object_assets import resolve
class PreparationResources(C.Structure):
 _fields_=[('traits',P),('traits_size',C.c_size_t)]+[(n,P) for n in ('wind_low','wind_step','wind_limit')]+[(n,C.c_size_t) for n in ('wind_low_size','wind_step_size','wind_limit_size')]
class TerrainMap(C.Structure):_fields_=[('data',P),('size',C.c_size_t)]
class TerrainResources(C.Structure):_fields_=[('large',TerrainMap),('close',TerrainMap),('masks',P),('shapes',P),('masks_size',C.c_size_t),('shapes_size',C.c_size_t)]
class AnimatedJob(C.Structure):
 _fields_=[(n,U8) for n in ('kind','speed','pending_index')]+[(n,U16) for n in ('offset','count','source_color','target_color','control_offset')]+[(n,C.POINTER(U8)) for n in ('source','target','blend')]+[(n,C.c_size_t) for n in ('source_size','target_size','blend_size')]
class Animated(C.Structure):_fields_=[('jobs',AnimatedJob*8),('fraction',U8*8),('level',U8*8)]
class ShotResources(C.Structure):_fields_=[(n,P) for n in ('preparation','terrain','banner','objects','exit_palettes')]
shot_keep=[]
def sp(a,n):
 b=blob(data(a,n));shot_keep.append(b);return addr(b)
preparation=PreparationResources(sp(0x21ee81-128,256),256,sp(0x2050b9,16),sp(0x2050d9,16),sp(0x2050a7,18),16,16,18)
# Original first-shot point is exactly the prepared tee centre and returns4
# before map lookup. No invented binding for still-uninitialized large map5730.
terrain=TerrainResources(TerrainMap(None,0),TerrainMap(None,0),sp(0x223104,8),sp(0x22310c,2048),8,2048)
resolved=resolve(load,data);animations=(O.Animation*40)()
for velocity,(b,frames) in resolved['animations'].items():
 ab=blob(b);shot_keep.append(ab);animations[velocity//4]=O.Animation(ab,len(ab),frames)
shapes=blob(resolved['shapes']);tiles=blob(resolved['tiles']);shot_keep.extend((shapes,tiles))
shot_objects=O.Assets(animations,shapes,tiles,len(shapes),len(tiles))
blend=blob(data(0x20319d,512));shot_keep.append(blend)
def colors(table,count):return b''.join(data(load(0x26001c,4)+load(table+2*i,2),8) for i in range(count))
exit_palettes=(AnimatedJob*2)()
for i,command in enumerate((0x6c,0x70)):
 packed=load(0x2198f3+command,4);kind=packed&255;desc=packed>>8
 if kind==24:
  count=load(desc,1);source=blob(colors(load(desc+7,4),count));target=blob(colors(load(desc+3,4),count));shot_keep.extend((source,target))
  exit_palettes[i]=AnimatedJob(kind,load(desc+2,1),0,8*load(desc+1,1),count,0,0,0,source,target,blend,len(source),len(target),512)
 else:
  assert kind==40
  exit_palettes[i]=AnimatedJob(kind,load(desc+1,1),load(desc+8,1),480+load(desc,1),0,load(desc+4,2),load(desc+2,2),load(desc+6,2),None,None,blend,0,0,512)
shot_resources=ShotResources(addr(preparation),addr(terrain),addr(banner_resources),addr(shot_objects),addr(exit_palettes))
