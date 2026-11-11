#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Privately resolve original panorama assets, never embed extracted bytes."""
from tee_setup_case_common import *
class AimSetupResources(C.Structure):
 _fields_=[('panorama',(Asset*18)*3),('metadata',P),('metadata_size',C.c_size_t)]
aim_setup_resources=AimSetupResources()
for course in range(3):
 for hole in range(18):aim_setup_resources.panorama[course][hole]=tee_asset(0xa0+4*course,4*hole)
b=blob(data(0x219769,5));tee_keep.append(b);aim_setup_resources.metadata=addr(b);aim_setup_resources.metadata_size=5
