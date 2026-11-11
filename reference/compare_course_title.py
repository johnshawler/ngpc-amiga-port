#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Isolated original/native title lifecycle oracle; private input only.
Uses existing screen-probe build, never rebuilds it. No ROM bytes in source.
"""
from course_intro_case_common import *
lib=C.CDLL(os.environ.get('NTM_TITLE_LIBRARY',str(ROOT/'build/course-title-check/native_title.so')))
class Cursor(C.Structure):_fields_=[(n,U16) for n in ('tag','y','width','velocity','tile','link')]+[(n,U8) for n in ('id','second','byte14','byte15','flags','byte30')]
class TitleResources(C.Structure):_fields_=[('descriptor',P),('descriptor_size',C.c_size_t),('metadata',P),('metadata_size',C.c_size_t),('title',Asset),('tournament',Asset*2)]
class TitleContext(C.Structure):_fields_=[('intro',P),('cursor',P),('cursor_x',P),('tasks',P),('game_mode',U8),('input_mode',U8),('control',U8),('linked_controls',U8*2)]
class Title(C.Structure):_fields_=[(n,P) for n in ('title_wait','tournament_delay','tournament_wait','cleanup','descriptor_next')]+[('asset_group',U16),('phase',U8),('finish_outcome',U8)]
for a in (0x2078da,0x207959,0x207990,0x2079a7):
 cb=CALL(lambda c,t:0);callbacks[a]=cb;reverse[C.cast(cb,P).value]=a
lib.ntm_course_title_initialize.argtypes=[C.POINTER(Title),C.POINTER(Task),C.POINTER(TitleContext),C.POINTER(TitleResources),C.POINTER(Intro)]
lib.ntm_course_title_tick.argtypes=[C.POINTER(Title),C.POINTER(Task),C.POINTER(TitleContext),C.POINTER(TitleResources)]
lib.ntm_course_title_skip.argtypes=[U8,U8,P]
lib.ntm_display_reset.argtypes=[C.POINTER(Display)]
lib.ntm_course_screen_stamp_queue.argtypes=[C.POINTER(Screen),P,P]
def asset(source):
 count=load(source,2);end=source+6+2*count;rows=load(end,1)&127;end+=1
 for y in range(rows):
  while load(end,2)!=65535:end+=2
  end+=2
 size=max(load(source+6+2*i,2) for i in range(count))+16
 ab=blob(data(source,end-source));tb=blob(data(load(source+2,4),size));retained.extend((ab,tb));return Asset(ab,len(ab),tb,len(tb))
def cursor_serial(o,b):
 for n,off in (('tag',2),('y',6),('width',8),('velocity',10),('tile',18),('link',28)):write(b,0x453b+off,getattr(o,n),2)
 for n,off in (('id',0),('second',1),('byte14',14),('byte15',15),('flags',16),('byte30',30)):write(b,0x453b+off,getattr(o,n))
 write(b,0x453f,cx.value,2)
def serial(b):
 serial_display(d,b);cursor_serial(o,b)
 write(b,0x7000,reverse[t.callback],4);write(b,0x7014,t.delay,2);write(b,0x7016,s.asset_group,2);write(b,0x7018,dp+s.descriptor_next-addr(db),4)
 write(b,0x4d80,words[0],2);write(b,0x4d82,words[1],2);write(b,0x4d84,screen.stamp_bytes,2)
 for j in range(screen.stamp_bytes//8):
  q=screen.stamps[j];src=next(src for src,a in assets if C.cast(a.data,P).value==C.cast(q.asset.data,P).value)
  write(b,0x4d86+j*8,src,4);write(b,0x4d8a+j*8,0x9000+q.offset,2);write(b,0x4d8c+j*8,q.tile,2)
cases=0;delays=0
for course in range(3):
 dp=metadata[course][0];db=blob(data(dp,12));md=blob(data(0x21980e,5));group=load(load(0x260020,4)+load(dp,2),4)&0xffffff
 assets=[(load(group+off,4),asset(load(group+off,4))) for off in (12,8,16)]
 r=TitleResources(addr(db),12,addr(md),5,assets[0][1],(Asset*2)(assets[1][1],assets[2][1]))
 for stamps in range(33):
  for count in (0,1,31,32):
   setup();put(0x4000,R.randbytes(16384));reg(6,0x7000)
   screen=Screen();d=Display();lib.ntm_display_reset(C.byref(d));o=Cursor.from_buffer_copy(R.randbytes(C.sizeof(Cursor)));cx=U16(R.randrange(65536));o.id=d.object_ids[0];o.tag=d.object_tags[0]
   d.zero_flags[8]=count;d.zero_words[0]=count*2;d.zero_flags[9]=R.randrange(256)
   maps=blob(R.randbytes(4096));tiles=blob(R.randbytes(8192));vp=blob(bytes((0,0,160,152)));words=(U16*2)(R.randrange(65536),R.randrange(65536))
   ctx=Context(addr(screen),addr(d),None,addr(maps),None,None,None,addr(vp),addr(words));tc=TaskContext();c=TitleContext(addr(ctx),addr(o),addr(cx),addr(tc),0,0,0,(U8*2)(0,0))
   t=Task();t.callback=fn(0x20781a);t.delay=65535;intro=Intro(fn(0x2077d3),fn(0x207809),fn(0x20781a),addr(db)+4,load(dp,2),3)
   s=Title(fn(0x2078da),fn(0x207959),fn(0x207990),fn(0x2079a7),addr(db)+4,load(dp,2),0,0)
   screen.stamp_bytes=8*stamps
   for j in range(32):screen.stamps[j]=Stamp(assets[0][1],0x188,load(dp+4,2))
   b=bytearray(ram[:16384]);serial(b);put(0x4000,b);put(0x9000,maps);put(0xa000,tiles);put(0x8002,vp);store(0x40c0,0,1);store(0x4148,0,1)
   b=bytearray(ram[:16384]);assert lib.ntm_course_title_initialize(C.byref(s),C.byref(t),C.byref(c),C.byref(r),C.byref(intro))==0
   serial(b);services=[];service_entries.add(0x2007ca);execute(0x20781a,services=services)
   assert services==[(0x200513,None),(0x2007ca,None)],services
   compare_ram(b,('title',course,stamps,count));assert bytes(maps)==data(0x9000,4096)
   assert lib.ntm_course_screen_stamp_queue(C.byref(screen),tiles,maps)==0;execute(0x20054f);write(b,0x4d84,0,2);compare_ram(b,'title stamps');assert bytes(maps)==data(0x9000,4096) and bytes(tiles)==data(0xa000,8192)
   # Every supported mode and skip/timer edge, restoring original/native snapshots.
   baseline=bytes(ram[:16384]);snap=[bytes(v) for v in (screen,d,o,cx,maps,tiles,vp,words,t,s)]
   for mode in ((0,1,2,255) if stamps==0 and count==0 else (2,)):
    for delay in ((0,1,59,32767,32768,32769,65535) if stamps==0 and count==0 else (0,)):
     for skip in ((0,16) if stamps==0 and count==0 else (16,)):
      put(0x4000,baseline)
      for v,bs in zip((screen,d,o,cx,maps,tiles,vp,words,t,s),snap):C.memmove(addr(v),bs,len(bs))
      put(0x9000,maps);put(0xa000,tiles);put(0x8002,vp);c.game_mode=mode;c.control=skip;t.delay=delay
      screen.stamp_bytes=8*stamps;store(0x4d84,8*stamps,2)
      store(0x41e9,mode,1);store(0x40c0,skip,1);store(0x7014,delay,2);reg(6,0x7000);b=bytearray(ram[:16384])
      assert lib.ntm_course_title_tick(C.byref(s),C.byref(t),C.byref(c),C.byref(r))==0
      serial(b);services=[];service_entries.add(0x205318);execute(0x2078da,services=services)
      expected_services=[(0x205318,None)]
      if s.phase==2:expected_services += [(0x2006cf,None),(0x201a82,None),(0x200513,None)]
      assert services==expected_services,services
      compare_ram(b,('title wait',course,stamps,count,mode,delay,skip));assert bytes(maps)==data(0x9000,4096);delays+=1
      if s.phase==2:
       assert lib.ntm_course_screen_stamp_queue(C.byref(screen),tiles,maps)==0;execute(0x20054f);write(b,0x4d84,0,2)
       compare_ram(b,'tournament stamps');assert bytes(maps)==data(0x9000,4096) and bytes(tiles)==data(0xa000,8192)
   cases+=1
print(json.dumps({'title_cases':cases,'title_wait_cases':delays,'scope':'full nonstack RAM, registration including full lists, queues, all courses and tournament variants; tile/map drain'}))
# Independent later-delay and cleanup boundaries, including descendants.
later=0
for phase,pc in ((2,0x207959),(3,0x207990)):
 for delay in (0,1,2,59,32767,32768,32769,65534,65535):
  for skip in (0,16):
   for flagsvalue in (0,1):
    for descendants in (False,True):
     setup();put(0x4000,baseline)
     for v,bs in zip((screen,d,o,cx,maps,tiles,vp,words,t,s),snap):C.memmove(addr(v),bs,len(bs))
     put(0x9000,maps);put(0xa000,tiles);put(0x8002,vp)
     owner=Task();end=Task();child=Task();nodes=[t,owner,end,child];addresses={addr(v):0x7000+64*i for i,v in enumerate(nodes)}
     def old(p):return addresses[p] if p else 0xffffffff
     for v in nodes:v.callback=fn(pc);v.owner=addr(owner)
     t.previous=addr(owner);t.next=addr(child) if descendants else addr(end);t.priority=1;t.flags=flagsvalue
     owner.next=addr(t);owner.previous=addr(end);owner.priority=0
     end.previous=addr(child) if descendants else addr(t);end.next=addr(owner);end.priority=0
     child.previous=addr(t);child.next=addr(end);child.priority=2;child.flags=flagsvalue
     tc.current=addr(t);tc.free_head=None;s.phase=phase;s.descriptor_next=addr(db)+12;t.callback=fn(pc);t.delay=delay;c.control=skip
     def task_serial(b):
      for j,v in enumerate(nodes):
       a=0x7000+64*j
       for off,value,size in ((0,reverse[v.callback],4),(4,old(v.previous),4),(8,old(v.next),4),(12,old(v.owner),4),(16,v.priority,1),(17,v.result,1),(18,v.flags,1),(19,v.substate,1),(20,v.delay,2)):write(b,a+off,value,size)
      write(b,0x5706,old(tc.free_head),4)
     b=bytearray(ram[:16384]);serial(b);task_serial(b);put(0x4000,b);store(0x40c0,skip,1);store(0x4148,0,1);reg(6,0x7000);b=bytearray(ram[:16384])
     ret=lib.ntm_course_title_tick(C.byref(s),C.byref(t),C.byref(c),C.byref(r));assert ret>=0
     serial(b);task_serial(b);execute(pc);compare_ram(b,('later',phase,delay,skip,flagsvalue,descendants));assert bytes(vp)==data(0x8002,4)
     assert get(6)==old(tc.current),('current',hex(get(6)),hex(old(tc.current)))
     later+=1
# Input helper linked-pointer resolution is adapter-owned; verify byte semantics
# against the actual helper with both linked pointer chains populated.
inputs=0
for mode in (0,1,255):
 for a in range(256):
  for bvalue in (0,16,255):
   setup();store(0x40c0,a,1);store(0x4148,mode,1);store(0x41c9,0x6000,4);store(0x41cd,0x6100,4)
   store(0x6031,0x6200,2);store(0x6131,0x6300,2);store(0x6201,a,1);store(0x6301,bvalue,1)
   linked=(U8*2)(a,bvalue);before=bytearray(ram[:16384]);result=lib.ntm_course_title_skip(a,mode,linked)
   # The helper SCF/RCF carry is consumed by JR C at2078de. Stop directly on
   # the selected path before any timer/service instruction executes.
   reg(6,0x7000);stop=execute(0x2078da,(0x2078e0,0x2078e7));assert (stop==0x2078e7)==bool(result)
   compare_ram(before,('input skip',mode,a,bvalue));inputs+=1
print(json.dumps({'later_delay_cleanup_cases':later,'original_skip_cases':inputs}))
