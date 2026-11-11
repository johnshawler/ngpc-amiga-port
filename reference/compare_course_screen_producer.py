#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# User-supplied original stays private. Run tools/check-course-screen.sh.
from screen_case_common import *
import struct
checks={};n=0
for selection in range(3):
 for cache in range(65536):
  setup();reg(6,0x6700);store(0x40fb,selection,2);store(0x40fd,cache,2);reg(8,0x2095e3)
  for _ in range(8):
   if get(8) in (0xef00,0x209603):break
   step(0)
  native_screen=Screen();native_screen.cached_selection=cache
  for j in range(8):native_screen.fill_status[j]=255
  outcome=native.ntm_course_screen_tick(C.byref(native_screen),selection,assets)
  assert outcome==int(cache!=selection)
  assert get(8)==(0xef00 if outcome==0 else 0x209603)
  assert native_screen.cached_selection==load(0x40fd,2)==selection
  n+=1
checks['native_original_gate_all_cache_words']=n
base_screen=Screen();orig_fills=b'';orig_stamps=b''
for j in range(8):
 f=base_screen.fills[j];f.offset=0x300+j*2;f.fill=j*0x1111;f.width=j+1;f.height=8-j
 orig_fills+=struct.pack('<IBBH',0x9000+f.offset,f.height,f.width,f.fill)
for j in range(32):
 ai=j%3;st=base_screen.stamps[j];st.asset=assets[ai];st.offset=0x302+12*ai;st.tile=301
 orig_stamps+=struct.pack('<IHH',sources[ai],0x9000+st.offset,st.tile)
n=0
for selection in range(3):
 for mask in range(256):
  for depth in range(33):
   setup();reg(6,0x6700);store(0x40fb,selection,2);store(0x40fd,0xbeef,2)
   screen=Screen.from_buffer_copy(base_screen);screen.cached_selection=0xbeef;screen.stamp_bytes=depth*8
   setmem(0x5742,orig_fills);setmem(0x4d86,orig_stamps);store(0x4d84,depth*8,2)
   for j in range(8):screen.fill_status[j]=255 if mask&(1<<j) else j*31;store(0x5782+j,screen.fill_status[j],1)
   run(0x209603);outcome=native.ntm_course_screen_draw(C.byref(screen),selection,assets)
   assert outcome==0 and screen.cached_selection==load(0x40fd,2)
   assert screen.cursor_x==load(0x453f,2) and screen.displayed_selection==load(0x447d,1)
   assert bytes(screen.fill_status)==data(0x5782,8)
   assert screen.stamp_bytes==load(0x4d84,2)
   fills=b''.join(struct.pack('<IBBH',0x9000+f.offset,f.height,f.width,f.fill) for f in screen.fills)
   assert fills==data(0x5742,64)
   expected=bytearray(orig_stamps)
   if depth<32:expected[depth*8:depth*8+8]=struct.pack('<IHH',sources[selection],0x9302+12*selection,301)
   assert expected==data(0x4d86,256)
   for j in range(32):
    ai=selection if j==depth else j%3
    st=screen.stamps[j]
    assert C.addressof(st.asset.data.contents)==C.addressof(assets[ai].data.contents)
    assert st.offset==0x302+12*ai and st.tile==301
   n+=1
checks['native_original_all_course_producer_states']=n
print(json.dumps({'status':'PASS','checks':checks,'native_source_sha256':hashlib.sha256((ROOT/'native-game/course_screen.c').read_bytes()).hexdigest()},sort_keys=True))
