#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private power-meter resources and ctypes layouts. No test execution."""
from shot_scene_objects_case_common import *
import os,random
class Meter(C.Structure):
 _fields_=[('wait',P),('position',U16),('velocity',U16)]+[(n,U8) for n in ('upper','lower','hold','phase')]
class MeterResources(C.Structure):
 _fields_=[('speed_player',P),('speed_lie',P),('speed_player_size',C.c_size_t),('speed_lie_size',C.c_size_t),('scale',SceneObjectResources),('glyphs',Asset*9)]
r=MeterResources(scene_span(0x221785-128,257),scene_span(0x22179d-128,257),257,257,scene_object_resources)
for j in range(9):r.glyphs[j]=tee_asset(0x9c,4*(45+j),True)

meter_resources=r
