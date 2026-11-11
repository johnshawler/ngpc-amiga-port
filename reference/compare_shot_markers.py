#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exact unchanged-original20b03b/20aee1/20af8a normal-status comparison."""
from shot_markers_case_common import *
import hashlib,os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_MARKER_LIBRARY',str(ROOT/'build/shot-probe/markers.so')))
R=random.Random(0x209830);cases=0
entries=[('position',0x20b03b),('initialize',0x20aee1),('tick',0x20af8a)]
for name,pc in entries:
 fn=getattr(lib,'ntm_shot_markers_'+name);fn.argtypes=([] if name=='position' else [C.POINTER(Markers),C.POINTER(Task)])+[C.POINTER(MarkerContext),C.POINTER(MarkerResources)]
 for case in range(512):
  setup();put(0x4000,R.randbytes(16384));put(0x9000,R.randbytes(4096))
  d=Display.from_buffer_copy(R.randbytes(C.sizeof(Display)));objects=(Cursor*32).from_buffer_copy(R.randbytes(C.sizeof(Cursor)*32));x=(U16*32)(*[R.randrange(65536) for _ in range(32)])
  h=AM.State.from_buffer_copy(R.randbytes(C.sizeof(AM.State)));p=AM.Player(0,case%2,1-case%2,0);aim=AM.Aim();aim.club=case%16
  words=(U16*2)(*[R.randrange(65536) for _ in range(2)]);maps=blob(data(0x9000,4096));saved=blob(data(0x44ff,32))
  slots=(2,3)
  available=[j for j in range(32) if j not in slots]
  count=0 if case%3==0 else len(available) if case%3==1 else 32
  d.zero_flags[8]=count;d.zero_words[0]=2*count;d.capacity=34;d.zero_flags[0]=0
  for j in range(count):d.list[j]=j if count==32 else available[j]
  for j,o in enumerate(objects):o.id=d.object_ids[j]=j;o.tag=d.object_tags[j]
  ic=Context();ic.display=addr(d);ic.screen_words=addr(words);ic.maps=addr(maps)
  tc=TeeContext();tc.intro=addr(ic);tc.objects=addr(objects);tc.x=addr(x)
  dirty=U8(case%2);ctx=MarkerContext(addr(tc),addr(h),addr(aim),addr(dirty),case%16,0)
  aim.angle=R.randrange(4096);aim.initial_angle=R.randrange(65536);aim.angle_delta=R.randrange(65536);aim.reset_d1=case%4;aim.reset_4416=case%3
  state=Markers(0x20af8a,0x20afa5,4,0 if name=='initialize' else 1,512);task=Task();task.callback=pc
  h.hole[0x6a:0x6c]=(R.choice([0,48,49,160,161,32767,32768,65535])).to_bytes(2,'little')
  def serial(b):
   serial_display(d,b)
   for j,o in enumerate(objects):
    for n,off in [('tag',2),('y',6),('width',8),('velocity',10),('tile',18),('link',28)]:write(b,0x453b+64*j+off,getattr(o,n),2)
    for n,off in [('id',0),('second',1),('byte14',14),('byte15',15),('flags',16),('byte30',30)]:write(b,0x453b+64*j+off,getattr(o,n))
    write(b,0x453f+64*j,x[j],2)
   b[0x149:0x189]=bytes(h.players[0]);b[0x189:0x1c9]=bytes(h.players[1]);b[0x200:0x300]=bytes(h.hole)
   write(b,0x41c9,0x4149+64*p.active_player,4);write(b,0x41cd,0x4149+64*p.other_player,4);write(b,0x41dc,aim.club)
   for j in range(2):write(b,0x4d80+2*j,words[j],2)
   b[0x4ff:0x51f]=bytes(saved)
   AM.serialize_aim(aim,b);write(b,0x41db,0);write(b,0x41da,dirty.value)
   write(b,0x6000,task.callback,4);write(b,0x6016,state.step);write(b,0x6017,state.limit,2)
   write(b,0x4149+64*p.active_player+0x31,0x4100,4);write(b,0x4100,ctx.held)
  expected=bytearray(ram[:16384]);serial(expected);put(0x4000,expected)
  result=fn(*(() if name=='position' else (C.byref(state),C.byref(task))),C.byref(ctx),C.byref(marker_resources))
  assert result==(2 if name=="tick" and aim.reset_d1==1 else 0),(name,case,'native',result)
  if name=='initialize':
   assert all(objects[j].tag==d.object_tags[j] for j in slots),('native duplicate tag synchronization',case)
  serial(expected)
  reg(0,aim.angle);reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
  for _ in range(10000):
   if get(8)==0xef00 or (result==2 and get(8)==0x20b1d6):break
   step(0);low=min(low,get(7))
  else:raise AssertionError(('budget',name,hex(get(8))))
  actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
  assert actual==expected,(name,case,[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:20])
  cases+=1
print(json.dumps({'cases':cases,'scope':'marker positioning, initialization and early control, full nonstack RAM; status0; reset1 stops before20b1d6',
    'source_sha256':hashlib.sha256((ROOT/'native-game/shot_markers.c').read_bytes()).hexdigest()}))
