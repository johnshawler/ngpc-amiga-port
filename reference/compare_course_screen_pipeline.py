#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# User-supplied original stays private. Run tools/check-course-screen.sh.
from screen_case_common import *
requests=0;pipelines=0
for trial in range(128):
 setup();reg(6,0x6700);screen=Screen();cache=R.randrange(65536);screen.cached_selection=cache;store(0x40fd,cache,2)
 for j in range(8):
  status=R.choice((0xff,0xff,0,1,254));screen.fill_status[j]=status;store(0x5782+j,status,1)
  f=screen.fills[j];f.offset=0x300+2*R.randrange(6);f.width=R.randrange(1,11);f.height=R.randrange(1,7);f.fill=R.randrange(65536)
  d=0x5742+j*8
  for a,v,z in ((d,f.offset+0x9000,4),(d+4,f.height,1),(d+5,f.width,1),(d+6,f.fill,2)):store(a,v,z)
 tiles=bytes(R.randrange(256) for _ in range(8192));maps=bytes(R.randrange(256) for _ in range(4096));setmem(0xa000,tiles);setmem(0x9000,maps);t=blob(tiles);m=blob(maps)
 for _ in range(R.randrange(1,51)):
  s=R.randrange(3);store(0x40fb,s,2);was=screen.cached_selection;run(0x2095e3);r=native.ntm_course_screen_tick(C.byref(screen),s,assets)
  assert r==int(s!=was)
  assert screen.cached_selection==load(0x40fd,2)
  assert screen.cursor_x==load(0x453f,2) and screen.displayed_selection==load(0x447d,1)
  assert bytes(screen.fill_status)==data(0x5782,8)
  assert screen.stamp_bytes==load(0x4d84,2)
  for j in range(8):
   d=0x5742+j*8;f=screen.fills[j]
   assert (f.offset+0x9000,f.height,f.width,f.fill)==(load(d,4),load(d+4,1),load(d+5,1),load(d+6,2))
  for j in range(screen.stamp_bytes//8):
   st=screen.stamps[j];d=0x4d86+j*8
   assert st.offset+0x9000==load(d+4,2) and st.tile==load(d+6,2)
   ai=next(k for k in range(3) if C.addressof(st.asset.data.contents)==C.addressof(assets[k].data.contents))
   assert sources[ai]==load(d,4)
  requests+=1
 assert bytes(t)==data(0xa000,8192) and bytes(m)==data(0x9000,4096),'producer prematurely wrote VRAM'
 run(0x201c25);assert native.ntm_course_screen_fill_queue(C.byref(screen),m)==0
 assert bytes(m)==data(0x9000,4096),('fill result',trial)
 assert bytes(screen.fill_status)==data(0x5782,8)
 run(0x20054f);assert native.ntm_course_screen_stamp_queue(C.byref(screen),t,m)==0
 assert screen.stamp_bytes==load(0x4d84,2)==0
 assert bytes(m)==data(0x9000,4096),('stamp maps',trial)
 assert bytes(t)==data(0xa000,8192),('stamp tiles',trial)
 pipelines+=1
print(json.dumps({'status':'PASS','pipelines':pipelines,'gated_requests':requests,'native_source_sha256':hashlib.sha256((ROOT/'native-game/course_screen.c').read_bytes()).hexdigest()},sort_keys=True))
