#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original swing parent, real result wait and signed hit delay; no fake child."""
from shot_fire_case_common import *
import os,random
class Swing(C.Structure):
 _fields_=[('waiting',P),('hit_delay',P),('hit_reset',P),('phase',U8),('boundary',U8)]
class SwingContext(C.Structure):_fields_=[('scene',P),('overlay',P),('suppress_sound',U8)]
lib=C.CDLL(os.environ.get('NTM_SHOT_SWING_LIBRARY',str(ROOT/'build/shot-swing-check/native.so')))
for n in ('initialize','tick'):getattr(lib,'ntm_shot_swing_'+n).argtypes=[P,P,P]
r=random.Random(0x209aa2);cases=0;sounds=[]
Sound=C.CFUNCTYPE(None,P,U8,U16,U16)
@Sound
def sound(_,service,command,parameter):sounds.append((service,command,parameter))
def video():return data(0x8000,0x4000)
def check(pc,result=0,delay=0,suppress=0,flags=0):
 global cases
 setup();put(0x4000,r.randbytes(16384));a=AM.read_aim();t=Task();objects=(Cursor*32)();overlay=U8(load(0x413b,1))
 t.callback=pc;t.previous=t.next=t.owner=0;t.priority=load(0x6010,1);t.result=result;t.flags=load(0x6012,1);t.substate=load(0x6013,1);t.delay=delay
 objects[16].flags=flags
 q=TeeContext();q.objects=addr(objects);q.sound_request=C.cast(sound,P).value
 scene=SceneObjectContext();scene.shared=addr(q);scene.aim=addr(a)
 c=SwingContext(addr(scene),addr(overlay),suppress)
 s=Swing(0x209ad3,0x209af4,0x209b05,{0x209aa2:0,0x209ad3:1,0x209af4:2}[pc],0)
 def serial(b):
  AM.serialize_aim(a,b);write(b,0x413b,overlay.value);write(b,0x447e,suppress)
  write(b,0x494b,objects[16].flags);write(b,0x6000,t.callback,4);write(b,0x6011,t.result);write(b,0x6014,t.delay,2)
 expected=bytearray(ram[:16384]);serial(expected);put(0x4000,expected);before_video=video();sounds.clear()
 fn=lib.ntm_shot_swing_initialize if pc==0x209aa2 else lib.ntm_shot_swing_tick
 rc=fn(C.byref(s),C.byref(t),C.byref(c));serial(expected);expected_sounds=sounds[:]
 reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0;original_sounds=[]
 for _ in range(1000):
  stop=get(8)
  if stop in (0xef00,0x209b05,0x209b4b):break
  if stop==0x2500ea:
   original_sounds.append((0,get(0)&65535,0));sp=get(7);reg(8,load(sp,4));reg(7,sp+4);continue
  step(0);low=min(low,get(7))
 else:raise AssertionError(('budget',hex(get(8))))
 assert rc==(0 if stop==0xef00 else 2),(cases,hex(pc),rc,hex(stop))
 assert s.boundary=={0xef00:0,0x209b05:1,0x209b4b:2}[stop],(cases,'boundary')
 actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==expected,(cases,hex(pc),result,delay,[(hex(i+0x4000),v,w) for i,(v,w) in enumerate(zip(actual,expected)) if v!=w][:20])
 assert video()==before_video,(cases,'video')
 assert original_sounds==expected_sounds,(cases,'sounds',original_sounds,expected_sounds)
 cases+=1
for suppress in range(256):check(0x209aa2,result=r.randrange(256),delay=r.randrange(65536),suppress=suppress,flags=r.randrange(256))
for result in range(256):
 for flags in (0,1,63,64,127,128,191,255):check(0x209ad3,result,delay=r.randrange(65536),flags=flags)
for delay in (0,1,2,3,4,32767,32768,32769,65534,65535):
 for result in range(256):check(0x209af4,result,delay,flags=r.randrange(256))
print(json.dumps({'cases':cases,'scope':'complete nonstack RAM/video,real task result/callback/delay and conditional sound request; explicit boundaries before hit209b05 and miss209b4b; no child bypass'}))
