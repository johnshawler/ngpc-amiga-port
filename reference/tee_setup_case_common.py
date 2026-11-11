#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private tee overview adapter. No private asset bytes are embedded."""
from course_intro_case_common import *
from cursor_native_case_common import Cursor
class HoleArt(C.Structure):_fields_=[('views',Asset*3),('marker_x',U16),('marker_y',U16)]
class TeeResources(C.Structure):
 _fields_=[('palettes',Job*2),('font',Asset),('header',Asset),('hole_numbers',Asset*18),('pars',Asset*3),('handicap',Asset*2),('holes',(HoleArt*18)*3),('metadata',P*4),('metadata_size',C.c_size_t*4),('objects',P)]
class TeeContext(C.Structure):
 _fields_=[(n,P) for n in ('intro','tasks','fallback','objects','x','tiles')]+[(n,U8) for n in ('game_mode','input_mode','control')]+[('linked_controls',U8*2)]+[(n,U8) for n in ('course','round_index','hole_index','par')]+[(n,U16) for n in ('distance','pin_x','pin_y','origin_x','origin_y','text_base')]+[(n,P) for n in ('saved_hole_scores','handicaps')]+[(n,U8) for n in ('reset_kind','language')]+[(n,P) for n in ('prepare','prepare_context','sound_request','sound_context')]
class Tee(C.Structure):_fields_=[(n,P) for n in ('notice_entry','notice_wait','setup_delay','open_delay','hold','handicap_entry','handicap_wait')]+[('asset_group',U16),('asset_index',U16),('phase',U8),('finish_outcome',U8)]
class Handicap(C.Structure):_fields_=[('wait',P),('cleanup',P),('phase',U8),('finish_outcome',U8)]
class TeeOwner(C.Structure):_fields_=[('overview_entry',P),('overview_wait',P)]+[(n,U8) for n in ('phase','skip_overview','player_status','course','current_music')]+[(n,P) for n in ('sound_request','sound_context')]
tee_keep=[];tee_assets=[]
def tee_asset(group,offset,font=False):
 root=load(load(0x260020,4)+group,4)&0xffffff;source=load(root+offset,4);count=load(source,2);p=source+6+2*count
 if not font:
  header=load(p,1);p+=1;width=2 if header&128 else 1
  for _ in range(header&127):
   while load(p,width)!=(65535 if width==2 else 255):p+=width
   p+=width
 size=max(load(source+6+2*i,2) for i in range(count))+16
 ab=blob(data(source,p-source));tb=blob(data(load(source+2,4),size));tee_keep.extend((ab,tb));a=Asset(ab,len(ab),tb,len(tb));tee_assets.append((source,a));return a
jobs=[];tee_jobmeta=[]
for command in (0x84,0x88):
 packed=load(0x2198f3+command,4);kind=packed&255;desc=packed>>8
 if kind==4:
  count=load(desc,1);op=load(desc+2,4);ob=blob(data(op,count*2));cs=max(load(op+2*i,2) for i in range(count))+8;cb=blob(data(load(0x26001c,4),cs));tee_keep.extend((ob,cb));jobs.append(Job(1,0,load(desc+1,1)*8,count,0,0,ob,cb,len(ob),len(cb)))
 else:
  assert kind==32;jobs.append(Job(2,load(desc+5,1),480+load(desc,1),0,load(desc+1,2),load(desc+3,2),None,None,0,0))
 tee_jobmeta.append((command,desc))
tee_resources=TeeResources();tee_resources.palettes[:]=jobs
tee_resources.font=tee_asset(0xb4,0x80,True);tee_resources.header=tee_asset(0x64,0)
for i in range(18):tee_resources.hole_numbers[i]=tee_asset(0x8c,4*(i+1))
for i in range(3):tee_resources.pars[i]=tee_asset(0xb4,4*(i+33))
for i in range(2):tee_resources.handicap[i]=tee_asset(0x2c,0x88+4*i)
for course in range(3):
 for hole in range(18):
  a=tee_resources.holes[course][hole]
  for view in range(3):a.views[view]=tee_asset(4*(course+36),4*hole+0x48*view)
  position=load(0x21ba69+4*course,4)+4*hole;a.marker_x=load(position,2);a.marker_y=load(position+2,2)
for i,p in enumerate((0x2197e6,0x2197e1,0x2197eb,0x2197f0)):
 b=blob(data(p,5));tee_keep.append(b);tee_resources.metadata[i]=addr(b);tee_resources.metadata_size[i]=5
# Object service resources are independently resolved by tee_object_case_common;
# isolated setup only registers objects, and never reads this non-null identity.
tee_resources.objects=1
