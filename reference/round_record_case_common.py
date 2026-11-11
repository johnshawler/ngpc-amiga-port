#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private resource adapter. Cartridge bytes remain in ignored inputs/output."""
from course_intro_case_common import *
from cursor_native_case_common import Assets,Animation,Cursor
class RoundResources(C.Structure):
 _fields_=[('palettes',Job*2),('font',Asset),('header',Asset),('footer',Asset),('course',(Asset*2)*3),('golfer',(Asset*2)*6),('labels',Asset*2),('missing_score',P),('missing_score_size',C.c_size_t),('metadata',P),('metadata_size',C.c_size_t),('object',P)]
class RoundContext(C.Structure):
 _fields_=[(n,P) for n in ('intro','tasks','fallback','cursor','cursor_x','tiles')]+[(n,U8) for n in ('game_mode','alternate','record_mode','golfer','record_kind','score','course','input_mode','control')]+[('linked_controls',U8*2),('player_markers',U8*2),('text_mode',U8),('text_base',U16),('prepare',P),('prepare_context',P),('sound_request',P),('sound_context',P)]
class Round(C.Structure):_fields_=[(n,P) for n in ('drawing_entry','blink_entry','allocate_blink','open','wait','cleanup')]+[('phase',U8),('finish_outcome',U8)]
class Drawing(C.Structure):_fields_=[('finish',P),('asset_group',U16),('phase',U8)]
class Blink(C.Structure):_fields_=[(n,P) for n in ('initial_delay','shown_delay','hidden_delay')]+[('phase',U8)]
class Prepare(C.Structure):_fields_=[('players',(U8*64)*2),('course_order',U8*36),('course_record_flags',U8*3)]+[(n,U8) for n in ('active_player','mapped_course','course_order_kind','reset_kind','round_index','hole_index')]
round_keep=[];round_assets=[]
def round_asset(group,offset):
 root=load(load(0x260020,4)+group,4)&0xffffff;source=load(root+offset,4)
 count=load(source,2);p=source+6+2*count;header=load(p,1);p+=1;width=2 if header&128 else 1
 for _ in range(header&127):
  while load(p,width)!=(65535 if width==2 else 255):p+=width
  p+=width
 size=max(load(source+6+2*i,2) for i in range(count))+16
 ab=blob(data(source,p-source));tb=blob(data(load(source+2,4),size));round_keep.extend((ab,tb));a=Asset(ab,len(ab),tb,len(tb));round_assets.append((source,a));return a
jobs=[];round_jobmeta=[]
for command in (0x9c,0xa0):
 packed=load(0x2198f3+command,4);kind=packed&255;desc=packed>>8
 if kind==4:
  count=load(desc,1);op=load(desc+2,4);ob=blob(data(op,count*2));cs=max(load(op+2*i,2) for i in range(count))+8;cb=blob(data(load(0x26001c,4),cs));round_keep.extend((ob,cb));jobs.append(Job(1,0,load(desc+1,1)*8,count,0,0,ob,cb,len(ob),len(cb)))
 else:
  assert kind==32;jobs.append(Job(2,load(desc+5,1),480+load(desc,1),0,load(desc+1,2),load(desc+3,2),None,None,0,0))
 round_jobmeta.append((command,desc))
index=32;ap=load(load(load(0x260024,4)+index,4)&0xbfffff,4)&0xffffff
round_shapes=load(load(0x260028,4)+index,4);comp=load(ap+2,2);shape=load(ap+comp,2)&32767
round_count=1
while not load(round_shapes+shape+2*(round_count-1),1)&128:round_count+=1
size=shape+4*round_count+4;source=load(load(round_shapes+shape+2*round_count,4),4);tsize=16*(1+max(load(round_shapes+shape+2*round_count+4+2*i,2) for i in range(round_count)))
ab=blob(data(ap,comp+4));sb=blob(data(round_shapes,size));tb=blob(data(source,tsize));round_keep.extend((ab,sb,tb));round_objects=Assets((Animation*2)(Animation(ab,len(ab),1),Animation()),sb,tb,len(sb),len(tb))
length=1
while load(0x21b282+length-1,1)!=255:length+=1
missing=blob(data(0x21b282,length));md=blob(data(0x2197f5,5));round_keep.extend((missing,md))
round_resources=RoundResources((Job*2)(*jobs),round_asset(0xb4,0x90),round_asset(0x2c,0x84),round_asset(0x2c,0x70),((Asset*2)*3)(*((Asset*2)(round_asset(4*(i+12),8),round_asset(4*(i+12),12)) for i in range(3))),((Asset*2)*6)(*((Asset*2)(round_asset(0x2c,load(0x223b6d+2*i,2)),round_asset(0x28,load(0x223b6d+2*i,2))) for i in range(6))),(Asset*2)(round_asset(0x2c,0x30),round_asset(0x28,0x30)),addr(missing),len(missing),addr(md),len(md),addr(round_objects))
