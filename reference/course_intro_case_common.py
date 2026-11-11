#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private cartridge oracle for the bounded intro/owner native conversion.

No cartridge bytes, hashes or captures belong in source. Build with
 tools/check-course-intro.sh. The original routines run unchanged; only explicit
 unsupported continuation boundaries stop execution. This is not a full game.
"""
from screen_probe import *
import random
import os
R=__import__('random').Random(0x20771d)
U8=C.c_uint8;U16=C.c_uint16;U32=C.c_uint32;P=C.c_void_p
class Asset(C.Structure):_fields_=[('data',C.POINTER(U8)),('size',C.c_size_t),('tiles',C.POINTER(U8)),('tiles_size',C.c_size_t)]
class Stamp(C.Structure):_fields_=[('asset',Asset),('offset',U16),('tile',U16)]
class Fill(C.Structure):_fields_=[('offset',U16),('fill',U16),('width',U8),('height',U8)]
class Screen(C.Structure):_fields_=[('fill_status',U8*8),('fills',Fill*8),('stamp_bytes',U16),('stamps',Stamp*32),('cached_selection',U16),('cursor_x',U16),('displayed_selection',U8)]
class Display(C.Structure):_fields_=[('zero_flags',U8*16),('capacity',U8),('limit',U8),('object_ids',U8*32),('pools',(U8*256)*2),('zero_words',U16*2),('object_tags',U16*32),('list',U16*32),('reset_words',U32*2)]
class Job(C.Structure):_fields_=[('kind',U8),('pending_index',U8),('offset',U16),('count',U16),('color',U16),('control_offset',U16),('offsets',C.POINTER(U8)),('colors',C.POINTER(U8)),('offsets_size',C.c_size_t),('colors_size',C.c_size_t)]
class Copy(C.Structure):_fields_=[('offset',U16),('words',U16)]
class Palette(C.Structure):_fields_=[('slots',U16*8),('status',U16),('copy_bytes',U16),('jobs',Job*8),('copies',Copy*8),('staging',U8*512),('palette',U8*512),('pending',U8*2),('background',U8),('window_control',U8)]
class Task(C.Structure):_fields_=[('callback',P),('previous',P),('next',P),('owner',P),('priority',U8),('result',U8),('flags',U8),('substate',U8),('delay',U16)]
class TaskContext(C.Structure):_fields_=[('free_head',P),('current',P),('published',P),('controls',P),('selection',U8),('gate',U8),('sound_request',P),('sound_context',P)]
class Records(C.Structure):_fields_=[('active_player',U8),('golfer',U8*2),('record_kind',U8*2),('active',U8*6),('count',U16*6)]
class Owner(C.Structure):_fields_=[('intro_entry',P),('wait_intro',P),('phase',U8)]
class Intro(C.Structure):_fields_=[('scenery_delay',P),('visible_delay',P),('title',P),('descriptor_next',P),('asset_group',U16),('phase',U8)]
class Resources(C.Structure):_fields_=[('descriptor',P),('descriptor_size',C.c_size_t),('background',Asset),('palettes',Job*2)]
class Context(C.Structure):_fields_=[('screen',P),('display',P),('palette',P),('maps',P),('scroll_x',P),('scroll_y',P),('flags',P),('viewport',P),('screen_words',P)]
lib=C.CDLL(os.environ.get('NTM_INTRO_LIBRARY',str(ROOT/'build/course-intro-check/native_intro.so')))
lib.ntm_course_records_prepare.argtypes=[C.POINTER(Records)]
lib.ntm_course_owner_tick.argtypes=[C.POINTER(TaskContext),C.POINTER(Task),C.POINTER(Owner),C.POINTER(Records),C.POINTER(Task)]
lib.ntm_course_intro_initialize.argtypes=[C.POINTER(Intro),C.POINTER(Task),C.POINTER(Context),C.POINTER(Resources)]
lib.ntm_course_intro_tick.argtypes=[C.POINTER(Intro),C.POINTER(Task),C.POINTER(Context)]
lib.ntm_course_palette_tail.argtypes=[C.POINTER(Palette)];lib.ntm_course_palette_head.argtypes=[C.POINTER(Palette)]
lib.ntm_course_screen_fill_queue.argtypes=[C.POINTER(Screen),P];lib.ntm_course_screen_stamp_queue.argtypes=[C.POINTER(Screen),P,P]
def addr(o):return C.addressof(o)
def blob(v):return (U8*len(v)).from_buffer_copy(v)
def data(a,n):return bytes(load(a+i,1) for i in range(n))
def put(a,v):
 if 0x4000<=a and a+len(v)<=0x8000:C.memmove(C.addressof(ram.contents)+a-0x4000,bytes(v),len(v))
 else:
  for i,x in enumerate(v):store(a+i,x,1)
def write(b,a,v,n=1):b[a-0x4000:a-0x4000+n]=int(v).to_bytes(n,'little')
def compare_ram(b,label):
 actual=bytes(ram[:16384]);diff=[(hex(0x4000+i),x,y) for i,(x,y) in enumerate(zip(actual,b)) if x!=y and not 0x2bb0<=i<0x2be4]
 assert not diff,(label,diff[:20])
CALL=C.CFUNCTYPE(C.c_int,P,P);callbacks={};reverse={}
for a in (0x203f31,0x203f59,0x20771d,0x2077d3,0x207809,0x20781a):
 cb=CALL(lambda c,t:0);callbacks[a]=cb;reverse[C.cast(cb,P).value]=a
def fn(a):return C.cast(callbacks[a],P).value
service_entries={0x201a82,0x2006cf,0x2037a8,0x200513}
def execute(pc,stops=(),services=None):
 reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc)
 for i in range(200000):
  pc=get(8)
  if pc==0xef00 or pc in stops:return pc
  if services is not None and pc in service_entries:services.append((pc,get(0)&65535 if pc==0x2037a8 else None))
  step(0)
 raise RuntimeError(('instruction budget',hex(get(8))))
def serial_records(r,b):
 for i in range(6):write(b,0x594f+i*0x27+0x24,r.count[i],2);write(b,0x594f+i*0x27+0x26,r.active[i])
def create_records(bank,index,count,special=False):
 r=Records();r.active_player=bank;r.golfer[0]=r.golfer[1]=index;r.record_kind[0]=r.record_kind[1]=6 if special else 0
 store(0x4144,bank,1)
 for base in (0x4149,0x4189):store(base+1,index,1);store(base+0x33,6 if special else 0,1)
 for i in range(6):r.count[i]=count;r.active[i]=R.randrange(256);store(0x594f+i*0x27+0x24,count,2);store(0x594f+i*0x27+0x26,r.active[i],1)
 return r
retained=[];resources=[];metadata=[]
for course in range(3):
 dp=load(0x21b17d+4*course,4);db=blob(data(dp,4));group=load(load(0x260020,4)+load(dp,2),4)&0xffffff;source=load(group,4)
 count=load(source,2);end=source+6+2*count;rows=load(end,1)&127;end+=1
 for y in range(rows):
  while load(end,2)!=65535:end+=2
  end+=2
 tile_size=max(load(source+6+2*i,2) for i in range(count))+16;ab=blob(data(source,end-source));tb=blob(data(load(source+2,4),tile_size));retained.extend((db,ab,tb))
 jobs=[];jobmeta=[]
 for command in ((course+53)*4,(course+56)*4):
  packed=load(0x2198f3+command,4);kind=packed&255;desc=packed>>8
  if kind==4:
   count=load(desc,1);op=load(desc+2,4);ob=blob(data(op,count*2));cs=max(load(op+2*i,2) for i in range(count))+8;cb=blob(data(load(0x26001c,4),cs));retained.extend((ob,cb));jobs.append(Job(1,0,load(desc+1,1)*8,count,0,0,ob,cb,len(ob),len(cb)))
  else:
   assert kind==32;jobs.append(Job(2,load(desc+5,1),480+load(desc,1),0,load(desc+1,2),load(desc+3,2),None,None,0,0))
  jobmeta.append((command,desc))
 resources.append(Resources(addr(db),4,Asset(ab,len(ab),tb,len(tb)),(Job*2)(*jobs)));metadata.append((dp,source,jobmeta))
zero_addresses=[0x66bb,0x61ba,0x61b4,0x61b5,0x61b7,0x61b8,0x4d80,0x4d82,0x4d7d,0x4d7e,0x6182,0x619d,0x61a9,0x61a6,0x61a7,0x61a8]
def serial_display(d,b):
 for a,v in zip(zero_addresses,d.zero_flags):write(b,a,v)
 write(b,0x4d7b,d.zero_words[0],2);write(b,0x6180,d.zero_words[1],2);write(b,0x66bc,d.capacity);write(b,0x61aa,d.limit)
 for i in range(32):write(b,0x453b+64*i,d.object_ids[i]);write(b,0x453d+64*i,d.object_tags[i],2);write(b,0x4d3b+2*i,d.list[i],2)
 b[0x21bb:0x22bb]=bytes(d.pools[0]);b[0x22fb:0x23fb]=bytes(d.pools[1]);write(b,0x5bcb,d.reset_words[0],4);write(b,0x61ac,d.reset_words[1],4)
def serial_palette(p,b):
 for i in range(8):write(b,0x66bf+2*i,p.slots[i],2);write(b,0x67cf+4*i,p.copies[i].offset,2);write(b,0x67d1+4*i,p.copies[i].words,2)
 write(b,0x66bd,p.status,2);write(b,0x67ef,p.copy_bytes,2);b[0x27f1:0x29f1]=bytes(p.staging);b[0x29f1:0x29f3]=bytes(p.pending)
def write_job(b,index,j,meta):
 command,desc=meta;a=0x66cf+index*32;write(b,a,4 if j.kind==1 else 32,2);write(b,a+2,command,2);write(b,a+8,j.offset,2)
 if j.kind==1:write(b,a+10,j.count,2);write(b,a+12,load(desc+2,4),4)
 else:write(b,a+12,j.color,2);write(b,a+14,j.control_offset,2);write(b,a+26,j.pending_index)
def graphics_match(p,maps,tiles):
 assert bytes(maps)==data(0x9000,4096);assert bytes(tiles)==data(0xa000,8192);assert bytes(p.palette)==data(0x8200,512)
 assert (p.background,p.window_control)==(load(0x8118,1),load(0x8012,1))
