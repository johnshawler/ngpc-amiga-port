#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Resource-free accuracy-child ctypes layout."""
from shot_scene_objects_case_common import *
class Accuracy(C.Structure):
 _fields_=[('wait',P),('position',U16),('velocity',U16)]+[(n,U8) for n in ('upper','lower','phase')]
