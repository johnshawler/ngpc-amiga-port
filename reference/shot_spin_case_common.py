#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private borrowed spin bounds; original bytes never written to tracked files."""
from shot_scene_objects_case_common import *
class SpinResources(C.Structure):_fields_=[('bounds',P),('bounds_size',C.c_size_t)]
class Spin(C.Structure):_fields_=[('wait',P)]+[(n,U8) for n in ('minimum','maximum','current','phase')]
spin_resources=SpinResources(scene_span(0x21ee9f,24),24)

class SpinDrawResources(C.Structure):_fields_=[('indicators',Asset*9),('highlights',Asset*9)]
spin_draw_resources=SpinDrawResources()
for spin in range(9):
 spin_draw_resources.indicators[spin]=tee_asset(0x9c,4*(spin+1))
 spin_draw_resources.highlights[spin]=tee_asset(0xac,4*spin)
