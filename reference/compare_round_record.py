#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Isolated original/native normal round record, exact service boundaries."""
from round_record_case_common import *
lib=C.CDLL(os.environ.get('NTM_ROUND_LIBRARY',str(ROOT/'build/round-record-check/native_round_default.so')))
for name,typ in [('initialize',Round),('tick',Round),('drawing_initialize',Drawing),('drawing_finish',Drawing),('blink_initialize',Blink),('blink_tick',Blink)]:
 getattr(lib,'ntm_round_record_'+name).argtypes=[C.POINTER(typ),C.POINTER(Task),C.POINTER(RoundContext),C.POINTER(RoundResources)]
lib.ntm_course_screen_stamp_queue.argtypes=[C.POINTER(Screen),P,P]
pcs=(0x21480a,0x2148b6,0x2148ca,0x21490a,0x214921,0x214941,0x214954,0x214c10,0x214c25,0x214c4b,0x214c73)
for pc in pcs:
 cb=CALL(lambda c,t:0);callbacks[pc]=cb;reverse[C.cast(cb,P).value]=pc
sounds=[]
SOUND=C.CFUNCTYPE(None,P,U16)
@SOUND
def sound(p,v):sounds.append(v)

def run(pc,services=None):
 reg(6,0x7000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc)
 for _ in range(200000):
  pc=get(8)
  if pc==0xef00:return
  if services is not None and pc in (0x250103,0x204a88,0x208414,0x2006cf,0x2037a8,0x200513,0x2012e7,0x201a82,0x205318,0x201363,0x20132b):
   services.append((pc,get(0)&65535 if pc in (0x250103,0x2037a8) else None))
  if pc in (0x250103,0x204a88): # separately verified prepare; requests not audio synthesis
   sp=get(7);reg(8,load(sp,4));reg(7,sp+4);continue
  step(0)
 raise AssertionError(('budget',hex(get(8))))

def seed(stamps=0,occupied=0,descendants=False,reserved=False):
 global q,d,p,o,x,maps,tiles,words,vp,sx,sy,flags,ic,c,tc,t,owner,end,child,nodes,s,drawing,blink,b
 setup();put(0x4000,R.randbytes(16384));q=Screen();d=Display.from_buffer_copy(R.randbytes(C.sizeof(Display)));p=Palette();o=Cursor.from_buffer_copy(R.randbytes(C.sizeof(Cursor)));x=U16(R.randrange(65536))
 o.id=d.object_ids[0];o.tag=d.object_tags[0]
 maps=blob(R.randbytes(4096));tiles=blob(R.randbytes(8192));words=(U16*2)(R.randrange(65536),R.randrange(65536));vp=blob(bytes((7,9,0,152)));sx=blob(R.randbytes(2));sy=blob(R.randbytes(2));flags=U8(R.randrange(256))
 t=Task();owner=Task();end=Task();child=Task();nodes=[t,owner,end,child]
 for node in nodes:node.callback=fn(0x21480a);node.owner=addr(owner)
 t.previous=addr(owner);t.next=addr(child) if descendants else addr(end);t.priority=1;t.flags=int(reserved);t.delay=R.randrange(65536)
 owner.next=addr(t);owner.previous=addr(end);end.next=addr(owner);end.previous=addr(child) if descendants else addr(t)
 child.previous=addr(t);child.next=addr(end);child.priority=2;child.flags=int(reserved)
 tc=TaskContext(None,addr(t));ic=Context(addr(q),addr(d),addr(p),addr(maps),addr(sx),addr(sy),addr(flags),addr(vp),addr(words))
 c=RoundContext();c.intro=addr(ic);c.tasks=addr(tc);c.fallback=addr(end);c.cursor=addr(o);c.cursor_x=addr(x);c.tiles=addr(tiles);c.score=255;c.sound_request=C.cast(sound,P).value
 for i in range(8):p.slots[i]=(0x400 if i<occupied else 0xff00)+4*i
 p.status=R.randrange(65536);q.stamp_bytes=8*stamps
 for i in range(32):q.stamps[i]=Stamp(round_resources.header,0,0x180)
 s=Round(*(fn(n) for n in (0x214941,0x214c10,0x2148b6,0x2148ca,0x21490a,0x214921)),0,0)
 drawing=Drawing(fn(0x214954),0,0);blink=Blink(fn(0x214c25),fn(0x214c4b),fn(0x214c73),0)
 b=bytearray(ram[:16384]);serial(b);put(0x4000,b);put(0x9000,maps);put(0xa000,tiles);put(0x8002,vp)
 for a,v in zip((0x8032,0x8033,0x8034,0x8035),(sx[0],sy[0],sx[1],sy[1])):store(a,v,1)
 store(0x40fa,16,1);store(0x41e9,0,1);store(0x41ec,0,1);store(0x41ed,0,1);store(0x417c,0,1);store(0x417d,255,1);store(0x4148,0,1);store(0x40c0,0,1);store(0x4204,0,1);store(0x414a,0,1)
 b=bytearray(ram[:16384]);sounds.clear()

