#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private borrowed inputs for the209830 scene producer, no extracted bytes."""
from shot_aim_setup_case_common import *
import shot_aim_case_common as AM
class SceneObjectResources(C.Structure):
 _fields_=[('metadata',P),('metadata_size',C.c_size_t),('power_scale',P),('power_scale_size',C.c_size_t),('remainders',P),('remainders_size',C.c_size_t)]
class SceneObjectContext(C.Structure):
 _fields_=[('shared',P),('hole',P),('player',P),('aim',P),('saved_map',P)]
scene_object_keep=[]
def scene_span(a,n):
 b=blob(data(a,n));scene_object_keep.append(b);return addr(b)
scene_object_resources=SceneObjectResources(scene_span(0x219755,55),55,scene_span(0x2217bb-128,256),256,scene_span(0x21b82e,10),10)
