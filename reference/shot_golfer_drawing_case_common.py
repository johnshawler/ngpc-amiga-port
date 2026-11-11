#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private exact resource spans for the actual golfer initial drawing callback."""
from shot_golfer_animation_case_common import *
from shot_golfer_features_case_common import GolferFeatures,GolferFeatureResources,feature_resources
class GolferDrawing(C.Structure):_fields_=[('features',GolferFeatures),('center_x',U16),('half_width',U16),('phase',U8)]
class GolferDrawingResources(C.Structure):
 _fields_=[('features',GolferFeatureResources),('animation',P),('rim',Asset),('green',Asset*4)]+[(n,P) for n in ('eligible','depth_records','clip_records','metadata')]+[(n,C.c_size_t) for n in ('eligible_size','depth_records_size','clip_records_size','metadata_size','clip_origin')]
golfer_drawing_resources=GolferDrawingResources()
golfer_drawing_resources.animation=addr(golfer_animation_resources)
golfer_drawing_resources.rim=tee_asset(0x6c,0x88)
for i,selector in enumerate((0xa8,0xa4,0xa0,0x9c)):golfer_drawing_resources.green[i]=tee_asset(0x6c,selector)
for n,a,size in [('eligible',0x21ba55,16),('depth_records',0x21b975,20),('clip_records',0x21b99d,116),('metadata',0x219854,5)]:
 setattr(golfer_drawing_resources,n,scene_span(a,size));setattr(golfer_drawing_resources,n+'_size',size)
golfer_drawing_resources.clip_origin=56
