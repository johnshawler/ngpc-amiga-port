#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Private original comparison; no cartridge bytes or captured assets embedded.
from cursor_native_case_common import *
import course_init_case_common as I
app=C.CDLL(str(ROOT/'build/cursor-check/native_application_final.so'));app.bridge_start.argtypes=[C.c_void_p]*17+[C.c_uint];app.bridge_pointer.argtypes=[C.c_uint];app.bridge_pointer.restype=C.c_void_p;app.bridge_export.argtypes=[C.POINTER(U32)]
fi=buf(data(0x219859-128,256));fo=buf(data(0x219867-128,256));rngtable=buf(data(load(0x260010,4)-128,256));values=(U32*68)()
completion=I.Job(2,load(0x21a24e+5,1),480+load(0x21a24e,1),0,load(0x21a24e+1,2),load(0x21a24e+3,2),None,None,0,0)
palettejobs=(I.Job*3)(I.group,I.single,completion)
def obj(n,t):return C.cast(app.bridge_pointer(n),C.POINTER(t)).contents

def compare_cursor(label,head):
 m=object.__new__(Native);m.b=bytearray(ram[:16384]);m.d=obj(2,Display);m.e=obj(9,Engine)
 ni=obj(0,I.Init);m.c=C.cast(app.bridge_pointer(0)+I.Init.cursor.offset,C.POINTER(Cursor)).contents;m.global_=U8(ni.flags)
 m.ts=obj(5,U8*8192);m.ss=obj(7,U8*256);m.ps=obj(8,U8*64)
 m.serial(head=head)
 for k in range(m.d.zero_flags[10]):m.serial(newjob=k)
 m.compare(label)
count=updates=0
for case in range(96):
 setup();initial=I.Init();s=I.Screen();p=I.Palette();disp=I.Display();controls=buf(bytes([0,0,0,0,0,case%256,32,0]+[0]*8));rng=buf(bytes([case*7%256]));tiles=buf(R.randbytes(8192));maps=buf(R.randbytes(4096));sprites=buf(R.randbytes(256));spal=buf(R.randbytes(64));sel=case%3
 for i in range(8):s.fill_status[i]=255;p.slots[i]=0xff00+4*i;store(0x5782+i,255,1);store(0x66bf+2*i,p.slots[i],2)
 s.displayed_selection=sel;store(0x447d,sel,1);store(0x40fa,16,1);store(0x87e2,0,1);p.pending[0]=p.pending[1]=255;store(0x69f1,255,1);store(0x69f2,255,1)
 for a,v in ((0x8118,0x81),(0x8012,7)):store(a,v,1)
 p.background=0x81;p.window_control=7
 for i in range(512):store(0x8200+i,0,1)
 for a,blob in ((0xa000,tiles),(0x9000,maps),(0x8800,sprites),(0x8c00,spal)):
  put(a,blob)
 reg(6,0x7040);execute(0x209215);execute(0x202c77);execute(0x201192);execute(0x200c7b);store(0x400d,load(0x400d,1)|192,1)
 assert app.bridge_start(C.byref(initial),C.byref(s),C.byref(disp),C.byref(p),I.assets,palettejobs,I.descriptor,fi,fo,rngtable,rng,C.byref(assets),tiles,maps,sprites,spal,controls,0)==0
 s=obj(1,I.Screen);p=obj(3,I.Palette);disp=obj(2,I.Display);ni=obj(0,I.Init)
 controls=obj(4,U8*16);tiles=obj(5,U8*8192);maps=obj(6,U8*4096)
 compare_cursor((case,'start'),False);I.compare(p)
 app.bridge_export(values)
 for i in range(5):
  a=0x7000+64*i;v=values[i*9:i*9+9]
  for o,x,n in zip((0,4,8,12,16,17,18,19,20),v,(4,4,4,4,1,1,1,1,2)):store(a+o,x,n)
 store(0x70c8,0xffffffff,4);store(0x5706,values[45],4);store(0x40fb,sel,2);store(0x4148,0,1);store(0x400e,rng[0],1);put(0x40bf,controls)
 for i,x in enumerate((80,0,0,152)):store(0x8002+i,x,1)
 sounds=[0,0,0];tick=0
 while True:
  if tick<8:buttons=8 if tick==7 and case&1 else 0
  elif tick<28:buttons=(8,0,4,0,8,0,0,0,4,0)[(tick-8)%10]
  elif tick==28:buttons=(16,32,80)[case%3]
  else:buttons=0
  store(0x6f82,buttons,1);execute(0x2002c2);execute(0x202bce);execute(0x201c25);execute(0x20054f)
  if load(0x400d,1)&80==80:execute(0x20067c);execute(0x200aa6)
  store(0x400d,load(0x400d,1)&63,1)
  reg(6,load(0x7008,4));reg(7,0x6be0);reg(8,0x20137c)
  for k in range(100000):
   pc=get(8)
   if pc==0x20137c and get(6)==0x7000:break
   if pc==0x2501ba:
    sounds[get(0)&255]+=1;sp=get(7);reg(8,load(sp,4));reg(7,sp+4);continue
   step(0)
  else:raise RuntimeError(('dispatchlimit',case,tick,hex(get(8))))
  execute(0x202c77)
  if load(0x400d,1)&144==16:execute(0x201192);execute(0x200c7b);store(0x400d,load(0x400d,1)|192,1)
  result=app.bridge_update(buttons);assert result in (0,1),(case,tick,result);app.bridge_export(values)
  compare_cursor((case,tick),True)
  assert bytes(controls)==data(0x40bf,16)
  assert values[48]==load(0x40fb,1);assert list(values[49:53])==[load(0x4545,2),load(0x454b,1),load(0x4113,1),load(0x4204,1)]
  assert list(values[53:57])==[load(0x8002+i,1) for i in range(4)];assert list(values[57:61])==[load(0x420b+i,1) for i in range(4)];assert list(values[61:64])==sounds
  assert values[45]==load(0x5706,4);assert values[47]==load(0x570a,4)
  free=load(0x5706,4);frees=set()
  while free!=0xffffffff:frees.add(free);free=load(free+8,4)
  for i in range(5):
   a=0x7000+64*i
   for j,(o,n) in enumerate(zip((0,4,8,12,16,17,18,19,20),(4,4,4,4,1,1,1,1,2))):
    expected=values[i*9+j]
    if j==2 and a in frees and expected==0:expected=0xffffffff
    assert expected==load(a+o,n),(case,tick,hex(a+o),hex(expected),hex(load(a+o,n)))
  I.compare(p);assert bytes(maps)==data(0x9000,4096);assert bytes(tiles)==data(0xa000,8192)
  assert s.cached_selection==load(0x40fd,2) and s.cursor_x==load(0x453f,2) and s.displayed_selection==load(0x447d,1)
  assert s.stamp_bytes==load(0x4d84,2);assert bytes(s.fill_status)==data(0x5782,8)
  updates+=1;tick+=1
  if result:break
  assert tick<150
 count+=1
 if count%12==0:print("FLOW_PROGRESS",count,updates,file=sys.stderr,flush=True)
print(json.dumps({'flows':count,'updates':updates,'scope':'actual full native application vs original bounded scheduler: cursor,taskring,input,tiles,maps,sprites,palettes,queues,viewport,sounds as events; actual live initial controls'}))
