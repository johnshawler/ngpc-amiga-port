#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original task/map/object initialization comparison.

Only audio and the animated-palette enqueue requests are captured at their
call boundaries here. Raster registration runs its real original/native helper.
This is not an integrated animated-palette or raster display proof.
"""
from shot_banner_case_common import *
lib=C.CDLL(os.environ.get('NTM_SHOT_BANNER_LIBRARY',str(ROOT/'build/shot-check/native-banner.so')))
for name,typ in [('initialize',Banner),('tick',Banner),('animation_initialize',BannerAnimation),('animation_tick',BannerAnimation),('exit_initialize',BannerExit)]:
 getattr(lib,'ntm_shot_banner_'+name).argtypes=[C.POINTER(typ),C.POINTER(Task),C.POINTER(BannerContext),C.POINTER(BannerResources)]
lib.ntm_shot_banner_exit_tick.argtypes=[C.POINTER(BannerExit),C.POINTER(Task),C.POINTER(BannerContext)]
lib.ntm_shot_banner_notice.argtypes=[C.POINTER(Task),C.POINTER(BannerContext)]
pcs=(0x20bf8e,0x20c0f7,0x20c108,0x20c13e,0x20c158,0x20c199,0x20c3eb,0x20c45b,0x20c503,0x20c564,0x20c5b9,0x209749)
for pc in pcs:
 cb=CALL(lambda c,t:0);callbacks[pc]=cb;reverse[C.cast(cb,P).value]=pc
SOUND=C.CFUNCTYPE(None,P,U8,U16,U16);PALETTE=C.CFUNCTYPE(C.c_int,P,U16)
sounds=[];palette_requests=[]
@SOUND
def sound(p,service,command,value):sounds.append((service,command))
@PALETTE
def palette_request(p,command):palette_requests.append(command);return 0
def ret():
 sp=get(7);reg(8,load(sp,4));reg(7,sp+4)
def run(pc):
 reg(6,0x7000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);calls=[]
 for _ in range(200000):
  pc=get(8)
  if pc==0xef00:return calls
  if pc==0x2500ea or (pc==0x2037a8 and (get(0)&65535) in (0x6c,0x70)):
   calls.append((pc,get(0)&65535,get(4)))
   ret();continue
  step(0)
 raise AssertionError(('budget',hex(get(8))))
def serial(b):
 serial_display(d,b);serial_palette(p,b)
 for j,o in enumerate(objects):
  for name,off in [('tag',2),('y',6),('width',8),('velocity',10),('tile',18),('link',28)]:write(b,0x453b+64*j+off,getattr(o,name),2)
  for name,off in [('id',0),('second',1),('byte14',14),('byte15',15),('flags',16),('byte30',30)]:write(b,0x453b+64*j+off,getattr(o,name))
  write(b,0x453f+64*j,x[j],2)
 write(b,0x400d,flags.value);write(b,0x4d80,words[0],2);write(b,0x4d82,words[1],2)
 write(b,0x4443,raster.upper_scroll);write(b,0x4441,raster.lower_scroll);write(b,0x6fd4,0x20c620 if raster.callback else 0x200471,4);write(b,0x6c27,raster.interrupt_shadow)
 addresses={addr(v):0x7000+64*i for i,v in enumerate(nodes)}
 def old(v):return addresses[v] if v else 0xffffffff
 for j,v in enumerate(nodes):
  a=0x7000+64*j
  for off,value,size in ((0,reverse[v.callback],4),(4,old(v.previous),4),(8,old(v.next),4),(12,old(v.owner),4),(16,v.priority,1),(17,v.result,1),(18,v.flags,1),(19,v.substate,1),(20,v.delay,2)):write(b,a+off,value,size)
 write(b,0x5706,old(tc.free_head),4)
 write(b,0x4d84,q.stamp_bytes,2)
 for j in range(q.stamp_bytes//8):
  st=q.stamps[j];source=next(src for src,a in tee_assets if C.cast(a.data,P).value==C.cast(st.asset.data,P).value)
  write(b,0x4d86+8*j,source,4);write(b,0x4d8a+8*j,0x9000+st.offset,2);write(b,0x4d8c+8*j,st.tile,2)
def seed(occupied=0,descendants=False,reserved=False):
 global q,d,p,objects,x,maps,tiles,words,vp,sx,sy,flags,ic,c,bc,h,player,tc,t,owner,end,child,nodes,s,anim,ex,b,raster
 setup();put(0x4000,R.randbytes(16384));q=Screen();d=Display.from_buffer_copy(R.randbytes(C.sizeof(Display)));p=Palette();objects=(Cursor*32).from_buffer_copy(R.randbytes(C.sizeof(Cursor)*32));x=(U16*32)(*(R.randrange(65536) for _ in range(32)))
 for j,o in enumerate(objects):o.id=d.object_ids[j]=j;o.tag=d.object_tags[j]
 maps=blob(R.randbytes(4096));tiles=blob(R.randbytes(8192));words=(U16*2)(R.randrange(65536),R.randrange(65536));vp=blob(bytes((7,9,0,152)));sx=blob(R.randbytes(2));sy=blob(R.randbytes(2));flags=U8(R.randrange(256));raster=Raster(R.randrange(256),R.randrange(256),0,0,1,0,0,0,R.randrange(256))
 nodes=[Task() for _ in range(8)];t,owner,end,child,spare,spare2,spare3,fallback=nodes
 for node in nodes:node.callback=fn(0x20bf8e);node.owner=addr(owner)
 t.previous=addr(owner);t.next=addr(child) if descendants else addr(end);t.priority=2;t.flags=int(reserved);t.delay=R.randrange(65536)
 owner.next=addr(t);owner.previous=addr(end);end.next=addr(owner);end.previous=addr(child) if descendants else addr(t)
 child.previous=addr(t);child.next=addr(end);child.priority=3;child.flags=int(reserved);fallback.flags=1
 spare.next=addr(spare2);spare2.next=addr(spare3)
 tc=TaskContext(addr(spare),addr(t));ic=Context(addr(q),addr(d),addr(p),addr(maps),addr(sx),addr(sy),addr(flags),addr(vp),addr(words));c=TeeContext()
 c.intro=addr(ic);c.tasks=addr(tc);c.fallback=addr(fallback);c.objects=addr(objects);c.x=addr(x);c.tiles=addr(tiles);c.language=1;c.sound_request=C.cast(sound,P).value
 h=H.read_state();h.hole[0x70]=4;h.players[0][0]=0;h.players[0][1]=0;h.players[0][2]=1;h.players[0][3]=0;player=Player(0,0,1,1)
 bc=BannerContext(addr(c),addr(h),addr(player),0,0,C.cast(palette_request,P).value,None,addr(raster))
 for j in range(8):p.slots[j]=(0x400 if j<occupied else 0xff00)+4*j
 p.status=R.randrange(65536)
 s=Banner(*(fn(n) for n in (0x20c0f7,0x20c13e,0x20c158,0x20c199,0x20c3eb,0x20c45b,0x20c564)),0,0,0,0)
 anim=BannerAnimation(fn(0x20c503),0,0,0);ex=BannerExit(fn(0x20c5b9),0)
 b=bytearray(ram[:16384]);serial(b);put(0x4000,b);put(0x9000,maps);put(0xa000,tiles);put(0x8002,vp)
 for a,v in zip((0x8032,0x8033,0x8034,0x8035),(sx[0],sy[0],sx[1],sy[1])):store(a,v,1)
 for a,v in ((0x40fa,16),(0x41e9,0),(0x41ec,0),(0x41ed,0),(0x4148,0),(0x40c0,0),(0x40f9,1),(0x447e,0),(0x4144,0)):store(a,v,1)
 store(0x41c9,0x4149,4);put(0x4149,h.players[0]);store(0x4270,4,1);
 for a,v in ((0x20,raster.timer_run),(0x24,raster.timer_mode),(0x22,raster.timer_divisor),(0x8000,raster.gpu_control)):store(a,v,1)
 b=bytearray(ram[:16384]);sounds.clear();palette_requests.clear()
def check(label):
 serial(b);compare_ram(b,label)
 assert bytes(maps)==data(0x9000,4096),(label,'maps',[(hex(i),a,v) for i,(a,v) in enumerate(zip(maps,data(0x9000,4096))) if a!=v][:12])
 assert bytes(tiles)==data(0xa000,8192),(label,'tiles')
 assert bytes(vp)==data(0x8002,4),(label,'viewport')
 assert (raster.timer_run,raster.gpu_control)==tuple(load(a,1) for a in (0x20,0x8000)),(label,'raster control')
 assert list((sx[0],sy[0],sx[1],sy[1]))==[load(a,1) for a in (0x8032,0x8033,0x8034,0x8035)],(label,'scroll')
def scratch():write(b,0x7016,s.blink_delay);write(b,0x7017,s.blink_state)
initial=0
for occupied in range(9):
 for char in range(6):
  for playerid in range(2):
   seed(occupied);h.players[0][0]=playerid;h.players[0][1]=char;put(0x4149,h.players[0]);b=bytearray(ram[:16384])
   assert lib.ntm_shot_banner_initialize(C.byref(s),C.byref(t),C.byref(bc),C.byref(banner_resources))==0;scratch()
   for j in range(min(2,8-occupied)):write_job(b,occupied+j,banner_resources.palettes[j],banner_jobmeta[j])
   calls=run(0x20bf8e);check(('init',occupied,char,playerid));assert sounds==[(0,65534)] and calls[0][:2]==(0x2500ea,65534);initial+=1
   lib.ntm_course_screen_stamp_queue.argtypes=[C.POINTER(Screen),P,P]
   assert lib.ntm_course_screen_stamp_queue(C.byref(q),tiles,maps)==0;run(0x20054f);check(('init head',occupied,char,playerid))
later=0
for phase,pc in ((1,0x20c0f7),(2,0x20c13e),(3,0x20c158),(4,0x20c199)):
 for delay in (0,1,20,149,32767,32768,65535):
  for result in (0,255):
   for control in (0,16):
    seed(descendants=phase==4);s.phase=phase;t.callback=fn(pc);t.delay=delay;t.result=result;c.control=control;s.blink_delay=1;s.blink_state=0
    serial(b);scratch();put(0x4000,b);store(0x40c0,control,1);b=bytearray(ram[:16384])
    assert lib.ntm_shot_banner_tick(C.byref(s),C.byref(t),C.byref(bc),C.byref(banner_resources))>=0;scratch()
    calls=run(pc);check(('later',phase,delay,result,control));assert not palette_requests;later+=1
animation=0
for inactive in (0,1):
 seed();bc.controlling_player=inactive;store(0x4144,inactive,1)
 # Coherent empty display, as guaranteed by parent initializer.
 lib.ntm_tee_objects_reset.argtypes=[C.POINTER(Display),C.POINTER(Cursor)]
 assert lib.ntm_tee_objects_reset(C.byref(d),objects)==0;serial(b);write(b,0x4144,inactive);put(0x4000,b);b=bytearray(ram[:16384]);t.callback=fn(0x20c45b);serial(b);put(0x4000,b)
 assert lib.ntm_shot_banner_animation_initialize(C.byref(anim),C.byref(t),C.byref(bc),C.byref(banner_resources))==0
 write(b,0x7016,anim.variant,2);write(b,0x7018,anim.inactive);run(0x20c45b);check(('animation init',inactive));animation+=1
 for completed in (0,64):
  objects[11].byte30=completed;store(0x4819,completed,1);b=bytearray(ram[:16384])
  assert lib.ntm_shot_banner_animation_tick(C.byref(anim),C.byref(t),C.byref(bc),C.byref(banner_resources))>=0;run(0x20c503);check(('animation tick',inactive,completed));animation+=1
exit_cases=0
for occupied in range(9):
 seed(occupied);t.callback=fn(0x20c564);serial(b);put(0x4000,b);b=bytearray(ram[:16384])
 assert lib.ntm_shot_banner_exit_initialize(C.byref(ex),C.byref(t),C.byref(bc),C.byref(banner_resources))==0
 calls=run(0x20c564);check(('exit init',occupied));assert palette_requests==[0x6c,0x70] and [v[:2] for v in calls]==[(0x2037a8,0x6c),(0x2037a8,0x70)];exit_cases+=1
 for tick in range(20):
  assert lib.ntm_shot_banner_exit_tick(C.byref(ex),C.byref(t),C.byref(bc))>=0;run(0x20c5b9);check(('exit tick',occupied,tick));exit_cases+=1
notices=0
for reserved in (False,True):
 seed(reserved=reserved);t.callback=fn(0x20c3eb);serial(b);put(0x4000,b)
 assert lib.ntm_shot_banner_notice(C.byref(t),C.byref(bc))==0;run(0x20c3eb);check(('notice',reserved));notices+=1
print(json.dumps(dict(initializations=initial,later_callbacks=later,animation_callbacks=animation,exit_callbacks=exit_cases,notices=notices,scope='Full modeled nonstack RAM/maps/tiles/objects/tasks and real raster install/remove; animated palette enqueue/audio captured at explicit service boundary')))
