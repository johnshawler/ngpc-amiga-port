#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private current-hole feature-list resolution, with actual terminator bounds."""
from shot_aim_case_common import *
class GolferFeatures(C.Structure):
 _fields_=[(n,U16) for n in ('origin_x','origin_y','extent')]+[(n,U32) for n in ('lateral_bias','depth_bias','lateral_scratch','depth_scratch')]+[('nearby',U8*81),('view',U8*81)]
class GolferFeatureResources(C.Structure):
 _fields_=[('features',P),('features_size',C.c_size_t),('aim',P)]
feature_resources=[]
for g in geometries:
 source=g.path_identity;size=0
 while load(source+size,1)!=255:
  size+=5;assert size<4096
 size+=1;feature_resources.append((GolferFeatureResources(span(source,size),size,C.addressof(aim_resources)),source))
