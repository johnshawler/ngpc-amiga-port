#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Resolve user-owned original swing resources privately, without extracted output."""
from shot_trajectory_case_common import *
class TrajectorySwingResources(C.Structure):
 _fields_=[(n,P) for n in ('putter_frames','terrain_sounds','club_sounds','hud_bases')]+[(n,C.c_size_t) for n in ('putter_frames_size','terrain_sounds_size','club_sounds_size','hud_bases_size')]+[('hud',((Asset*15)*3)*6)]
class TrajectorySwing(C.Structure):
 _fields_=[('animate',P),('timer',U8),('result',U8),('phase',U8)]
swing_resources=TrajectorySwingResources(scene_span(0x20b5c1,16),scene_span(0x21b882,15),scene_span(0x21b891,23),scene_span(0x21ee8d,6),16,15,23,6)
def swing_asset(offset):
 root=load(load(0x260020,4)+0xac,4)&0xffffff;source=load(root+offset,4);count=load(source,2)
 if count:return tee_asset(0xac,offset)
 p=source+6;header=load(p,1);p+=1;width=2 if header&128 else 1
 for _ in range(header&127):
  while load(p,width)!=(65535 if width==2 else 255):p+=width
  p+=width
 ab=blob(data(source,p-source));tb=blob(bytes(16));tee_keep.extend((ab,tb));a=Asset(ab,len(ab),tb,len(tb));tee_assets.append((source,a));return a
for g in range(6):
 for k in range(3):
  for f in range(15):swing_resources.hud[g][k][f]=swing_asset(4*load(0x21ee8d+g,1)+60*k+4*f)