def serial(b):
 serial_display(d,b);serial_palette(p,b)
 for name,off in [('tag',2),('y',6),('width',8),('velocity',10),('tile',18),('link',28)]:write(b,0x453b+off,getattr(o,name),2)
 for name,off in [('id',0),('second',1),('byte14',14),('byte15',15),('flags',16),('byte30',30)]:write(b,0x453b+off,getattr(o,name))
 write(b,0x453f,x.value,2);write(b,0x400d,flags.value);write(b,0x4d80,words[0],2);write(b,0x4d82,words[1],2)
 write(b,0x416d,c.player_markers[0]);write(b,0x41ad,c.player_markers[1]);write(b,0x4019,c.text_mode);write(b,0x401a,0x9000+c.text_base,2)
 write(b,0x4d84,q.stamp_bytes,2)
 for j in range(q.stamp_bytes//8):
  st=q.stamps[j];source=next(src for src,a in round_assets if C.cast(a.data,P).value==C.cast(st.asset.data,P).value)
  write(b,0x4d86+8*j,source,4);write(b,0x4d8a+8*j,0x9000+st.offset,2);write(b,0x4d8c+8*j,st.tile,2)
 addresses={addr(v):0x7000+64*i for i,v in enumerate(nodes)}
 def old(v):return addresses[v] if v else 0xffffffff
 for j,v in enumerate(nodes):
  a=0x7000+64*j
  for off,value,size in ((0,reverse[v.callback],4),(4,old(v.previous),4),(8,old(v.next),4),(12,old(v.owner),4),(16,v.priority,1),(17,v.result,1),(18,v.flags,1),(19,v.substate,1),(20,v.delay,2)):write(b,a+off,value,size)
 write(b,0x5706,old(tc.free_head),4)

def check(label):
 serial(b);compare_ram(b,label);assert bytes(maps)==data(0x9000,4096),(label,'map');assert bytes(tiles)==data(0xa000,8192),(label,'tiles');assert bytes(vp)==data(0x8002,4),(label,'viewport')
 assert list((sx[0],sy[0],sx[1],sy[1]))==[load(a,1) for a in (0x8032,0x8033,0x8034,0x8035)]

initial=draws=0
for occupied in range(9):
 for stamps in range(33):
  seed(stamps,occupied);assert lib.ntm_round_record_initialize(C.byref(s),C.byref(t),C.byref(c),C.byref(round_resources))==0
  # Serialize only newly allocated palette records, old occupied records preserved.
  for j in range(min(2,8-occupied)):write_job(b,occupied+j,round_resources.palettes[j],round_jobmeta[j])
  services=[];run(0x21480a,services);check(('initialize',occupied,stamps))
  assert sounds==[65533]
  assert services==[(0x250103,65533),(0x204a88,None),(0x208414,None),(0x2006cf,None),(0x2037a8,0x9c),(0x2037a8,0xa0),(0x200513,None),(0x2012e7,None)],services
  initial+=1
for course in range(3):
 for golfer in range(6):
  for stamps in (0,28,30,31,32):
   seed(stamps);c.course=course;c.golfer=golfer;store(0x4204,course,1);store(0x414a,golfer,1);b=bytearray(ram[:16384])
   assert lib.ntm_round_record_drawing_initialize(C.byref(drawing),C.byref(t),C.byref(c),C.byref(round_resources))==0
   write(b,0x7016,drawing.asset_group,2);run(0x214941);check(('drawing first',course,golfer,stamps))
   assert lib.ntm_course_screen_stamp_queue(C.byref(q),tiles,maps)==0;run(0x20054f);check('drawing head')
   assert lib.ntm_round_record_drawing_finish(C.byref(drawing),C.byref(t),C.byref(c),C.byref(round_resources))==1
   run(0x214954);check(('drawing second',course,golfer,stamps))
   assert lib.ntm_course_screen_stamp_queue(C.byref(q),tiles,maps)==0;run(0x20054f);check('drawing second head');draws+=1
later=0
for phase,pc in ((1,0x2148b6),(2,0x2148ca),(3,0x21490a)):
 for delay in (0,1,239,32767,32768,32769,65535):
  for skip in (0,16):
   for desc in (False,True):
    for reserved in (False,True):
     seed(descendants=desc,reserved=reserved);s.phase=phase;t.callback=fn(pc);t.delay=delay;c.control=skip
     serial(b);put(0x4000,b);store(0x40c0,skip,1);b=bytearray(ram[:16384])
     assert lib.ntm_round_record_tick(C.byref(s),C.byref(t),C.byref(c),C.byref(round_resources))>=0
     run(pc);check(('later',phase,delay,skip,desc,reserved));later+=1
blinks=0
for phase,pc in ((0,0x214c10),(1,0x214c25),(2,0x214c4b),(3,0x214c73)):
 for delay in (0,1,4,29,32767,32768,32769,65535):
  for count in (0,1,31,32):
   seed();c.text_mode=1;c.text_base=0x4c0;blink.phase=phase;t.callback=fn(pc);t.delay=delay;d.zero_flags[8]=count;d.zero_words[0]=2*count
   serial(b);put(0x4000,b);b=bytearray(ram[:16384])
   call=lib.ntm_round_record_blink_initialize if not phase else lib.ntm_round_record_blink_tick
   assert call(C.byref(blink),C.byref(t),C.byref(c),C.byref(round_resources))==0
   run(pc);check(('blink',phase,delay,count));blinks+=1
print(json.dumps({'initializations':initial,'drawing_cases':draws,'later_cases':later,'blink_cases':blinks,'scope':'full nonstack RAM, direct narrow-font and footer, queue saturation, palette jobs, requests/order, task allocation exhaustion, cleanup/descendants, text maps, blink registration including full lists'}))
