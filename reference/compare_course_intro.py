#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Private original/native intro oracle. No cartridge-derived assets embedded.
from course_intro_case_common import *
record_cases=0
for bank in (0,1,255):
 for index in range(6):
  for count in (0,1,32767,32768,65534,65535):
   setup();put(0x4000,R.randbytes(16384));r=create_records(bank,index,count);b=bytearray(ram[:16384]);assert lib.ntm_course_records_prepare(C.byref(r))==0
   serial_records(r,b);execute(0x212d64);compare_ram(b,('records',bank,index,count));record_cases+=1
# All result bytes; allocator effects include empty/nonempty pool and stale node fields.
owner_cases=0
for result in range(256):
 for pool in (False,True):
  setup();put(0x4000,R.randbytes(16384));r=create_records(result&1,result%6,65535);nodes=(Task*4)();c=TaskContext();s=Owner(fn(0x20771d),fn(0x203f59),0)
  def na(i):return addr(nodes[i])
  def oa(p):return 0 if not p else 0x7000+64*((p-addr(nodes))//C.sizeof(Task))
  for i in range(4):
   t=nodes[i];t.callback=fn(0x203f31);t.previous=na(0);t.next=na(0);t.owner=na(0);t.priority=i;t.result=result;t.flags=0xa4;t.substate=37;t.delay=1234
  nodes[0].next=na(1);nodes[0].previous=na(1);nodes[1].result=result;nodes[2].next=None;c.current=na(1);c.free_head=na(2) if pool else None
  def serial_nodes(b):
   for i,t in enumerate(nodes):
    a=0x7000+64*i
    for off,v,n in ((0,reverse[t.callback],4),(4,oa(t.previous),4),(8,oa(t.next) if t.next else 0xffffffff,4),(12,oa(t.owner),4),(16,t.priority,1),(17,t.result,1),(18,t.flags,1),(19,t.substate,1),(20,t.delay,2)):write(b,a+off,v,n)
   write(b,0x5706,oa(c.free_head) if c.free_head else 0xffffffff,4)
  b=bytearray(ram[:16384]);serial_nodes(b);put(0x4000,b);reg(6,0x7040)
  ret=lib.ntm_course_owner_tick(C.byref(c),C.byref(nodes[1]),C.byref(s),C.byref(r),C.byref(nodes[3]));assert ret==(2 if result not in (0,249) else 0)
  serial_nodes(b);serial_records(r,b);execute(0x203f31,(0x203eee,));compare_ram(b,('owner',result,pool));owner_cases+=1
# Special records must stop before any unconverted helper or normal mutation.
for bank in (0,1):
 setup();put(0x4000,R.randbytes(16384));r=create_records(bank,2,42,True);b=bytes(r);assert lib.ntm_course_records_prepare(C.byref(r))==1 and bytes(r)==b
 assert execute(0x212d64,(0x212a02,))==0x212a02
intro_cases=0
for course in range(3):
 r=resources[course];dp,source,jmeta=metadata[course]
 for occupied in range(9):
  for stamps in range(33):
   setup();put(0x4000,R.randbytes(16384));store(0x40fa,16,1);store(0x4204,course,1);reg(6,0x7000);screen=Screen();d=Display();p=Palette();t=Task();s=Intro(fn(0x2077d3),fn(0x207809),fn(0x20781a),None,0,0)
   maps=blob(R.randbytes(4096));tiles=blob(R.randbytes(8192));sx=blob(R.randbytes(2));sy=blob(R.randbytes(2));flags=U8(load(0x400d,1));viewport=blob(b'\x50\x00\x00\x98');words=(U16*2)(load(0x4d80,2),load(0x4d82,2));ctx=Context(addr(screen),addr(d),addr(p),addr(maps),addr(sx),addr(sy),addr(flags),addr(viewport),addr(words))
   put(0x9000,maps);put(0xa000,tiles);put(0x8032,bytes((sx[0],sy[0],sx[1],sy[1])));put(0x8002,viewport);store(0x87e2,0,1)
   p.background=0x81;p.window_control=7;store(0x8118,p.background,1);store(0x8012,p.window_control,1);p.pending[0]=p.pending[1]=255;put(0x69f1,p.pending)
   p.staging[:]=data(0x67f1,512);p.palette[:]=R.randbytes(512);put(0x8200,p.palette);p.status=load(0x66bd,2);p.copy_bytes=0;store(0x67ef,0,2)
   for i in range(8):p.copies[i]=Copy(load(0x67cf+4*i,2),load(0x67d1+4*i,2))
   perm=list(range(8));R.shuffle(perm);b=bytearray(ram[:16384])
   for i,index in enumerate(perm):
    kind=i%2;p.slots[i]=(0x400 if kind==0 else 0x2000)+4*index if i<occupied else 0xff00+4*index
    if i<occupied:p.jobs[index]=r.palettes[kind];write_job(b,index,r.palettes[kind],jmeta[kind])
   serial_palette(p,b);put(0x4000,b)
   for i in range(8):
    f=Fill(2*R.randrange(32),R.randrange(65536),1+R.randrange(12),1+R.randrange(8));screen.fills[i]=f;screen.fill_status[i]=255 if R.randrange(2) else R.choice((0,1,254));store(0x5782+i,screen.fill_status[i],1);store(0x5742+8*i,f.offset+0x9000,4);store(0x5746+8*i,f.height,1);store(0x5747+8*i,f.width,1);store(0x5748+8*i,f.fill,2)
   screen.stamp_bytes=8*stamps;store(0x4d84,screen.stamp_bytes,2)
   for i in range(32):screen.stamps[i]=Stamp(r.background,0x800,load(dp+2,2));store(0x4d86+8*i,source,4);store(0x4d8a+8*i,0x9800,2);store(0x4d8c+8*i,load(dp+2,2),2)
   b=bytearray(ram[:16384]);assert lib.ntm_course_intro_initialize(C.byref(s),C.byref(t),C.byref(ctx),C.byref(r))==0
   serial_display(d,b);write(b,0x400d,flags.value);write(b,0x4d80,words[0],2);write(b,0x4d82,words[1],2);write(b,0x4d84,screen.stamp_bytes,2)
   serial_palette(p,b)
   for i in range(min(2,8-occupied)):write_job(b,perm[occupied+i],r.palettes[i],jmeta[i])
   write(b,0x7000,reverse[t.callback],4);write(b,0x7014,t.delay,2);write(b,0x7016,s.asset_group,2);write(b,0x7018,dp+4,4)
   services=[];execute(0x20771d,services=services);assert services==[(0x201a82,None),(0x2006cf,None),(0x2037a8,(course+53)*4),(0x2037a8,(course+56)*4),(0x200513,None)]
   compare_ram(b,('initialize',course,occupied,stamps));graphics_match(p,maps,tiles);assert data(0x8032,4)==bytes((sx[0],sy[0],sx[1],sy[1]))
   # Original tail, next-head then OLD fill/stamp queues, before task callback.
   assert lib.ntm_course_palette_tail(C.byref(p))==0;execute(0x202c77);serial_palette(p,b);compare_ram(b,'tail')
   assert lib.ntm_course_palette_head(C.byref(p))==0;execute(0x202bce);serial_palette(p,b);compare_ram(b,'head')
   assert lib.ntm_course_screen_fill_queue(C.byref(screen),maps)==0;execute(0x201c25);b[0x1782:0x178a]=bytes(screen.fill_status);compare_ram(b,'fills')
   assert lib.ntm_course_screen_stamp_queue(C.byref(screen),tiles,maps)==0;execute(0x20054f);write(b,0x4d84,screen.stamp_bytes,2);compare_ram(b,'stamps');graphics_match(p,maps,tiles)
   for n in range(2):
    assert lib.ntm_course_intro_tick(C.byref(s),C.byref(t),C.byref(ctx))==0;reg(6,0x7000);execute(0x2077d3);write(b,0x7000,reverse[t.callback],4);write(b,0x7014,t.delay,2);compare_ram(b,('delay',n));assert bytes(viewport)==data(0x8002,4)
   assert s.phase==2 and t.delay==19 and bytes(viewport)==bytes((0,0,160,152));intro_cases+=1
   if intro_cases%99==0:print('INTRO_PROGRESS',intro_cases,file=sys.stderr,flush=True)
# Every sign/carry edge of both callbacks, with stable explicit title stop.
delay_cases=0
for phase,pc in ((1,0x2077d3),(2,0x207809)):
 for delay in (0,1,2,19,20,32767,32768,32769,65534,65535):
  for flagsvalue in (0,255):
   setup();s.phase=phase;t.callback=fn(pc);t.delay=delay;store(0x7014,delay,2);store(0x7000,pc,4);reg(6,0x7000);put(0x8002,viewport);before=bytearray(ram[:16384]);reg(9,flagsvalue)
   ret=lib.ntm_course_intro_tick(C.byref(s),C.byref(t),C.byref(ctx));execute(pc,(0x20781a,));write(before,0x7000,reverse[t.callback],4);write(before,0x7014,t.delay,2);compare_ram(before,('boundary',phase,delay,flagsvalue));assert bytes(viewport)==data(0x8002,4);assert ret==(1 if s.phase==3 else 0);delay_cases+=1
print(json.dumps({'record_cases':record_cases,'owner_cases':owner_cases,'intro_cases':intro_cases,'delay_cases':delay_cases,'scope':'original/native whole non-stack RAM, ordered services, persistent queues, tile/map/palette memory and first-open viewport; unsupported title/record paths explicit'}))
