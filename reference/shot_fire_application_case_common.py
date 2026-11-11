#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Borrow actual fire resources, every hole map/list and real sprite families."""
from shot_golfer_drawing_case_common import *
from shot_spin_case_common import Spin,SpinResources,SpinDrawResources,spin_resources,spin_draw_resources
from shot_trajectory_case_common import Trajectory,trajectory_resources
import shot_trajectory_object_assets as TR
import shot_golfer_object_assets as PI
import shot_golfer_drawing_object_assets as GR
import compare_tee_objects as O
class FireApplicationResources(C.Structure):
 _fields_=[(n,P) for n in ('parent','spin','spin_draw','trajectory','golfer','drawing','terrain')]+[('hole_count',C.c_size_t)]+[(n,P) for n in ('trajectory_objects','pin_objects','green_objects')]
fire_application_keep=[];object_shape_addresses={}
def object_assets(module):
 resolved=module.resolve(load,data);animations=(O.Animation*40)()
 for velocity,(b,n) in resolved['animations'].items():
  ab=blob(b);fire_application_keep.append(ab)
  index=resolved.get('animation_indices',{}).get(velocity,velocity//4)
  animations[index]=O.Animation(ab,len(ab),n)
 shapes=blob(resolved['shapes']);tiles=blob(resolved['tiles']);fire_application_keep.extend((animations,shapes,tiles))
 object_shape_addresses[C.cast(shapes,P).value]=resolved['shape_address']
 return O.Assets(animations,shapes,tiles,len(shapes),len(tiles))
trajectory_objects=object_assets(TR);pin_objects=object_assets(PI);green_objects=object_assets(GR)
terrains=(TerrainResources*54)();drawings=(GolferDrawingResources*54)();terrain_sources=[]
masks=scene_span(0x223104,8);terrain_shapes=scene_span(0x22310c,2048)
for index,g in enumerate(AM.geometries):
 large,ls=resolve_terrain(g.map_index,fire_application_keep);close,cs=resolve_terrain(g.aux_index,fire_application_keep)
 terrains[index]=TerrainResources(large,close,masks,terrain_shapes,8,2048);terrain_sources.append((ls,cs))
 drawings[index]=golfer_drawing_resources;drawings[index].features=feature_resources[index][0]
resources=FireApplicationResources(addr(fire_resources),addr(spin_resources),addr(spin_draw_resources),addr(trajectory_resources),addr(golfer_resources),addr(drawings),addr(terrains),54,addr(trajectory_objects),addr(pin_objects),addr(green_objects))
