#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private resolved aim inputs. No cartridge data embedded."""
from hole_prepare_case_common import *
class AimResources(C.Structure):
 _fields_=[('angles',P),('angles_size',C.c_size_t),('powers',P*12),('power_sizes',C.c_size_t*12),('minimums',P*6),('minimum_sizes',C.c_size_t*6)]
class Aim(C.Structure):
 _fields_=[(n,U16) for n in ('angle','initial_angle','angle_delta','origin_x','origin_y')]+[(n,U8) for n in ('club','reset_d1','reset_d2','reset_d3','reset_e2','reset_e4','reset_e6','reset_4416')]
class Player(C.Structure):_fields_=[('selection',U16),('active_player',U8),('other_player',U8),('stroke_snapshot',U8)]
aim_resources=AimResources(span(0x21e67f,2050),2050,(P*12)(*[span(load(0x21eeb7+4*i,4),52) for i in range(12)]),(C.c_size_t*12)(*[52]*12),(P*6)(*[span(load(0x21f187+4*i,4)-128,256) for i in range(6)]),(C.c_size_t*6)(*[256]*6))
AIM_FIELDS={'angle':(0x41d6,2),'initial_angle':(0x41d4,2),'angle_delta':(0x41d8,2),'origin_x':(0x5710,2),'origin_y':(0x5712,2),'club':(0x41dc,1),'reset_d1':(0x41d1,1),'reset_d2':(0x41d2,1),'reset_d3':(0x41d3,1),'reset_e2':(0x41e2,1),'reset_e4':(0x41e4,1),'reset_e6':(0x41e6,1),'reset_4416':(0x4416,1)}
def read_aim():return Aim(*[load(a,n) for a,n in AIM_FIELDS.values()])
def serialize_aim(s,b):
 for field,(a,n) in AIM_FIELDS.items():b[a-0x4000:a-0x4000+n]=getattr(s,field).to_bytes(n,'little')
