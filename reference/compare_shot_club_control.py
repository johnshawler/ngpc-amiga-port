#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Unchanged-original20ad1a control, redraw/highlight and restore-delay oracle.

The only intercepted call is the existing audio request service, as in the
application oracle. No control, drawing, timer or lifecycle branch is replaced.
"""
from shot_club_case_common import *
import os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_CLUB_LIBRARY',str(ROOT/'build/shot-club-control-check/native_default.so')))
lib.ntm_shot_club_control.argtypes=[C.POINTER(Club),C.POINTER(SceneObjectContext),C.POINTER(ClubResources),U8,U8]
lib.ntm_course_screen_stamp_queue.argtypes=[C.POINTER(Screen),P,P]
R=random.Random(0x20ad1a);cases=0;drains=0;sounds=[]
SOUND=C.CFUNCTYPE(None,P,U8,U16,U16)
@SOUND
def sound(p,service,command,parameter):sounds.append((service,command,parameter))
source_by_asset={C.cast(a.data,P).value:src for src,a in tee_assets}

def seed(power=0,terrain=0,club=3,queued=0,bank=0):
 global q,d,objects,x,h,p,aim,words,maps,tiles,saved,ic,tc,tasks,ctx,s,task,b
 setup();put(0x4000,R.randbytes(16384));q=Screen();q.stamp_bytes=8*queued
 for j in range(32):q.stamps[j]=Stamp(club_resources.labels[0],0x450,0xc4)
 d=Display.from_buffer_copy(R.randbytes(C.sizeof(Display)));objects=(Cursor*32).from_buffer_copy(R.randbytes(C.sizeof(Cursor)*32));x=(U16*32)()
 h=AM.State.from_buffer_copy(R.randbytes(C.sizeof(AM.State)));p=AM.Player(0,bank,1-bank,0);aim=AM.Aim();aim.club=club
 h.players[bank][1]=power%6;h.players[bank][0x33]=6 if power>=6 else 0;h.players[bank][3]=0
 h.players[bank][0x23]=R.randrange(256);h.players[bank][0x31]=0xbf;h.players[bank][0x32]=0x40
 h.hole[0x70]=terrain;h.hole[0x77]=0
 words=(U16*2)(*[R.randrange(65536) for _ in range(2)]);maps=blob(R.randbytes(4096));tiles=blob(R.randbytes(8192));saved=blob(R.randbytes(32))
 ic=Context();ic.display=addr(d);ic.screen=addr(q);ic.screen_words=addr(words);ic.maps=addr(maps)
 tasks=TaskContext()
 tc=TeeContext();tc.intro=addr(ic);tc.objects=addr(objects);tc.x=addr(x);tc.tasks=addr(tasks);tc.sound_request=C.cast(sound,P).value
 ctx=SceneObjectContext(addr(tc),addr(h),addr(p),addr(aim),addr(saved));s=Club();s.wait=0x20ad1a;s.maximum=13;s.minimum=0;s.phase=1;s.label=R.randrange(23);s.change_flag=R.randrange(256)
 task=Task();task.callback=0x20ad1a;b=bytearray(ram[:16384]);put(0x9000,maps);put(0xa000,tiles)

def serial(held,pressed):
 serial_display(d,b);b[0x149:0x189]=bytes(h.players[0]);b[0x189:0x1c9]=bytes(h.players[1]);b[0x200:0x300]=bytes(h.hole)
 write(b,0x41c9,0x4149+64*p.active_player,4);write(b,0x41d1,aim.reset_d1);write(b,0x4416,aim.reset_4416)
 write(b,0x41dc,aim.club);write(b,0x41dd,s.label);write(b,0x41de,s.delta);write(b,0x41e1,s.restore_delay);write(b,0x41da,s.change_flag)
 write(b,0x4d84,q.stamp_bytes,2);write(b,0x6000,task.callback,4);write(b,0x6016,s.maximum);write(b,0x6017,s.minimum)
 write(b,0x47c1,objects[10].y,2);write(b,0x40bf,held);write(b,0x40c0,pressed);b[0x4ff:0x51f]=bytes(saved)
 for j in range(2):write(b,0x4d80+2*j,words[j],2)
 for j in range(q.stamp_bytes//8):
  st=q.stamps[j];write(b,0x4d86+8*j,source_by_asset[C.cast(st.asset.data,P).value],4)
  write(b,0x4d8a+8*j,0x9000+st.offset,2);write(b,0x4d8c+8*j,st.tile,2)

def run(pc):
 original_sounds=[];reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
 for _ in range(20000):
  pc=get(8)
  if pc==0xef00:break
  if pc==0x2501d1:
   original_sounds.append((2,get(0)&65535,get(1)&255));sp=get(7);reg(8,load(sp,4));reg(7,sp+4);continue
  step(0);low=min(low,get(7))
 else:raise AssertionError(('budget',hex(get(8))))
 return low,original_sounds

def check(label,held=0,pressed=0,drain=False):
 global cases,drains
 serial(held,pressed);put(0x4000,b);sounds.clear()
 assert lib.ntm_shot_club_control(C.byref(s),C.byref(ctx),C.byref(club_resources),held,pressed)==0
 serial(held,pressed);low,original_sounds=run(0x20ad1a);actual=bytes(ram[:16384]);b[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==b,(label,[(hex(0x4000+i),a,v) for i,(a,v) in enumerate(zip(actual,b)) if a!=v][:30])
 assert bytes(maps)==data(0x9000,4096),(label,'maps')
 assert bytes(tiles)==data(0xa000,8192),(label,'tiles')
 assert sounds==original_sounds,(label,'sounds',sounds,original_sounds)
 cases+=1
 if drain:
  assert lib.ntm_course_screen_stamp_queue(C.byref(q),tiles,maps)==0
  serial(held,pressed);low,original_sounds=run(0x20054f);actual=bytes(ram[:16384]);b[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
  assert actual==b,(label,'drain RAM',[(hex(0x4000+i),a,v) for i,(a,v) in enumerate(zip(actual,b)) if a!=v][:30])
  assert bytes(maps)==data(0x9000,4096),(label,'drain maps')
  assert bytes(tiles)==data(0xa000,8192),(label,'drain tiles')
  assert not original_sounds;drains+=1

# Every real modifier/terrain table, wrap direction and relevant queue threshold.
for power in range(12):
 for terrain in range(15):
  for queued in (0,27,28,29,30,31,32):
   for edge in (1,2):
    seed(power,terrain,0 if edge==1 else 13,queued,(power+terrain)%2)
    check(('change',power,terrain,queued,edge),edge,edge,True)
# Both simultaneous edges and every held byte: left/right inhibit club changes.
for held in range(256):
 for edge in range(4):
  seed();check(('buttons',held,edge),held,edge)
# Every legal club range and both wrap edges; high input bits are ignored.
for minimum in range(14):
 for maximum in range(minimum,14):
  for edge in (1,2):
   seed(club=minimum if edge==1 else maximum);s.minimum=minimum;s.maximum=maximum
   check(('range',minimum,maximum,edge),0,edge|0xfc)
# Signed state comparison, gate bytes, all nonzero timer values and preexisting deltas.
for state in range(256):
 for gate in range(3):
  seed();aim.reset_d1=state;aim.reset_4416=int(gate==1);h.hole[0x77]=int(gate==2)
  s.restore_delay=1;check(('state gates',state,gate),1,1)
for delay in range(256):
 for delta in (0,1,255):
  seed(6,6,7,27);s.restore_delay=delay;s.delta=delta
  check(('delay',delay,delta),0,0,True)
# Real sequence: repeated presses refresh the delay, held inputs do not repeat;
# all32 expiry ticks, mid-count freeze, blocked control, and exact map restoration.
for power in range(12):
 seed(power,power%15,7)
 for tick in range(96):
  edge=1 if tick in (0,4,12,40) else 2 if tick==41 else 0
  aim.reset_d1=2 if 16<=tick<24 else 0
  check(('sequence',power,tick),1 if tick<16 else 0,edge,True)
print(json.dumps({'cases':cases,'drains':drains,'scope':'20ad1a full nonstack RAM, maps, tiles, sound requests, callbacks, exact assets; signed gates, all held bytes, wrapping, highlights, every delay value, repeated presses and frozen timers'}))
