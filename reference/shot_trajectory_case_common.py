#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private original resolved trajectory resources; no extracted bytes saved."""
from shot_fire_case_common import *
class TrajectoryResources(C.Structure):
 _fields_=[(n,P) for n in ('metadata','sprite_bases','hud_bases')]+[(n,C.c_size_t) for n in ('metadata_size','sprite_bases_size','hud_bases_size')]+[('palettes',Job*6),('hud',(Asset*3)*6)]
class Trajectory(C.Structure):
 _fields_=[('wait',P)]+[(n,U16) for n in ('hud_base','sprite_base','hud_delta')]+[(n,U8) for n in ('counter','last_label','phase')]
trajectory_resources=TrajectoryResources(scene_span(0x2197d7,5),scene_span(0x21ee93,6),scene_span(0x21ee8d,6),5,6,6)
trajectory_descriptors=[]
for g in range(6):
 command=0x30+4*g;packed=load(0x2198f3+command,4);assert packed&255==4
 descriptor=packed>>8;trajectory_descriptors.append(descriptor)
 count=load(descriptor,1);op=load(descriptor+2,4);ob=blob(data(op,count*2));cs=max(load(op+2*i,2) for i in range(count))+8
 cb=blob(data(load(0x26001c,4),cs));fire_keep.extend((ob,cb))
 trajectory_resources.palettes[g]=Job(1,0,load(descriptor+1,1)*8,count,0,0,ob,cb,len(ob),len(cb))
 for k in range(3):trajectory_resources.hud[g][k]=tee_asset(0xac,4*load(0x21ee8d+g,1)+60*k)
