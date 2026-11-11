#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Test-only private resource wiring for real209751 continuation."""
from shot_aim_setup_case_common import *
import shot_aim_case_common as AM
from aim_world_case_common import WorldResources,WorldState,resolve_world
from shot_hud_case_common import hud_resources
from aim_object_assets import resolve
import compare_tee_objects as O
class AimApplicationResources(C.Structure):
 _fields_=[('aim',P),('worlds',P),('world_count',C.c_size_t),('hud',P),('setup',P),('objects',P*54),('palette',Job)]
world_keep=[];worlds=(WorldResources*54)()
for course in range(3):
 for hole in range(18):
  offset=load(load(0x36000c,4)+4*course,2)+18*hole
  worlds[18*course+hole]=resolve_world(rom,offset,world_keep)
object_keep=[];object_resources=[];object_shapes=[]
for entry in range(54):
 course,hole=divmod(entry,18)
 res=resolve(load,data,course,hole);animations=(O.Animation*40)()
 for velocity,(b,n) in res['animations'].items():
  ab=blob(b);object_keep.append(ab);animations[velocity//4]=O.Animation(ab,len(ab),n)
 shapes=blob(res['shapes']);tiles=blob(res['tiles']);object_keep.extend((shapes,tiles,animations))
 a=O.Assets(animations,shapes,tiles,len(shapes),len(tiles));object_resources.append(a);object_shapes.append(res['shape_address'])
packed=load(0x2198f3+12,4);assert packed&255==4;desc=packed>>8
count=load(desc,1);source=load(desc+2,4);offsets=blob(data(source,2*count));colors=blob(data(load(0x26001c,4),max(load(source+2*i,2) for i in range(count))+8))
palette=Job(1,0,8*load(desc+1,1),count,0,0,offsets,colors,len(offsets),len(colors))
resources=AimApplicationResources(addr(AM.aim_resources),addr(worlds),54,addr(hud_resources),addr(aim_setup_resources),(P*54)(*[addr(a) for a in object_resources]),palette)
