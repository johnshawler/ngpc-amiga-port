#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Isolated original comparisons. User-supplied assets remain private.
from course_palette_case_common import *
execute_original=execute
def execute(pc):
 reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc)
 for i in range(200000):
  if get(8)==0xef00:return
  step(0)
 raise RuntimeError(("limit",hex(get(8))))
class Asset(C.Structure):_fields_=[('data',C.POINTER(U8)),('size',C.c_size_t),('tiles',C.POINTER(U8)),('tiles_size',C.c_size_t)]
class Stamp(C.Structure):_fields_=[('asset',Asset),('offset',U16),('tile',U16)]
class Fill(C.Structure):_fields_=[('offset',U16),('fill',U16),('width',U8),('height',U8)]
class Screen(C.Structure):_fields_=[('fill_status',U8*8),('fills',Fill*8),('stamp_bytes',U16),('stamps',Stamp*32),('cached_selection',U16),('cursor_x',U16),('displayed_selection',U8)]
class Display(C.Structure):_fields_=[('zero_flags',U8*16),('capacity',U8),('limit',U8),('object_ids',U8*32),('pools',(U8*256)*2),('zero_words',U16*2),('object_tags',U16*32),('list',U16*32),('reset_words',C.c_uint32*2)]
class Cursor(C.Structure):_fields_=[(n,U16) for n in ('tag','y','width','velocity','tile','link')]+[(n,U8) for n in ('id','second','byte14','byte15','flags','byte30')]
class Init(C.Structure):_fields_=[(n,U8) for n in ('guard','flags','plane_swap','sprite_x','sprite_y','selection','mapped_course')]+[('screen_words',U16*2),('cursor',Cursor)]
lib=C.CDLL(str(ROOT/'build/course-init-check/native_initialization.so'));lib.ntm_course_initialize.argtypes=[C.POINTER(Init),C.POINTER(Screen),C.POINTER(Display),C.POINTER(Palette),C.POINTER(U8),C.POINTER(U8),C.POINTER(Asset),C.POINTER(U8),C.POINTER(Job)]
lib.ntm_course_palette_tail.argtypes=[C.POINTER(Palette)];lib.ntm_course_palette_head.argtypes=[C.POINTER(Palette)]
lib.ntm_course_screen_fill_queue.argtypes=[C.POINTER(Screen),C.POINTER(U8)];lib.ntm_course_screen_stamp_queue.argtypes=[C.POINTER(Screen),C.POINTER(U8),C.POINTER(U8)]
assets=(Asset*5)();retained=[];sources=[]
table=load(load(0x260020,4)+24,4)&0xffffff
for s in range(5):
 src=load(table+4*s,4);n=load(src,2);q=src+6+2*n;rows=load(q,1)&127;q+=1
 for y in range(rows):
  while load(q,2)!=65535:q+=2
  q+=2
 size=max(load(src+6+2*i,2) for i in range(n))+16;a=buf(data(src,q-src));b=buf(data(load(src+2,4),size));retained.extend([a,b]);assets[s]=Asset(a,len(a),b,len(b));sources.append(src)
descriptor=buf(data(0x219836,5));jobs=(Job*2)(group,single)
zero_addresses=[0x66bb,0x61ba,0x61b4,0x61b5,0x61b7,0x61b8,0x4d80,0x4d82,0x4d7d,0x4d7e,0x6182,0x619d,0x61a9,0x61a6,0x61a7,0x61a8]
asset_addresses={C.addressof(a.data.contents):src for a,src in zip(assets,sources)}
def serial_init(init,screen,display,pal,b):
 def w(a,v,n=1):b[a-0x4000:a-0x4000+n]=int(v).to_bytes(n,'little')
 for a,v in zip(zero_addresses,display.zero_flags):w(a,v)
 w(0x4d7b,display.zero_words[0],2);w(0x6180,display.zero_words[1],2);w(0x66bc,display.capacity);w(0x61aa,display.limit)
 for i in range(32):w(0x453b+64*i,display.object_ids[i]);w(0x453d+64*i,display.object_tags[i],2);w(0x4d3b+2*i,display.list[i],2)
 b[0x21bb:0x22bb]=bytes(display.pools[0]);b[0x22fb:0x23fb]=bytes(display.pools[1]);w(0x5bcb,display.reset_words[0],4);w(0x61ac,display.reset_words[1],4)
 w(0x400d,init.flags);w(0x4204,init.mapped_course);w(0x40fb,init.selection,2);w(0x40fd,screen.cached_selection,2);w(0x447d,screen.displayed_selection);w(0x453f,screen.cursor_x,2);w(0x4d80,init.screen_words[0],2);w(0x4d82,init.screen_words[1],2)
 c=init.cursor
 for a,v,n in ((0x453b,c.id,1),(0x453c,c.second,1),(0x453d,c.tag,2),(0x4541,c.y,2),(0x4543,c.width,2),(0x4545,c.velocity,2),(0x4549,c.byte14,1),(0x454a,c.byte15,1),(0x454b,c.flags,1),(0x454d,c.tile,2),(0x4557,c.link,2),(0x4559,c.byte30,1)):w(a,v,n)
 for i in range(8):
  f=screen.fills[i];w(0x5782+i,screen.fill_status[i]);w(0x5742+8*i,f.offset+0x9000,4);w(0x5746+8*i,f.height);w(0x5747+8*i,f.width);w(0x5748+8*i,f.fill,2)
 w(0x4d84,screen.stamp_bytes,2)
 for i in range(32):
  st=screen.stamps[i];w(0x4d86+8*i,asset_addresses[C.addressof(st.asset.data.contents)],4);w(0x4d8a+8*i,0x9000+st.offset,2);w(0x4d8c+8*i,st.tile,2)
 serialize(pal,b)
 # Original service records preserve fields outside the two initializer jobs.
 for i in range(8):
  j=pal.jobs[i];a=0x66cf+32*i
  if j.kind==1:w(a,4,2);w(a+2,0x124,2);w(a+8,0,2);w(a+10,48,2);w(a+12,0x21a073,4)
  elif j.kind==2:w(a,32,2);w(a+2,0x128,2);w(a+8,480,2);w(a+12,single.color,2);w(a+14,0x118,2);w(a+26,0)
 w(0x7000,0x209366,4)
 return b
