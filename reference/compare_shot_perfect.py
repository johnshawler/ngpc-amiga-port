#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Unchanged perfect notice child: RAM/hardware, queues and real finish."""
from shot_perfect_case_common import *
import os,random
lib=C.CDLL(os.environ['NTM_SHOT_PERFECT_LIBRARY']);lib.ntm_shot_perfect_initialize.argtypes=[P,P,P,P];lib.ntm_shot_perfect_tick.argtypes=[P,P,P]
finish=C.cast(lib.ntm_task_finish,P).value;R=random.Random(0x20c64e);cases=0
for mode in ('init','first','visible'):
 for k in range(768):
  setup();put(0x4000,R.randbytes(16384));put(0x8000,R.randbytes(16384));store(0x40fa,16,1);video=bytearray(data(0x8000,16384))
  q=Screen();q.stamp_bytes=(0,248,256)[k%3];palette=Palette.from_buffer_copy(R.randbytes(C.sizeof(Palette)));occupied=k%9
  for j in range(8):palette.slots[j]=4*j+(0x400 if j<occupied else 0xff00)
  h=AM.State.from_buffer_copy(R.randbytes(C.sizeof(AM.State)));player=AM.Player(0,k%2,1-k%2,0);h.players[player.active_player][1]=k%6
  viewport=blob(video[2:6]);sx=blob(bytes((video[0x32],video[0x34])));sy=blob(bytes((video[0x33],video[0x35])))
  ic=Context();ic.screen=addr(q);ic.palette=addr(palette);ic.viewport=addr(viewport);ic.scroll_x=addr(sx);ic.scroll_y=addr(sy)
  tc=TeeContext();tc.intro=addr(ic);ctx=SceneObjectContext(addr(tc),addr(h),addr(player),None,None)
  s=Perfect(0x20c6f6,0x20c72f,0x20c740,(0,1,30,32767,32768,32769,65535)[k%7],R.randrange(256),0 if mode=='init' else 1 if mode=='first' else 2)
  nodes=(Task*4)();addresses=(0x6100,0x6000,0x6200,0x6300);mapping={addr(n):a for n,a in zip(nodes,addresses)};mapping[None]=0xffffffff
  tasks=TaskContext();tasks.current=addr(nodes[1]);tasks.free_head=addr(nodes[3]);tc.tasks=addr(tasks);lifecycle=k%3
  for i,n in enumerate(nodes):
   n.callback=0x20c64e if mode=='init' else s.first_delay if mode=='first' else s.visible_delay;n.previous=addr(nodes[(i+3)%4]);n.next=addr(nodes[(i+1)%4]);n.owner=addr(nodes[0]);n.priority=22;n.flags=R.randrange(256);n.result=R.randrange(256)
  nodes[2].priority=23 if lifecycle==0 else 22;nodes[1].flags=(nodes[1].flags&254)|(lifecycle==1);nodes[1].delay=(0,1,2,32767,32768,32769,65535)[k%7]
  def serial(b):
   b[0x149:0x189]=bytes(h.players[0]);b[0x189:0x1c9]=bytes(h.players[1]);write(b,0x41c9,0x4149+64*player.active_player,4);serial_palette(palette,b)
   for a,n in zip(addresses,nodes):
    write(b,a,0x20132b if n.callback==finish else n.callback,4)
    for name,off in (('previous',4),('next',8),('owner',12)):write(b,a+off,mapping[getattr(n,name)],4)
    for name,off in (('priority',16),('result',17),('flags',18),('substate',19)):write(b,a+off,getattr(n,name))
    write(b,a+20,n.delay,2)
   write(b,0x5706,mapping[tasks.free_head],4);write(b,0x6016,s.kind);write(b,0x6017,s.duration,2);write(b,0x4d84,q.stamp_bytes,2)
  b=bytearray(ram[:16384]);serial(b);put(0x4000,b);before=q.stamp_bytes//8
  if mode=='init':result=lib.ntm_shot_perfect_initialize(C.byref(s),C.byref(nodes[1]),C.byref(ctx),C.byref(perfect_resources));pc=0x20c64e
  else:result=lib.ntm_shot_perfect_tick(C.byref(s),C.byref(nodes[1]),C.byref(ctx));pc=0x20c6f6 if mode=='first' else 0x20c72f
  assert result>=0,(mode,k,result);serial(b)
  if mode=='init':
   for j in range(min(2,8-occupied)):write_job(b,occupied+j,palette.jobs[occupied+j],perfect_jobmeta[j])
  for j in range(before,q.stamp_bytes//8):
   st=q.stamps[j];source=next(src for src,a in tee_assets if C.cast(a.data,P).value==C.cast(st.asset.data,P).value)
   write(b,0x4d86+8*j,source,4);write(b,0x4d8a+8*j,0x9000+st.offset,2);write(b,0x4d8c+8*j,st.tile,2)
  video[2:6]=bytes(viewport);video[0x32]=sx[0];video[0x34]=sx[1];video[0x33]=sy[0];video[0x35]=sy[1]
  reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
  for _ in range(15000):
   if get(8)==0xef00:break
   step(0);low=min(low,get(7))
  else:raise AssertionError(('budget',mode,k,hex(get(8))))
  actual=bytes(ram[:16384]);b[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
  assert actual==b,(mode,k,[(hex(0x4000+i),a,v) for i,(a,v) in enumerate(zip(actual,b)) if a!=v][:20])
  assert data(0x8000,16384)==video,(mode,k,'video',[(hex(0x8000+i),a,v) for i,(a,v) in enumerate(zip(data(0x8000,16384),video)) if a!=v][:15])
  assert get(6)==mapping[tasks.current],(mode,k,'current');cases+=1
print(json.dumps({'cases':cases,'scope':'unchanged20c64e/20c6f6/20c707/20c72f/20c740 full nonstack RAM/video, palette and art queue identities, real deferred/unlink/recycle completion'}))
