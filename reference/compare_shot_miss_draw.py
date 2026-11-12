#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original queued miss notice drawing, including complete video and RAM."""
from shot_miss_case_common import *
import os,random
lib=C.CDLL(os.environ['NTM_SHOT_MISS_LIBRARY']);lib.ntm_course_screen_stamp_queue.argtypes=[P,P,P]
R=random.Random(0x210a11)
a=miss_resources.notice;source=next(src for src,b in tee_assets if C.cast(b.data,P).value==C.cast(a.data,P).value)
for k in range(16):
 setup();put(0x4000,R.randbytes(16384));put(0x8000,R.randbytes(16384))
 store(0x4d84,8,2);store(0x4d86,source,4);store(0x4d8a,0x91ca,2);store(0x4d8c,0x22f8,2)
 before=bytearray(ram[:16384]);video=data(0x8000,16384);tiles=blob(video[8192:]);maps=blob(video[4096:8192]);q=Screen();q.stamp_bytes=8;q.stamps[0].asset=a;q.stamps[0].offset=0x1ca;q.stamps[0].tile=0x22f8
 assert lib.ntm_course_screen_stamp_queue(C.byref(q),tiles,maps)==0 and q.stamp_bytes==0
 write(before,0x4d84,0,2);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x20054f);low=0x6be0
 for _ in range(100000):
  if get(8)==0xef00:break
  step(0);low=min(low,get(7))
 else:raise AssertionError(('budget',hex(get(8))))
 actual=bytes(ram[:16384]);before[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==before,(k,'RAM')
 assert data(0x8000,16384)==video[:4096]+bytes(maps)+bytes(tiles),(k,'video')
print(json.dumps({'draw_cases':16,'scope':'original20054f queued miss notice; full nonstack RAM/hardware/maps/tiles'}))
