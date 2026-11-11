#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Unchanged20b3aa/20b432 nonstack RAM and queued resource oracle."""
from shot_trajectory_case_common import *
import os,random
lib=C.CDLL(os.environ['NTM_SHOT_TRAJECTORY_LIBRARY']);lib.ntm_shot_trajectory_initialize.argtypes=[P,P,P,P,U8];lib.ntm_shot_trajectory_tick.argtypes=[P,P,P,U8]
r=random.Random(0x20b3aa);cases=0
for init in (True,False):
 for g in range(6):
  for label in range(256):
   setup();put(0x4000,r.randbytes(16384));store(0x40fa,16,1)
   d=Display.from_buffer_copy(r.randbytes(C.sizeof(Display)));objects=(Cursor*32).from_buffer_copy(r.randbytes(C.sizeof(Cursor)*32));x=(U16*32)(*[r.randrange(65536) for _ in range(32)])
   full=label%7==0;ids=[j for j in range(32) if j!=16 or full];r.shuffle(ids);d.zero_flags[8]=len(ids);d.zero_words[0]=2*len(ids)
   for j in range(32):
    d.list[j]=ids[j] if j<len(ids) else r.randrange(65536);objects[j].id=d.object_ids[j]=j
   q=Screen();q.stamp_bytes=(0,248,256)[label%3]
   palette=Palette.from_buffer_copy(r.randbytes(C.sizeof(Palette)));occupied=label%9
   for j in range(8):palette.slots[j]=4*j+(0x400 if j<occupied else 0xff00)
   h=AM.State.from_buffer_copy(r.randbytes(C.sizeof(AM.State)));p=AM.Player(0,label%2,1-label%2,0);h.players[p.active_player][1]=g
   aim=AM.Aim();aim.reset_d1=1;words=(U16*2)(r.randrange(65536),r.randrange(65536))
   ic=Context();ic.display=addr(d);ic.palette=addr(palette);ic.screen=addr(q);ic.screen_words=addr(words)
   tc=TeeContext();tc.intro=addr(ic);tc.objects=addr(objects);tc.x=addr(x)
   ctx=SceneObjectContext(addr(tc),addr(h),addr(p),addr(aim),None)
   s=Trajectory();s.wait=0x20b432;s.phase=0 if init else 1;s.last_label=label if label%5==0 else r.randrange(256);s.counter=r.randrange(256);s.sprite_base=4*load(0x21ee93+g,1);s.hud_base=4*load(0x21ee8d+g,1);s.hud_delta=r.randrange(65536)
   task=Task();task.callback=0x20b3aa if init else 0x20b432
   def serial(b):
    serial_display(d,b);serial_objects(objects,x,b);serial_palette(palette,b)
    b[0x149:0x189]=bytes(h.players[0]);b[0x189:0x1c9]=bytes(h.players[1]);write(b,0x41c9,0x4149+64*p.active_player,4);write(b,0x41d1,aim.reset_d1);write(b,0x41dd,label)
    write(b,0x6000,task.callback,4);write(b,0x6016,s.counter);write(b,0x6018,s.hud_base,2);write(b,0x601a,s.sprite_base,2);write(b,0x601c,s.hud_delta,2);write(b,0x601e,s.last_label);write(b,0x4d84,q.stamp_bytes,2)
    for j in range(2):write(b,0x4d80+2*j,words[j],2)
   b=bytearray(ram[:16384]);serial(b);put(0x4000,b);before=q.stamp_bytes//8
   result=lib.ntm_shot_trajectory_initialize(C.byref(s),C.byref(task),C.byref(ctx),C.byref(trajectory_resources),label) if init else lib.ntm_shot_trajectory_tick(C.byref(s),C.byref(ctx),C.byref(trajectory_resources),label)
   assert result==0,(init,g,label,result)
   if init:assert d.object_tags[16]==objects[16].tag
   serial(b)
   if init and occupied<8:write_job(b,occupied,palette.jobs[occupied],(0x30+4*g,trajectory_descriptors[g]))
   for j in range(before,q.stamp_bytes//8):
    st=q.stamps[j];source=next(src for src,a in tee_assets if C.cast(a.data,P).value==C.cast(st.asset.data,P).value)
    write(b,0x4d86+8*j,source,4);write(b,0x4d8a+8*j,0x9000+st.offset,2);write(b,0x4d8c+8*j,st.tile,2)
   reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x20b3aa if init else 0x20b432);low=0x6be0
   for _ in range(5000):
    if get(8)==0xef00:break
    step(0);low=min(low,get(7))
   else:raise AssertionError(('budget',hex(get(8))))
   actual=bytes(ram[:16384]);b[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
   assert actual==b,(init,g,label,[(hex(0x4000+i),a,v) for i,(a,v) in enumerate(zip(actual,b)) if a!=v][:20])
   cases+=1
print(json.dumps({'cases':cases,'scope':'unchanged original20b3aa/20b432/20b49e full nonstack RAM and queued asset/palette identities; all labels, six golfers, queue saturation'}))
