#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Privately resolved original first-fire palette; no embedded asset bytes."""
from shot_markers_case_common import *
class Fire(C.Structure):
 _fields_=[(n,P) for n in ('spin_entry','trajectory_entry','golfer_entry','closed_delay','open_delay','ready')]+[('phase',U8),('boundary',U8)]
class FireContext(C.Structure):_fields_=[('shared',P),('aim',P),('overlay',P)]
class FireResources(C.Structure):_fields_=[('palette',Job)]
fire_keep=[]
fire_palette_command=0x10
packed=load(0x2198f3+fire_palette_command,4);kind=packed&255;fire_palette_descriptor=packed>>8
assert kind==4
count=load(fire_palette_descriptor,1);op=load(fire_palette_descriptor+2,4)
ob=blob(data(op,count*2));cs=max(load(op+2*i,2) for i in range(count))+8
cb=blob(data(load(0x26001c,4),cs));fire_keep.extend((ob,cb))
fire_resources=FireResources(Job(1,0,load(fire_palette_descriptor+1,1)*8,count,0,0,ob,cb,len(ob),len(cb)))
def new_fire():return Fire(0x20b604,0x20b3aa,0x20a58d,0x2099a5,0x2099e1,0x2099f2,0,0)
def serial_objects(objects,x,b):
 for j,o in enumerate(objects):
  for n,off in [('tag',2),('y',6),('width',8),('velocity',10),('tile',18),('link',28)]:write(b,0x453b+64*j+off,getattr(o,n),2)
  for n,off in [('id',0),('second',1),('byte14',14),('byte15',15),('flags',16),('byte30',30)]:write(b,0x453b+64*j+off,getattr(o,n))
  write(b,0x453f+64*j,x[j],2)
def serial_tasks(nodes,addresses,tasks,b):
 mapping={addr(n):a for n,a in zip(nodes,addresses)};mapping[None]=0xffffffff
 for a,n in zip(addresses,nodes):
  write(b,a,n.callback,4)
  for name,off in (('previous',4),('next',8),('owner',12)):write(b,a+off,mapping[getattr(n,name)],4)
  for name,off in (('priority',16),('result',17),('flags',18),('substate',19)):write(b,a+off,getattr(n,name))
  write(b,a+20,n.delay,2)
 write(b,0x5706,mapping[tasks.free_head],4)
 return mapping
