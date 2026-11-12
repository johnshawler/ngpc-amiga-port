#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Resolve original perfect-shot child resources privately."""
from shot_trajectory_case_common import *
class PerfectResources(C.Structure):_fields_=[('palettes',Job*2),('player_art',Asset*6),('overlay',Asset)]
class Perfect(C.Structure):_fields_=[(n,P) for n in ('first_delay','visible_delay','finish')]+[('duration',U16),('kind',U8),('phase',U8)]
perfect_resources=PerfectResources();perfect_jobmeta=[]
for i,command in enumerate((0x48,0x50)):
 packed=load(0x2198f3+command,4);kind=packed&255;desc=packed>>8
 if kind==4:
  count=load(desc,1);op=load(desc+2,4);ob=blob(data(op,count*2));cs=max(load(op+2*j,2) for j in range(count))+8;cb=blob(data(load(0x26001c,4),cs));tee_keep.extend((ob,cb));job=Job(1,0,load(desc+1,1)*8,count,0,0,ob,cb,len(ob),len(cb))
 else:
  assert kind==32;job=Job(2,load(desc+5,1),480+load(desc,1),0,load(desc+1,2),load(desc+3,2),None,None,0,0)
 perfect_resources.palettes[i]=job;perfect_jobmeta.append((command,desc))
for g in range(6):perfect_resources.player_art[g]=tee_asset(0x4c,load(0x21ee99+g,1))
perfect_resources.overlay=tee_asset(0x88,0)
