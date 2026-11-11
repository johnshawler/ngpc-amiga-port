#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private resolution for the original scenery animation sequences."""
from shot_golfer_case_common import *
class GolferAnimationResources(C.Structure):_fields_=[('frames',(Asset*4)*5)]
class GolferGreen(C.Structure):_fields_=[('center_x',U16),('half_width',U16),('status',U8)]
golfer_animation_resources=GolferAnimationResources()
golfer_animation_sources={}
for row,table in enumerate((0x20acbf,0x20acc7,0x20accf,0x20acd7,0x20acdf)):
 for frame in range(4):
  selector=load(table+2*frame,2);a=tee_asset(0x6c,selector)
  golfer_animation_resources.frames[row][frame]=a
  golfer_animation_sources[C.cast(a.data,P).value]=tee_assets[-1][0]
