#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private camera fixture types; no cartridge content embedded."""
from shot_markers_case_common import *
from aim_world_case_common import WorldState,WorldResources,resolve_world
class CameraState(C.Structure):_fields_=[('saved_x',U16),('saved_y',U16)]
