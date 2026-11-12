#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original queued perfect notice drawing, including complete video and RAM."""
from shot_perfect_case_common import *
import os,random
lib=C.CDLL(os.environ['NTM_SHOT_PERFECT_LIBRARY']);lib.ntm_course_screen_stamp_queue.argtypes=[P,P,P]
R=random.Random(0x20c6bb)
for k in range(24):
 setup();put(0x4000,R.randbytes(16384));put(0x8000,R.randbytes(16384))
 assets=(perfect_resources.player_art[k%6],perfect_resources.overlay)
 q=Screen();q.stamp_bytes=16;store(0x4d84,16,2)
 for j,a in enumerate(assets):
  source=next(src for src,b in tee_assets if C.cast(b.data,P).value==C.cast(a.data,P).value)
  store(0x4d86+8*j,source,4);store(0x4d8a+8*j,0x9000+0x800*j,2);store(0x4d8c+8*j,(0x40,0x132)[j],2)
  q.stamps[j].asset=a;q.stamps[j].offset=0x800*j;q.stamps[j].tile=(0x40,0x132)[j]
 before=bytearray(ram[:16384]);video=data(0x8000,16384);tiles=blob(video[8192:]);maps=blob(video[4096:8192])
 assert lib.ntm_course_screen_stamp_queue(C.byref(q),tiles,maps)==0 and q.stamp_bytes==0
 write(before,0x4d84,0,2);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x20054f);low=0x6be0
 for _ in range(100000):
  if get(8)==0xef00:break
  step(0);low=min(low,get(7))
 else:raise AssertionError(('budget',hex(get(8))))
 actual=bytes(ram[:16384]);before[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==before,(k,'RAM')
 assert data(0x8000,16384)==video[:4096]+bytes(maps)+bytes(tiles),(k,'video')
print(json.dumps({'draw_cases':24,'scope':'original20054f queued perfect notice; full nonstack RAM/hardware/maps/tiles'}))
