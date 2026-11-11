#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Bounded private golfer scenery and terrain-map resolution; no ROM bytes."""
from shot_fire_case_common import *
class GolferAsset(C.Structure):_fields_=[('selector',U16),('asset',Asset)]
class GolferResources(C.Structure):
 _fields_=[('palettes',(Job*2)*3)]+[(n,P) for n in ('course_selectors','ground_records','foreground_records','frames','overrides','green_records','pin_records','metadata')]+[(n,C.c_size_t) for n in ('course_selectors_size','ground_records_size','foreground_records_size','frames_size','overrides_size','green_records_size','pin_records_size','metadata_size')]+[('assets',P),('asset_count',C.c_size_t),('aim',P)]
class Golfer(C.Structure):
 _fields_=[('drawing',P)]+[(n,U32) for n in ('delta_x','delta_y','sample_x','sample_y','direction_x','direction_y','depth_scratch')]+[('attempts',U8),('surface',U8),('task_bytes',U8*8),('phase',U8)]
class TerrainMap(C.Structure):_fields_=[('data',P),('size',C.c_size_t)]
class TerrainResources(C.Structure):_fields_=[('large',TerrainMap),('close',TerrainMap),('masks',P),('shapes',P),('masks_size',C.c_size_t),('shapes_size',C.c_size_t)]
class GolferContext(C.Structure):_fields_=[('scene',P),('terrain',P),('terrain_detail',P),('player_status',U8)]
def terminated(address,stride):
 size=0
 while True:
  size+=stride
  if load(address+size-stride,2)==65535:return size
  assert size<512
specs=[(0x20a62f,3),(0x21ba11,60),(0x20a876,60),(0x21ba65,64),(0x21ba4d,8),(0x21b93d,terminated(0x21b93d,6)),(0x21b955,terminated(0x21b955,4)),(0x21984f,5)]
golfer_resources=GolferResources();golfer_palette_descriptors=[]
for name,(a,n) in zip(('course_selectors','ground_records','foreground_records','frames','overrides','green_records','pin_records','metadata'),specs):setattr(golfer_resources,name,scene_span(a,n));setattr(golfer_resources,name+'_size',n)
for course in range(3):
 pair=[]
 for k in range(2):
  command=4*(course+6+3*k);packed=load(0x2198f3+command,4);descriptor=packed>>8;pair.append((command,descriptor))
  if packed&255==4:
   count=load(descriptor,1);op=load(descriptor+2,4);ob=blob(data(op,2*count));cs=max(load(op+2*i,2) for i in range(count))+8;cb=blob(data(load(0x26001c,4),cs));fire_keep.extend((ob,cb))
   golfer_resources.palettes[course][k]=Job(1,0,load(descriptor+1,1)*8,count,0,0,ob,cb,len(ob),len(cb))
  else:
   assert packed&255==32
   golfer_resources.palettes[course][k]=Job(2,load(descriptor+5,1),480+load(descriptor,1),0,load(descriptor+1,2),load(descriptor+3,2),None,None,0,0)
 golfer_palette_descriptors.append(pair)
selectors={0,*data(0x20a62f,3)}
for a,n,stride,offset in [(0x21ba11,60,4,0),(0x20a876,60,4,0),(0x21ba4d,8,2,0),(0x21b93d,specs[5][1],6,2)]:selectors.update(load(a+i+offset,2) for i in range(0,n,stride))
golfer_prior_assets=list(tee_assets)
golfer_assets=(GolferAsset*len(selectors))()
for target,selector in zip(golfer_assets,sorted(selectors)):target.selector=selector;target.asset=tee_asset(0x6c,selector)
golfer_resources.assets=addr(golfer_assets);golfer_resources.asset_count=len(golfer_assets);golfer_resources.aim=addr(AM.aim_resources)
def resolve_terrain(record,keep):
 """Read only the referenced map rows/RLE runs, no padded ROM windows."""
 root=load(0x360000,4)+record;blocks=load(root,4);delta=load(root+4,2);delta=delta if delta<32768 else delta-65536
 width=load(blocks+delta,1)*4;height=load(blocks+delta+1,1)*4;source=load(root+14,4)
 end=2*height
 for y in range(height):
  at=load(source+2*y,2);covered=0
  while covered<width:
   count=load(source+at,1);assert count>0
   covered+=count;at+=3;assert at<65536
  end=max(end,at)
 b=blob(data(source,end));keep.append(b);return TerrainMap(addr(b),len(b)),source
