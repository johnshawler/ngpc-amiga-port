#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original209e3a queued panorama and registered object, full nonstack RAM."""
from shot_aim_setup_case_common import *
import os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_AIM_SETUP_LIBRARY',str(ROOT/'build/shot-probe/aim-setup.so')))
lib.ntm_shot_aim_panorama.argtypes=[C.POINTER(TeeContext),C.POINTER(AimSetupResources),U8,U8]
R=random.Random(0x209e3a);cases=0
for course in range(3):
 for hole in range(18):
  for stamps,objects_count in ((0,0),(31,17),(32,32)):
   setup();put(0x4000,R.randbytes(16384));store(0x4204,course,1);store(0x4205,hole,1)
   q=Screen();q.stamp_bytes=stamps*8;d=Display.from_buffer_copy(R.randbytes(C.sizeof(Display)));objects=(Cursor*32).from_buffer_copy(R.randbytes(C.sizeof(Cursor)*32));x=(U16*32)(*[R.randrange(65536) for _ in range(32)])
   d.zero_flags[8]=objects_count;d.zero_words[0]=2*objects_count;d.capacity=32
   for j in range(objects_count):d.list[j]=j if j<4 else j+1 if objects_count<32 else j
   for j,o in enumerate(objects):o.id=d.object_ids[j]=j;o.tag=d.object_tags[j]
   ic=Context();ic.screen=addr(q);ic.display=addr(d);ctx=TeeContext();ctx.intro=addr(ic);ctx.objects=addr(objects);ctx.x=addr(x)
   def serial(b):
    serial_display(d,b)
    for j,o in enumerate(objects):
     for name,off in [('tag',2),('y',6),('width',8),('velocity',10),('tile',18),('link',28)]:write(b,0x453b+64*j+off,getattr(o,name),2)
     for name,off in [('id',0),('second',1),('byte14',14),('byte15',15),('flags',16),('byte30',30)]:write(b,0x453b+64*j+off,getattr(o,name))
     write(b,0x453f+64*j,x[j],2)
    write(b,0x4d84,q.stamp_bytes,2)
   expected=bytearray(ram[:16384]);serial(expected);put(0x4000,expected)
   assert lib.ntm_shot_aim_panorama(C.byref(ctx),C.byref(aim_setup_resources),course,hole)==0
   serial(expected)
   for j in range(stamps,q.stamp_bytes//8):
    st=q.stamps[j];source=next(src for src,a in tee_assets if C.cast(a.data,P).value==C.cast(st.asset.data,P).value)
    write(expected,0x4d86+8*j,source,4);write(expected,0x4d8a+8*j,0x9000+st.offset,2);write(expected,0x4d8c+8*j,st.tile,2)
   reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x209e3a);low=0x6be0
   for _ in range(5000):
    if get(8)==0xef00:break
    step(0);low=min(low,get(7))
   else:raise AssertionError(('budget',hex(get(8))))
   actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
   assert actual==expected,(course,hole,stamps,objects_count,[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:20])
   cases+=1
print(json.dumps({'cases':cases,'scope':'original209e3a full nonstack RAM, queued asset identities, genuine object register and saturated queues; animation has separate oracle'}))
