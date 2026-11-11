#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exact original20a5c0 animation callback and20a94a pin adjustment."""
from shot_golfer_animation_case_common import *
import os,random,itertools
lib=C.CDLL(os.environ.get('NTM_SHOT_GOLFER_ANIMATION_LIBRARY',str(ROOT/'build/shot-golfer-animation-check/native.so')))
lib.ntm_shot_golfer_animation_tick.argtypes=[P,P,P]
lib.ntm_shot_golfer_pin_adjust.argtypes=[P,P,P]
lib.ntm_course_screen_stamp_queue.argtypes=[P,P,P]
r=random.Random(0x20ac01);animation_cases=0;pin_cases=0;drains=0
def video_snapshot():
 # Four adjacent bytes per original bus read; covers the same complete span.
 return b''.join(load(a,4).to_bytes(4,'little') for a in range(0x8000,0xc000,4))
def original(pc,expected,video,budget=30000):
 global drains
 reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
 for _ in range(budget):
  if get(8)==0xef00:break
  step(0);low=min(low,get(7))
 else:raise AssertionError(('budget',hex(pc),hex(get(8))))
 actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==expected,(hex(pc),animation_cases,pin_cases,'RAM',[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:20])
 actual=video_snapshot()
 assert actual==video,(hex(pc),animation_cases,pin_cases,'video',[(hex(0x8000+i),a,b) for i,(a,b) in enumerate(zip(actual,video)) if a!=b][:20])
def check_animation(kind,fg,delay,fgdelay,frame,fgframe,reload,fgreload,queue):
 global animation_cases,drains
 setup();put(0x4000,r.randbytes(16384));put(0x9000,r.randbytes(4096))
 s=Golfer.from_buffer_copy(r.randbytes(C.sizeof(Golfer)));s.phase=1
 s.task_bytes[:]=[frame,delay,reload,kind,fgframe,fgdelay,fgreload,fg]
 screen=Screen();screen.stamp_bytes=queue
 expected=bytearray(ram[:16384]);expected[0x2016:0x201e]=bytes(s.task_bytes);write(expected,0x4d84,queue,2)
 put(0x4000,expected);video=bytearray(video_snapshot())
 assert lib.ntm_shot_golfer_animation_tick(C.byref(s),C.byref(screen),C.byref(golfer_animation_resources))==0
 expected[0x2016:0x201e]=bytes(s.task_bytes);write(expected,0x4d84,screen.stamp_bytes,2)
 for j in range(queue//8,screen.stamp_bytes//8):
  st=screen.stamps[j];source=golfer_animation_sources[C.cast(st.asset.data,P).value]
  write(expected,0x4d86+8*j,source,4);write(expected,0x4d8a+8*j,0x9000+st.offset,2);write(expected,0x4d8c+8*j,st.tile,2)
 original(0x20a5c0,expected,video);animation_cases+=1
 if queue==0 and screen.stamp_bytes:
  maps=blob(video[0x1000:0x2000]);tiles=blob(video[0x2000:0x4000])
  assert lib.ntm_course_screen_stamp_queue(C.byref(screen),tiles,maps)==0
  write(expected,0x4d84,screen.stamp_bytes,2);video[0x1000:0x2000]=bytes(maps);video[0x2000:0x4000]=bytes(tiles)
  original(0x20054f,expected,video);drains+=1
# Every wrapped countdown byte, both channels, all real dispatch kinds.
for kind,fg,delay in itertools.product((0,4,8,12,252),(0,252),range(256)):
 check_animation(kind,fg,delay,255-delay,2*(delay%4),2*((delay//4)%4),delay,255-delay,(0,248,256)[delay%3])
# Every genuine animation frame, simultaneous expiration and queue order/drop.
for kind,fg,frame,fgframe,queue,reload in itertools.product((0,4,8,12,252),(0,252),range(0,8,2),range(0,8,2),(0,248,256),(0,1,7,255)):
 check_animation(kind,fg,1,1,frame,fgframe,reload,255-reload,queue)
# Invalid-looking odd frame bytes are still preserved while the timer runs.
for frame in range(1,256,2):check_animation(0,0,0,255,frame,frame,3,5,0)
for case in range(4096):
 setup();put(0x4000,r.randbytes(16384))
 s=Golfer.from_buffer_copy(r.randbytes(C.sizeof(Golfer)));s.surface=case%15
 h=AM.State.from_buffer_copy(r.randbytes(C.sizeof(AM.State)))
 distance=(0,191,192,193,32767,32768,65535)[case%7] if case<2048 else r.randrange(65536)
 h.hole[0x6a:0x6c]=distance.to_bytes(2,'little')
 objects=(Cursor*32).from_buffer_copy(r.randbytes(C.sizeof(Cursor)*32));x=(U16*32)(*[r.randrange(65536) for _ in range(32)])
 words=(U16*2)(r.randrange(65536),r.randrange(65536));green=GolferGreen(r.randrange(65536),r.randrange(65536),(0,1,254,255)[case%4])
 if case<2048:
  objects[5].velocity=(0,111,112,113,32767,32768,65535)[(case//7)%7]
  green.center_x=(0,7,8,65535,32767,32768)[(case//49)%6];green.half_width=(0,1,32,32767,32768,65535)[(case//294)%6]
  local=(0,17,18,19,72,125,126,127,65535,(green.center_x+72)&65528)[(case//3)%10]
  x[5]=(local+words[0])&65535
 ic=Context();ic.screen_words=addr(words);tc=TeeContext();tc.intro=addr(ic);tc.objects=addr(objects);tc.x=addr(x)
 scene=SceneObjectContext(addr(tc),addr(h),None,None,None)
 def serial(b):
  serial_objects(objects,x,b);b[0x200:0x300]=bytes(h.hole)
  write(b,0x4d80,words[0],2);write(b,0x4d82,words[1],2);write(b,0x4534,s.surface)
  write(b,0x4535,green.center_x,2);write(b,0x4537,green.half_width,2);write(b,0x439f,green.status)
 expected=bytearray(ram[:16384]);serial(expected);put(0x4000,expected);video=video_snapshot()
 assert lib.ntm_shot_golfer_pin_adjust(C.byref(s),C.byref(scene),C.byref(green))==0
 serial(expected);original(0x20a94a,expected,video);pin_cases+=1
print(json.dumps({'animation_callbacks':animation_cases,'pin_adjustments':pin_cases,'drawing_drains':drains,'scope':'full nonstack RAM/video,all countdown bytes,all real scenery frames,queue saturation/order,signed pin distance/animation,wrapped green and screen bounds'}))
