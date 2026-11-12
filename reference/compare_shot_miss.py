#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original2109d5/210a27/210a3b full-state and real queued drawing comparison."""
from shot_miss_case_common import *
import os,random
lib=C.CDLL(os.environ['NTM_SHOT_MISS_LIBRARY'])
lib.ntm_shot_miss_initialize.argtypes=[P,P,P,P,P];lib.ntm_shot_miss_tick.argtypes=[P,P];lib.ntm_shot_miss_object_gate.argtypes=[P,U16,U16]
R=random.Random(0x2109d5);cases=0;boundaries=0
for mode in ('init','tick','gate'):
 for k in range(1024):
  setup();put(0x4000,R.randbytes(16384));put(0x8000,R.randbytes(16384));video=data(0x8000,16384)
  objects=(Cursor*32).from_buffer_copy(R.randbytes(C.sizeof(Cursor)*32));x=(U16*32)(*[R.randrange(65536) for _ in range(32)])
  words=(U16*2)(R.randrange(65536),R.randrange(65536));q=Screen();q.stamp_bytes=(0,248,256)[k%3]
  h=AM.State.from_buffer_copy(R.randbytes(C.sizeof(AM.State)));flag=U8(R.randrange(256));left=k%256;width=(0,68,32767,32768,65535)[k%5]
  objects[5].byte15=3 if k%2 else k%256;x[5]=(words[0]+left+(0,1,width,width+1,65535)[k%5])%65536
  ic=Context();ic.screen=addr(q);ic.screen_words=addr(words);tc=TeeContext();tc.intro=addr(ic);tc.objects=addr(objects);tc.x=addr(x)
  ctx=SceneObjectContext(addr(tc),addr(h),None,None,None);s=Miss(0x210a27,0x210a38,0 if mode=='init' else 1);t=Task();t.callback=0x2109d5 if mode=='init' else 0x210a27;t.delay=(0,1,30,32767,32768,32769,65535)[k%7]
  def serial(b):
   serial_objects(objects,x,b);b[0x200:0x300]=bytes(h.hole);write(b,0x413b,flag.value)
   write(b,0x6000,t.callback,4);write(b,0x6014,t.delay,2);write(b,0x4d84,q.stamp_bytes,2)
   for j in range(2):write(b,0x4d80+2*j,words[j],2)
  b=bytearray(ram[:16384]);serial(b);put(0x4000,b);before=q.stamp_bytes//8
  if mode=='init':result=lib.ntm_shot_miss_initialize(C.byref(s),C.byref(t),C.byref(ctx),C.byref(flag),C.byref(miss_resources));pc=0x2109d5
  elif mode=='tick':result=lib.ntm_shot_miss_tick(C.byref(s),C.byref(t));pc=0x210a27
  else:result=lib.ntm_shot_miss_object_gate(C.byref(ctx),left,width);pc=0x210a3b
  assert result>=0;serial(b)
  for j in range(before,q.stamp_bytes//8):
   st=q.stamps[j];source=next(src for src,a in tee_assets if C.cast(a.data,P).value==C.cast(st.asset.data,P).value)
   write(b,0x4d86+8*j,source,4);write(b,0x4d8a+8*j,0x9000+st.offset,2);write(b,0x4d8c+8*j,st.tile,2)
  reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc)
  if mode=='gate':reg(1,left);reg(2,width)
  low=0x6be0
  for _ in range(10000):
   if get(8) in (0xef00,0x210b74):break
   step(0);low=min(low,get(7))
  else:raise AssertionError(('budget',mode,k,hex(get(8))))
  actual=bytes(ram[:16384]);b[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
  assert actual==b,(mode,k,[(hex(0x4000+i),a,v) for i,(a,v) in enumerate(zip(actual,b)) if a!=v][:20])
  assert video==data(0x8000,16384),(mode,k,'video')
  assert (get(8)==0x210b74)==(result==2),(mode,k,'boundary')
  cases+=1;boundaries+=result==2
print(json.dumps({'cases':cases,'post_shot_boundaries':boundaries,'scope':'unchanged original full nonstack RAM/video, notice queue, wrapped gate and signed timer; no intercepted helpers; exact210b74 boundary'}))
