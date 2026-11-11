#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Independent original aim setup comparison, including complete nonstack RAM."""
from shot_aim_case_common import *
import random,os
R=random.Random(0x209751)
lib=C.CDLL(os.environ.get('NTM_SHOT_AIM_LIBRARY',str(ROOT/'build/shot-probe/aim.so')))
lib.ntm_shot_aim_angle.argtypes=[U16]*4+[C.POINTER(AimResources),C.POINTER(U16)]
lib.ntm_shot_aim_at_tee.argtypes=[P];lib.ntm_shot_aim_at_tee.restype=U8
lib.ntm_shot_aim_camera.argtypes=[C.POINTER(Aim),P]
lib.ntm_shot_aim_club.argtypes=[C.POINTER(Aim),C.POINTER(State),C.POINTER(Player),C.POINTER(AimResources)]
lib.ntm_shot_aim_prepare.argtypes=[C.POINTER(Aim),C.POINTER(State),C.POINTER(Player),C.POINTER(AimResources),C.POINTER(U8),C.POINTER(U16)]
def run(pc,stop=0xef00):
 reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
 for _ in range(5000):
  low=min(low,get(7))
  if get(8)==stop:return low
  step(0)
 raise AssertionError(('budget',hex(get(8))))
def check(b,low,label):
 actual=bytearray(ram[:16384]);b[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert b==actual,(label,[(hex(0x4000+i),a,z) for i,(a,z) in enumerate(zip(b,actual)) if a!=z][:20])
def seed():
 setup();put(0x4000,R.randbytes(16384));return read_state(),read_aim(),bytearray(ram[:16384])
counts={}
edges=(0,1,7,8,10,11,95,96,97,1023,1024,2048,32767,32768,65535)
vectors=[(x,y,0,0) for x in edges for y in edges]+[(R.randrange(65536),R.randrange(65536),R.randrange(65536),R.randrange(65536)) for _ in range(4096)]
for j,v in enumerate(vectors):
 h,s,b=seed();out=U16(0xfafa)
 assert lib.ntm_shot_aim_angle(*v,C.byref(aim_resources),C.byref(out))==0
 for i,n in enumerate(v):reg(i,n)
 low=run(0x20d5ca);assert out.value==get(0)&65535,('angle',j,v,out.value,get(0)&65535);check(b,low,('angle RAM',j))
counts['angles']=len(vectors)
for j in range(4096):
 h,s,b=seed()
 if j<225:
  for o,v in ((0x46,edges[j//15]),(0x4a,edges[j%15]),(0x2e,0),(0x30,0)):store(0x4200+o,v,2)
  h=read_state();b=bytearray(ram[:16384])
 out=lib.ntm_shot_aim_at_tee(h.hole);low=run(0x2050e9)
 assert out==get(3)&255,('tee',j,out,get(3)&255);check(b,low,('tee RAM',j))
counts['tee_bounds']=4096
for j in range(4096):
 h,s,b=seed();assert lib.ntm_shot_aim_camera(C.byref(s),h.hole)==0
 low=run(0x204e6c);serialize_aim(s,b);check(b,low,('camera',j))
counts['cameras']=4096
for j in range(12288):
 h,s,b=seed();bank=j&1;p=Player(0,bank,1-bank,0);h.players[bank][1]=(j//2)%6;h.players[bank][0x33]=6*((j//12)&1)
 if j<6144:h.players[bank][3]=0;h.hole[0x70]=(j//24)%15
 c=U8(b[14]);serialize(h,c,b);b[0x1c9:0x1cd]=(0x4149+64*bank).to_bytes(4,'little');put(0x4000,b)
 assert lib.ntm_shot_aim_club(C.byref(s),C.byref(h),C.byref(p),C.byref(aim_resources))==0
 low=run(0x20d8fe);serialize_aim(s,b);check(b,low,('club',j))
counts['clubs']=12288
for j in range(4096):
 h,s,b=seed();bank=j&1;p=Player(0,bank,1-bank,0);h.players[bank][1]=(j//2)%6;h.players[bank][0x33]=6*((j//12)&1);h.hole[0x70]=3 if j%17==0 else 4
 c=U8(b[14]);serialize(h,c,b);b[0x1c9:0x1cd]=(0x4149+64*bank).to_bytes(4,'little');put(0x4000,b);status=U8(load(0x41db,1));screen=(U16*2)(load(0x4d80,2),load(0x4d82,2));green=h.hole[0x70]==3
 assert lib.ntm_shot_aim_prepare(C.byref(s),C.byref(h),C.byref(p),C.byref(aim_resources),C.byref(status),screen)==(2 if green else 0)
 low=run(0x209751,0x209c07 if green else 0x2097f0);serialize(h,c,b);serialize_aim(s,b);b[0x1db]=status.value
 b[0xd80:0xd84]=b''.join(v.to_bytes(2,'little') for v in screen)
 check(b,low,('setup',j))
counts['setups']=4096
print(json.dumps(dict(**counts,scope='original20d5ca,2050e9,204e6c,20d8fe,209751..2097f0; full nonstack RAM; no substituted world loader or shot progression')))
