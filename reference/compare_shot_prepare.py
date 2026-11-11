#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Independent original shot-preparation helper comparison; no helper stubs."""
from hole_prepare_case_common import *
import random,os
class Player(C.Structure):_fields_=[('selection',U16),('active_player',U8),('other_player',U8),('stroke_snapshot',U8)]
class ShotResources(C.Structure):
 _fields_=[('traits',P),('traits_size',C.c_size_t)]+[(n,P) for n in ('wind_low','wind_step','wind_limit')]+[(n,C.c_size_t) for n in ('wind_low_size','wind_step_size','wind_limit_size')]
r=ShotResources(span(0x21ee81-128,256),256,span(0x2050b9,16),span(0x2050d9,16),span(0x2050a7,18),16,16,18)
lib=C.CDLL(os.environ.get('NTM_SHOT_PREPARE_LIBRARY',str(ROOT/'build/shot-check/native-prepare.so')))
lib.ntm_shot_prepare_geometry.argtypes=[C.POINTER(Player),C.POINTER(State),C.POINTER(ShotResources)]
lib.ntm_shot_advance_stroke.argtypes=[C.POINTER(Player),C.POINTER(State)]
lib.ntm_shot_update_wind.argtypes=[C.POINTER(State),C.POINTER(ShotResources),C.POINTER(U8),P]
R=random.Random(2096)
def seed():
 setup();put(0x4000,R.randbytes(16384));return read_state(),U8(load(0x400e,1)),bytearray(ram[:16384])
def run(pc,stop=0xef00):
 reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
 for _ in range(6000):
  low=min(low,get(7))
  if get(8)==stop:return low
  step(0)
 raise AssertionError(('budget',hex(get(8))))
def check(s,c,b,low,label):
 serialize(s,c,b);actual=bytearray(ram[:16384]);b[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 if b!=actual:
  dif=[(hex(0x4000+i),x,y) for i,(x,y) in enumerate(zip(b,actual)) if x!=y]
  raise AssertionError((label,dif[:24]))
geometry=0
for selection in (0,4,8):
 for j in range(512):
  s,c,b=seed();p=Player(selection,197,198,R.randrange(256));store(0x41ea,selection,2);b[0x1ea:0x1ec]=selection.to_bytes(2,'little')
  assert lib.ntm_shot_prepare_geometry(C.byref(p),C.byref(s),C.byref(r))==0
  low=run(0x2096bf,0x20970a);b[0x1c9:0x1cd]=(0x4149+64*p.active_player).to_bytes(4,'little');b[0x1cd:0x1d1]=(0x4149+64*p.other_player).to_bytes(4,'little')
  check(s,c,b,low,('geometry',selection,j));geometry+=1
stroke=0
for bank in range(2):
 for n in range(256):
  for terrain,counter in ((3,0),(0,0),(3,255),(255,255),(0,128)):
   s,c,b=seed();s.players[bank][2]=n;s.players[bank][42]=counter;s.hole[0x73]=terrain
   serialize(s,c,b);b[0x1c9:0x1cd]=(0x4149+64*bank).to_bytes(4,'little');put(0x4000,b);p=Player(0,bank,1-bank,0)
   assert lib.ntm_shot_advance_stroke(C.byref(p),C.byref(s))==0
   low=run(0x20d8b4);b[0x1e7]=p.stroke_snapshot;check(s,c,b,low,('stroke',bank,n,terrain,counter));stroke+=1
wind=0
for difficulty in range(8):
 for selected in range(8):
  for cursor in range(256):
   s,c,b=seed();s.difficulty=difficulty;s.selected_difficulty=selected;s.hole[6]=cursor%18;s.alternate=0;s.secondary_mode=0;c.value=cursor
   serialize(s,c,b);put(0x4000,b)
   assert lib.ntm_shot_update_wind(C.byref(s),C.byref(r),C.byref(c),window)==0
   low=run(0x20500e);check(s,c,b,low,('wind',difficulty,selected,cursor));wind+=1
for j in range(1024):
 s,c,b=seed();s.hole[6]=j%18;s.alternate=j%2;s.secondary_mode=j%4;serialize(s,c,b);put(0x4000,b)
 assert lib.ntm_shot_update_wind(C.byref(s),C.byref(r),C.byref(c),window)==0
 low=run(0x20500e);check(s,c,b,low,('wind branch',j));wind+=1
print(json.dumps(dict(geometry=geometry,stroke=stroke,wind=wind,scope='Full nonstack RAM, real prerequisite calls and shared RNG; no terrain or banner substitution')))
