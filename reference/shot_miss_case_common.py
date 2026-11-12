#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private original miss notice resolver; no extracted bytes published."""
from shot_trajectory_case_common import *
class MissResources(C.Structure):_fields_=[('notice',Asset)]
class Miss(C.Structure):_fields_=[('delay',P),('continuation',P),('phase',U8)]
miss_resources=MissResources(tee_asset(0x9c,0x190))
