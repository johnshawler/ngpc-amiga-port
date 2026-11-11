#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private initial-shot banner adapter. Source only; art remains caller supplied."""
from tee_setup_case_common import *
import hole_prepare_case_common as H
class Player(C.Structure):_fields_=[('selection',U16),('active_player',U8),('other_player',U8),('stroke_snapshot',U8)]
class BannerResources(C.Structure):
 _fields_=[('palettes',Job*2),('terrain',Asset),('characters',Asset*6),('players',Asset*2),('tee_shot',Asset),('portrait',Asset),('hints',Asset*2),('metadata',P),('metadata_size',C.c_size_t)]
class BannerContext(C.Structure):
 _fields_=[('shared',P),('hole',P),('player',P),('controlling_player',U8),('suppress_sound',U8)]+[(n,P) for n in ('exit_palette_request','exit_palette_context','raster')]
class Raster(C.Structure):
 _fields_=[(n,U8) for n in ("lower_scroll","upper_scroll","timer_run","timer_mode","timer_divisor","gpu_control","callback","counter","interrupt_shadow")]
class Banner(C.Structure):
 _fields_=[(n,P) for n in ('delay','animation_wait','hold','exit_wait','notice_entry','animation_entry','exit_entry')]+[(n,U8) for n in ('phase','blink_delay','blink_state','finish_outcome')]
class BannerAnimation(C.Structure):_fields_=[('wait',P),('variant',U16),('inactive',U8),('phase',U8)]
class BannerExit(C.Structure):_fields_=[('tick',P),('phase',U8)]
banner_resources=BannerResources();banner_jobmeta=[]
for i,command in enumerate((0x64,0x68)):
 packed=load(0x2198f3+command,4);kind=packed&255;desc=packed>>8
 if kind==4:
  count=load(desc,1);op=load(desc+2,4);ob=blob(data(op,count*2));cs=max(load(op+2*j,2) for j in range(count))+8;cb=blob(data(load(0x26001c,4),cs));tee_keep.extend((ob,cb));job=Job(1,0,load(desc+1,1)*8,count,0,0,ob,cb,len(ob),len(cb))
 else:
  assert kind==32;job=Job(2,load(desc+5,1),480+load(desc,1),0,load(desc+1,2),load(desc+3,2),None,None,0,0)
 banner_resources.palettes[i]=job;banner_jobmeta.append((command,desc))
banner_resources.terrain=tee_asset(12,load(0x21b85c+8,2))
for i in range(6):banner_resources.characters[i]=tee_asset(12,4*(i+1))
for i in range(2):banner_resources.players[i]=tee_asset(12,4*(i+7))
banner_resources.tee_shot=tee_asset(12,36);banner_resources.portrait=tee_asset(0x68,88)
for i in range(2):banner_resources.hints[i]=tee_asset(0x78+4*i,0x3c)
bmd=blob(data(0x2197c8,5));tee_keep.append(bmd);banner_resources.metadata=addr(bmd);banner_resources.metadata_size=5
