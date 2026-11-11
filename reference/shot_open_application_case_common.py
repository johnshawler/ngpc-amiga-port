#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
from shot_club_case_common import *
from shot_markers_case_common import marker_resources,Markers,Hint,Camera
from aim_object_assets import resolve_scene
import compare_tee_objects as O
class OpenResources(C.Structure):_fields_=[(n,P) for n in ('scene','club','markers','objects')]
resolved_scene=resolve_scene(load,data);scene_object_animations=(O.Animation*40)();scene_animation_keep=[]
for velocity,(b,n) in resolved_scene['animations'].items():
 ab=blob(b);scene_animation_keep.append(ab);scene_object_animations[velocity//4]=O.Animation(ab,len(ab),n)
scene_shapes=blob(resolved_scene['shapes']);scene_tiles=blob(resolved_scene['tiles'])
scene_objects=O.Assets(scene_object_animations,scene_shapes,scene_tiles,len(scene_shapes),len(scene_tiles))
open_resources=OpenResources(addr(scene_object_resources),addr(club_resources),addr(marker_resources),addr(scene_objects))
