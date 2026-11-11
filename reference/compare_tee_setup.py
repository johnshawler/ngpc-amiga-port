#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original/native overview state, scheduler and direct/deferred graphics.

The isolated initializer intercepts hole preparation at its explicit service
boundary. Its independent oracle and the integrated application execute that
helper; no production path uses this test callback.
"""
from tee_setup_case_common import *
lib=C.CDLL(os.environ.get('NTM_TEE_LIBRARY',str(ROOT/'build/tee-check/native_tee.so')))
for name,typ in [('initialize',Tee),('tick',Tee)]:getattr(lib,'ntm_tee_setup_'+name).argtypes=[C.POINTER(typ),C.POINTER(Task),C.POINTER(TeeContext),C.POINTER(TeeResources)]
lib.ntm_tee_notice.argtypes=[C.POINTER(Task),C.POINTER(TeeContext)]
lib.ntm_tee_handicap_initialize.argtypes=[C.POINTER(Handicap),C.POINTER(Task),C.POINTER(TeeContext),C.POINTER(TeeResources)]
lib.ntm_tee_handicap_tick.argtypes=[C.POINTER(Handicap),C.POINTER(Task),C.POINTER(TeeContext)]
lib.ntm_tee_owner_tick.argtypes=[C.POINTER(TeeOwner),C.POINTER(Task),C.POINTER(TaskContext),C.POINTER(Task)]
lib.ntm_tee_badge_offset.argtypes=[U8,U8];lib.ntm_tee_badge_offset.restype=U8
lib.ntm_course_screen_stamp_queue.argtypes=[C.POINTER(Screen),P,P]
pcs=(0x208753,0x20876f,0x20ca45,0x20cb83,0x20cd28,0x20cd5e,0x20cd81,0x20cdac,0x20ce7c,0x20cf21,0x20cfcc,0x20cfdd)
for pc in pcs:
 cb=CALL(lambda c,t:0);callbacks[pc]=cb;reverse[C.cast(cb,P).value]=pc
SOUND=C.CFUNCTYPE(None,P,U8,U16,U16);PREP=C.CFUNCTYPE(C.c_int,P,C.POINTER(TeeContext));MUSIC=C.CFUNCTYPE(U16,P,U8)
sounds=[];prepared=[];lookups=[];music_result=0
@SOUND
def sound(p,service,command,value):sounds.append((service,command,value))
@PREP
def prepare(p,c):prepared.append(1);return 0
@MUSIC
def music(p,course):lookups.append(course);return music_result

def ret():
 sp=get(7);reg(8,load(sp,4));reg(7,sp+4)
def run(pc,stop=(),services=None):
 reg(6,0x7000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc)
 for _ in range(200000):
  pc=get(8)
  if pc==0xef00 or pc in stop:return pc
  if services is not None and pc in (0x250103,0x2500ea,0x250208,0x204a77,0x2004e2,0x201a82,0x2037a8,0x200513,0x205119,0x2012e7,0x2006cf,0x2007ca,0x2128c6,0x20cdd2,0x20138b,0x205318,0x201363,0x20132b):services.append((pc,get(0)&65535,get(1)&65535))
  if pc in (0x250103,0x2500ea,0x204a77):ret();continue
  step(0)
 raise AssertionError(('budget',hex(get(8))))

def serial(b):
 serial_display(d,b);serial_palette(p,b)
 for j,o in enumerate(objects):
  for name,off in [('tag',2),('y',6),('width',8),('velocity',10),('tile',18),('link',28)]:write(b,0x453b+64*j+off,getattr(o,name),2)
  for name,off in [('id',0),('second',1),('byte14',14),('byte15',15),('flags',16),('byte30',30)]:write(b,0x453b+64*j+off,getattr(o,name))
  write(b,0x453f+64*j,x[j],2)
 write(b,0x400d,flags.value);write(b,0x4d80,words[0],2);write(b,0x4d82,words[1],2);write(b,0x400f,c.text_base,2)
 for name,a,n in [('course',0x4204,1),('round_index',0x4205,1),('hole_index',0x4206,1),('par',0x4215,1),('distance',0x4216,2),('pin_x',0x421a,2),('pin_y',0x421e,2),('origin_x',0x423c,2),('origin_y',0x423e,2)]:write(b,a,getattr(c,name),n)
 write(b,0x4d84,q.stamp_bytes,2)
 for j in range(q.stamp_bytes//8):
  st=q.stamps[j];source=next(src for src,a in tee_assets if C.cast(a.data,P).value==C.cast(st.asset.data,P).value)
  write(b,0x4d86+8*j,source,4);write(b,0x4d8a+8*j,0x9000+st.offset,2);write(b,0x4d8c+8*j,st.tile,2)
 addresses={addr(v):0x7000+64*i for i,v in enumerate(nodes)}
 def old(v):return addresses[v] if v else 0xffffffff
 for j,v in enumerate(nodes):
  a=0x7000+64*j
  for off,value,size in ((0,reverse[v.callback],4),(4,old(v.previous),4),(8,old(v.next),4),(12,old(v.owner),4),(16,v.priority,1),(17,v.result,1),(18,v.flags,1),(19,v.substate,1),(20,v.delay,2)):write(b,a+off,value,size)
 write(b,0x5706,old(tc.free_head),4)

def seed(stamps=0,occupied=0,descendants=False,reserved=False,free=False):
 global q,d,p,objects,x,maps,tiles,words,vp,sx,sy,flags,ic,c,tc,t,owner,end,child,nodes,s,handicap,b,saved,handicaps
 setup();put(0x4000,R.randbytes(16384));q=Screen();d=Display.from_buffer_copy(R.randbytes(C.sizeof(Display)));p=Palette();objects=(Cursor*32).from_buffer_copy(R.randbytes(C.sizeof(Cursor)*32));x=(U16*32)(*(R.randrange(65536) for i in range(32)))
 for i,o in enumerate(objects):o.id=d.object_ids[i]=i;o.tag=d.object_tags[i]
 maps=blob(R.randbytes(4096));tiles=blob(R.randbytes(8192));words=(U16*2)(R.randrange(65536),R.randrange(65536));vp=blob(bytes((7,9,0,152)));sx=blob(R.randbytes(2));sy=blob(R.randbytes(2));flags=U8(R.randrange(256))
 nodes=[Task() for _ in range(6)];t,owner,end,child,spare,fallback=nodes
 for node in nodes:node.callback=fn(0x20ca45);node.owner=addr(owner)
 t.previous=addr(owner);t.next=addr(child) if descendants else addr(end);t.priority=2;t.flags=int(reserved);t.delay=R.randrange(65536)
 owner.next=addr(t);owner.previous=addr(end);end.next=addr(owner);end.previous=addr(child) if descendants else addr(t)
 child.previous=addr(t);child.next=addr(end);child.priority=3;child.flags=int(reserved);fallback.flags=1
 tc=TaskContext(addr(spare) if free else None,addr(t));ic=Context(addr(q),addr(d),addr(p),addr(maps),addr(sx),addr(sy),addr(flags),addr(vp),addr(words))
 saved=blob(bytes([5]*54));handicaps=blob(bytes(18))
 c=TeeContext();c.intro=addr(ic);c.tasks=addr(tc);c.fallback=addr(fallback);c.objects=addr(objects);c.x=addr(x);c.tiles=addr(tiles)
 c.par=4;c.distance=512;c.pin_x=123;c.pin_y=456;c.origin_x=99;c.origin_y=440;c.text_base=R.randrange(65536)
 c.saved_hole_scores=addr(saved);c.handicaps=addr(handicaps);c.prepare=C.cast(prepare,P).value;c.sound_request=C.cast(sound,P).value
 for i in range(8):p.slots[i]=(0x400 if i<occupied else 0xff00)+4*i
 p.status=R.randrange(65536);q.stamp_bytes=8*stamps
 for i in range(32):q.stamps[i]=Stamp(tee_resources.header,0x800,0x50)
 s=Tee(*(fn(n) for n in (0x20ce7c,0x20cb83,0x20cd28,0x20cd5e,0x20cd81,0x20cf21,0x20cdac)),0,0,0,0)
 handicap=Handicap(fn(0x20cfcc),fn(0x20cfdd),0,0)
 b=bytearray(ram[:16384]);serial(b);put(0x4000,b);put(0x9000,maps);put(0xa000,tiles);put(0x8002,vp)
 for a,v in zip((0x8032,0x8033,0x8034,0x8035),(sx[0],sy[0],sx[1],sy[1])):store(a,v,1)
 for a,v in ((0x40fa,16),(0x41e9,0),(0x41ec,0),(0x41ed,0),(0x4148,0),(0x40c0,0),(0x4203,0),(0x40f9,0)):store(a,v,1)
 for course in range(3):put(0x586b+66*course,saved[18*course:18*(course+1)])
 put(0x41ee,handicaps);b=bytearray(ram[:16384]);sounds.clear();prepared.clear();lookups.clear()

def check(label):
 serial(b);compare_ram(b,label)
 assert bytes(maps)==data(0x9000,4096),(label,'maps')
 assert bytes(tiles)==data(0xa000,8192),(label,'tiles')
 assert bytes(vp)==data(0x8002,4),(label,'viewport')
 assert list((sx[0],sy[0],sx[1],sy[1]))==[load(a,1) for a in (0x8032,0x8033,0x8034,0x8035)],(label,'scroll')

initial=0
for occupied in range(9):
 for stamps in range(33):
  seed(stamps,occupied,free=bool(stamps%2));assert lib.ntm_tee_setup_initialize(C.byref(s),C.byref(t),C.byref(c),C.byref(tee_resources))==0
  for j in range(min(2,8-occupied)):write_job(b,occupied+j,tee_resources.palettes[j],tee_jobmeta[j])
  calls=[];run(0x20ca45,services=calls);check(('init',occupied,stamps));assert sounds==[(1,65535,8),(1,65534,8)] and prepared==[1]
  assert [pc for pc,a,bc in calls]==[0x250103,0x250103,0x201a82,0x2004e2,0x2037a8,0x2037a8,0x204a77,0x200513,0x200513,0x205119,0x200513,0x2012e7],calls
  assert lib.ntm_course_screen_stamp_queue(C.byref(q),tiles,maps)==0;run(0x20054f);check(('head',occupied,stamps));initial+=1

setups=0
for course in range(3):
 for hole in range(18):
  for badge_kind in range(3):
   seed(stamps=(0,30,32)[badge_kind]);c.course=course;c.round_index=hole;c.hole_index=hole
   # Exact original par is looked up by2128c6 independently of mutable4215.
   base=0x21bb4d;group=base+load(base+2*course,2);desc=group+load(group+2*hole,2);c.par=load(desc+1,1)
   saved[18*course+hole]=(1,c.par,c.par-1)[badge_kind];store(0x586b+66*course+hole,saved[18*course+hole],1)
   s.phase=1;t.callback=fn(0x20cb83);serial(b);write(b,0x586b+66*course+hole,saved[18*course+hole]);put(0x4000,b);b=bytearray(ram[:16384])
   assert lib.ntm_tee_setup_tick(C.byref(s),C.byref(t),C.byref(c),C.byref(tee_resources))==0
   write(b,0x7016,s.asset_group,2);write(b,0x7018,s.asset_index,2);run(0x20cb83);check(('setup',course,hole,badge_kind))
   assert lib.ntm_course_screen_stamp_queue(C.byref(q),tiles,maps)==0;run(0x20054f);check(('setup head',course,hole,badge_kind));setups+=1

later=0
for phase,pc in ((2,0x20cd28),(3,0x20cd5e),(4,0x20cd81),(5,0x20cdac)):
 for delay in (0,1,3,179,32767,32768,32769,65535):
  for skip in (0,16):
   for descendants in (False,True):
    for reserved in (False,True):
     seed(descendants=descendants,reserved=reserved,free=True);s.phase=phase;t.callback=fn(pc);t.delay=delay;c.control=skip
     serial(b);put(0x4000,b);store(0x40c0,skip,1);b=bytearray(ram[:16384])
     assert lib.ntm_tee_setup_tick(C.byref(s),C.byref(t),C.byref(c),C.byref(tee_resources))>=0
     run(pc);check(('later',phase,delay,skip,descendants,reserved));later+=1

notices=0
for reserved in (False,True):
 for descendants in (False,True):
  seed(descendants=descendants,reserved=reserved);t.callback=fn(0x20ce7c);serial(b);put(0x4000,b);b=bytearray(ram[:16384])
  assert lib.ntm_tee_notice(C.byref(t),C.byref(c))==0;run(0x20ce7c);check(('notice',reserved,descendants));notices+=1

handicap_cases=0
for raw in range(256):
 for language in (0,1):
  seed();c.language=language;handicaps[0]=raw;store(0x41ee,raw,1);store(0x40f9,language,1)
  # Registered marker, noncollision and coherent removal list.
  for j,o in enumerate(objects):o.tag=d.object_tags[j]=31 if j==2 else 65535
  objects[2].byte30=0;d.zero_flags[8]=1;d.zero_words[0]=2;d.list[0]=2;t.callback=fn(0x20cf21)
  serial(b);write(b,0x41ee,raw);write(b,0x40f9,language);put(0x4000,b);b=bytearray(ram[:16384])
  assert lib.ntm_tee_handicap_initialize(C.byref(handicap),C.byref(t),C.byref(c),C.byref(tee_resources))>=0
  run(0x20cf21);check(('handicap',raw,language));handicap_cases+=1
for delay in (0,1,89,32767,32768,32769,65535):
 for descendants in (False,True):
  for reserved in (False,True):
   seed(descendants=descendants,reserved=reserved);handicap.phase=1;t.callback=fn(0x20cfcc);t.delay=delay
   serial(b);put(0x4000,b);b=bytearray(ram[:16384])
   assert lib.ntm_tee_handicap_tick(C.byref(handicap),C.byref(t),C.byref(c))>=0
   run(0x20cfcc);check(('handicap wait',delay,descendants,reserved));handicap_cases+=1

owners=0
for skip in range(256):
 for music_result in (0,2):
  seed(free=True);t.callback=fn(0x208753);ownerstate=TeeOwner(fn(0x20ca45),fn(0x20876f),0,skip,0xa5,2,music_result,C.cast(sound,P).value,None)
  serial(b);put(0x4000,b);store(0x453a,skip,1);store(0x41db,0xa5,1);store(0x4204,2,1);store(0x6ad8,music_result,1);c.course=2;b=bytearray(ram[:16384])
  result=lib.ntm_tee_owner_tick(C.byref(ownerstate),C.byref(t),C.byref(tc),C.byref(nodes[-1]));assert result==bool(skip)
  run(0x208753,stop=(0x208785,));write(b,0x453a,ownerstate.skip_overview);write(b,0x41db,ownerstate.player_status);check(('owner',skip,music_result));owners+=1
owner_waits=0
for descendant in (False,True):
 for current in range(3):
  for course in range(3):
   seed(descendants=descendant);t.callback=fn(0x20876f)
   o=TeeOwner(fn(0x20ca45),fn(0x20876f),1,99,0xa5,course,current,C.cast(sound,P).value,None)
   c.course=course;serial(b);write(b,0x453a,99);write(b,0x41db,0xa5);write(b,0x6ad8,current);put(0x4000,b)
   assert lib.ntm_tee_owner_tick(C.byref(o),C.byref(t),C.byref(tc),C.byref(nodes[-1]))==int(not descendant)
   calls=[];run(0x20876f,stop=(0x208785,),services=calls)
   write(b,0x453a,o.skip_overview);write(b,0x41db,o.player_status);check(('owner wait',descendant,current,course))
   expected=[] if descendant or current==course else [(0,65535,course),(0,65534,course)]
   assert sounds==expected
   assert [(int(pc==0x250103),a,bc) for pc,a,bc in calls if pc in (0x250103,0x2500ea)]==expected
   owner_waits+=1
numbers=0
lib.ntm_tee_draw_number.argtypes=[U16,U8,U8,U16,U16,C.POINTER(U32),P]
for value in (0,1,9,10,99,100,101,999,1000,9999,10000,32767,32768,65535):
 for digits in (1,2,3,4):
  for color in (0,127,255):
   for base in (0,511,65535):
    seed();scratch=U32(d.reset_words[0]);c.text_base=base;offset=4096-2*digits
    assert lib.ntm_tee_draw_number(value,digits,color,base,offset,C.byref(scratch),maps)==0
    store(0x400f,base,2);reg(0,value);reg(1,digits);reg(2,(color<<8)|0xab);reg(4,0x9000+offset)
    run(0x205119);d.reset_words[0]=scratch.value;write(b,0x400f,base,2);check(('number',value,digits,color,base));numbers+=1
badges=0
for course,hole in ((0,0),(0,2),(0,3)):
 base=0x21bb4d;group=base+load(base+2*course,2);desc=group+load(group+2*hole,2);par=load(desc+1,1)
 for score in range(256):
  setup();store(0x4204,course,1);store(0x4205,hole,1);store(0x586b+66*course+hole,score,1)
  run(0x2128c6);assert lib.ntm_tee_badge_offset(score,par)==(get(0)&255),(score,par,get(0)&255);badges+=1
print(json.dumps({'initializations':initial,'setups':setups,'later_callbacks':later,'normal_notices':notices,'handicaps':handicap_cases,'owner_entries':owners,'owner_waits':owner_waits,'number_cases':numbers,'badge_cases':badges,'scope':'full modeled nonstack RAM, exact requests, queues, tiles/maps, viewport, objects, real allocation/removal; isolated prepare service boundary, not whole-game or audio synthesis'}))
