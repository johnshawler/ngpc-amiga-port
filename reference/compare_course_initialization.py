#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Isolated original comparisons. User-supplied assets remain private.
from course_init_case_common import *
cases=0
for case in range(768):
 setup();setram(0x4000,R.randbytes(16384));store(0x4144,0,1);store(0x40fa,16+case%240,1);reg(6,0x7000)
 init=Init();screen=Screen();display=Display();p=Palette();init.flags=load(0x400d,1);screen.displayed_selection=case%3;store(0x447d,case%3,1)
 # Valid randomized, potentially full, persistent service queue.
 perm=list(range(8));R.shuffle(perm);free=case%9
 for i,index in enumerate(perm):
  kind=1+R.randrange(2);p.slots[i]=((4 if kind==1 else 32)*256 if i<free else 0xff00)+index*4;store(0x66bf+2*i,p.slots[i],2)
  if i<free:
   p.jobs[index]=group if kind==1 else single;a=0x66cf+32*index;store(a,4 if kind==1 else 32,2);store(a+2,0x124 if kind==1 else 0x128,2);store(a+8,0 if kind==1 else 480,2)
   if kind==1:store(a+10,48,2);store(a+12,0x21a073,4)
   else:store(a+12,single.color,2);store(a+14,0x118,2);store(a+26,0,1)
 p.status=load(0x66bd,2);p.copy_bytes=0;store(0x67ef,0,2)
 for i in range(8):p.copies[i]=Copy(load(0x67cf+4*i,2),load(0x67d1+4*i,2))
 for i in range(512):p.staging[i]=load(0x67f1+i,1);v=R.randrange(256);store(0x8200+i,v,1);p.palette[i]=v
 p.pending[0]=load(0x69f1,1);p.pending[1]=load(0x69f2,1);store(0x8118,0x80,1);store(0x8012,0x80,1);p.background=load(0x8118,1);p.window_control=load(0x8012,1)
 for i in range(8):
  f=Fill(2*R.randrange(32),R.randrange(65536),1+R.randrange(12),1+R.randrange(8));screen.fills[i]=f;screen.fill_status[i]=255 if case&(1<<i) else R.choice((0,1,254));store(0x5782+i,screen.fill_status[i],1);store(0x5742+8*i,f.offset+0x9000,4);store(0x5746+8*i,f.height,1);store(0x5747+8*i,f.width,1);store(0x5748+8*i,f.fill,2)
 screen.stamp_bytes=(case%33)*8;store(0x4d84,screen.stamp_bytes,2)
 for i in range(32):
  sel=R.randrange(3);screen.stamps[i]=Stamp(assets[sel+2],0x302+12*sel,301);store(0x4d86+8*i,sources[sel+2],4);store(0x4d8a+8*i,0x9302+12*sel,2);store(0x4d8c+8*i,301,2)
 sprites=buf(R.randbytes(256));spal=buf(R.randbytes(64));tiles=buf(R.randbytes(8192));maps=buf(R.randbytes(4096))
 for a,blob in ((0x8800,sprites),(0x8c00,spal),(0xa000,tiles),(0x9000,maps)):
  for i,v in enumerate(blob):store(a+i,v,1)
 store(0x87e2,0,1)
 expected=bytearray(ram[:16384]);r=lib.ntm_course_initialize(C.byref(init),C.byref(screen),C.byref(display),C.byref(p),sprites,spal,assets,descriptor,jobs);assert r==0
 serial_init(init,screen,display,p,expected);execute(0x209215);compare(p,expected)
 assert bytes(sprites)==data(0x8800,256);assert bytes(spal)==data(0x8c00,64)
 assert [init.plane_swap,init.sprite_x,init.sprite_y]==[load(x,1) for x in (0x8030,0x8020,0x8021)]
 assert bytes(tiles)==data(0xa000,8192);assert bytes(maps)==data(0x9000,4096)
 # Drawing drains are isolated at their next-frame boundaries; palette pump has a separate oracle.
 execute(0x201c25);assert lib.ntm_course_screen_fill_queue(C.byref(screen),maps)==0;assert bytes(maps)==data(0x9000,4096)
 execute(0x20054f);assert lib.ntm_course_screen_stamp_queue(C.byref(screen),tiles,maps)==0;assert bytes(maps)==data(0x9000,4096);assert bytes(tiles)==data(0xa000,8192)
 cases+=1
print(json.dumps({'cases':cases,'source_sha256':hashlib.sha256((ROOT/'native-game/course_initialization.c').read_bytes()).hexdigest()}))
