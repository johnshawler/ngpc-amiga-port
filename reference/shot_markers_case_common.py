#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private original inputs for marker children; no embedded cartridge bytes."""
from shot_scene_objects_case_common import *
class MarkerResources(C.Structure):
 _fields_=[('quarter',P),('quarter_size',C.c_size_t),('metadata',P),('metadata_size',C.c_size_t)]
class MarkerContext(C.Structure):
 _fields_=[('shared',P),('hole',P),('aim',P),('dirty',P),('held',U8),('player_status',U8)]
class Markers(C.Structure):
 _fields_=[('tick',P),('waiting',P),('step',U8),('phase',U8),('limit',U16)]
class Hint(C.Structure):
 _fields_=[('delay',P),('active',P)]+[(n,U8) for n in ('repeat','last','pending','changed','phase')]
class Camera(C.Structure):_fields_=[('delay',P),('active',P),('phase',U8)]
marker_resources=MarkerResources(scene_span(0x21d67b,4100),4100,scene_span(0x21975f,10),10)
