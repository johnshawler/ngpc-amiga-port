#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Private original comparison; no cartridge bytes or captured assets embedded.
from cursor_native_case_common import *
import course_init_case_common as I
import course_intro_case_common as J
import os
import cursor_native_case_common as native
import compare_course_title_cursor as title_object
import round_record_case_common as K
app=C.CDLL(os.environ.get('NTM_ROUND_APPLICATION_LIBRARY',str(ROOT/'build/round-record-check/native_round_application_default.so')));app.bridge_start.argtypes=[C.c_void_p]*17+[C.c_uint]+[C.c_void_p]*2;app.bridge_pointer.argtypes=[C.c_uint];app.bridge_pointer.restype=C.c_void_p;app.bridge_export.argtypes=[C.POINTER(U32)]
fi=buf(data(0x219859-128,256));fo=buf(data(0x219867-128,256));rngtable=buf(data(load(0x260010,4)-128,256));values=(U32*74)()
completion=I.Job(2,load(0x21a24e+5,1),480+load(0x21a24e,1),0,load(0x21a24e+1,2),load(0x21a24e+3,2),None,None,0,0)
palettejobs=(I.Job*3)(I.group,I.single,completion)
class TitleResources(C.Structure):_fields_=[('descriptor',C.c_void_p),('descriptor_size',C.c_size_t),('metadata',C.c_void_p),('metadata_size',C.c_size_t),('title',J.Asset),('tournament',J.Asset*2)]
app.bridge_titles.argtypes=[C.c_void_p,C.c_void_p];app.bridge_title_export.argtypes=[C.POINTER(U32)]
title_values=(U32*5)();keep=[];title_list=[];object_list=[];shape_roots=[]
course_resources=(J.Resources*3)(*J.resources)
def stamp_asset(source):
 n=load(source,2);end=source+6+2*n;rows=load(end,1)&127;end+=1
 for _ in range(rows):
  while load(end,2)!=65535:end+=2
  end+=2
 size=max(load(source+6+2*i,2) for i in range(n))+16
 ab=buf(data(source,end-source));tb=buf(data(load(source+2,4),size));keep.extend((ab,tb));return J.Asset(ab,len(ab),tb,len(tb))
for course in range(3):
 dp=J.metadata[course][0];db=buf(data(dp,12));md=buf(data(0x21980e,5));keep.extend((db,md))
 group=load(load(0x260020,4)+load(dp,2),4)&0xffffff
 title_list.append(TitleResources(C.addressof(db),12,C.addressof(md),5,stamp_asset(load(group+12,4)),(J.Asset*2)(stamp_asset(load(group+8,4)),stamp_asset(load(group+16,4)))))
 course_resources[course].descriptor=C.addressof(db);course_resources[course].descriptor_size=12
 oa,sp,retained,_=title_object.title_assets(course);object_list.append(oa);shape_roots.append(sp);keep.extend(retained)
title_resources=(TitleResources*3)(*title_list);object_resources=(Assets*3)(*object_list)
app.bridge_titles(title_resources,object_resources)
app.bridge_round.argtypes=[C.c_void_p];app.bridge_round(C.byref(K.round_resources));app.bridge_round_export.argtypes=[C.POINTER(U32)];round_values=(U32*6)();app.bridge_round_sounds.argtypes=[C.POINTER(U32)];sound_values=(U32*4)()
selector_shapes=native.shapes
def obj(n,t):return C.cast(app.bridge_pointer(n),C.POINTER(t)).contents

def compare_cursor(label,head):
 m=object.__new__(Native);m.b=bytearray(ram[:16384]);m.d=obj(2,Display);m.e=obj(9,Engine)
 ni=obj(0,I.Init);m.c=obj(20,Cursor) if round_values[0] else obj(15,Cursor) if title_values[0] else C.cast(app.bridge_pointer(0)+I.Init.cursor.offset,C.POINTER(Cursor)).contents;m.global_=U8(ni.flags)
 native.shapes=K.round_shapes if round_values[0] else shape_roots[values[52]] if title_values[0] else selector_shapes
 m.ts=obj(5,U8*8192);m.ss=obj(7,U8*256);m.ps=obj(8,U8*64)
 m.serial(head=head)
 for k in range(m.d.zero_flags[10]):m.serial(newjob=k)
 m.compare(label)
count=updates=0;courses_seen=set();boundaries={}
for case in range(24):
 setup();initial=I.Init();s=I.Screen();p=I.Palette();disp=I.Display();controls=buf(bytes([0,0,0,0,0,case%256,32,0]+[0]*8));rng=buf(bytes([case*7%256]));tiles=buf(R.randbytes(8192));maps=buf(R.randbytes(4096));sprites=buf(R.randbytes(256));spal=buf(R.randbytes(64));sel=case%3
 for i in range(8):s.fill_status[i]=255;p.slots[i]=0xff00+4*i;store(0x5782+i,255,1);store(0x66bf+2*i,p.slots[i],2)
 s.displayed_selection=sel;store(0x447d,sel,1);store(0x40fa,16,1);store(0x87e2,0,1);p.pending[0]=p.pending[1]=255;store(0x69f1,255,1);store(0x69f2,255,1)
 for a,v in ((0x8118,0x81),(0x8012,7)):store(a,v,1)
 p.background=0x81;p.window_control=7
 for i in range(512):store(0x8200+i,0,1)
 for a,blob in ((0xa000,tiles),(0x9000,maps),(0x8800,sprites),(0x8c00,spal)):
  put(a,blob)
 reg(6,0x7040);execute(0x209215);execute(0x202c77);execute(0x201192);execute(0x200c7b);store(0x400d,load(0x400d,1)|192,1)
 records=J.create_records(case%2,case%6,65535)
 assert app.bridge_start(C.byref(initial),C.byref(s),C.byref(disp),C.byref(p),I.assets,palettejobs,I.descriptor,fi,fo,rngtable,rng,C.byref(assets),tiles,maps,sprites,spal,controls,0,course_resources,C.byref(records))==0
 s=obj(1,I.Screen);p=obj(3,I.Palette);disp=obj(2,I.Display);ni=obj(0,I.Init)
 controls=obj(4,U8*16);tiles=obj(5,U8*8192);maps=obj(6,U8*4096)
 app.bridge_title_export(title_values);app.bridge_round_export(round_values);
 prep=obj(18,K.Prepare);put(0x4149,bytes(prep.players[0]));put(0x4189,bytes(prep.players[1]));
 for bank in range(2):store(0x4149+64*bank+1,records.golfer[bank],1)
 for course in range(3):store(0x5847+66*course,255,1)
 compare_cursor((case,'start'),False);I.compare(p)
 app.bridge_export(values)
 for i in range(5):
  a=0x7000+64*i;v=values[i*9:i*9+9]
  for o,x,n in zip((0,4,8,12,16,17,18,19,20),v,(4,4,4,4,1,1,1,1,2)):store(a+o,x,n)
 store(0x70c8,0xffffffff,4);store(0x5706,values[45],4);store(0x40fb,sel,2);store(0x4148,0,1);store(0x400e,rng[0],1);put(0x40bf,controls)
 for i,x in enumerate((80,0,0,152)):store(0x8002+i,x,1)
 sounds=[0,0,0];round_sounds=[];tick=0
 while True:
  if tick<8:buttons=0
  elif tick<28:buttons=0 if case<3 else (8,0,4,0,8,0,0,0,4,0)[(tick-8)%10]
  elif tick==28:buttons=16 if case<18 else 80 if case<21 else 32
  elif case>=6 and case<21 and title_values[0] in (1,3):buttons=16 if tick%2==0 else 0
  elif case>=6 and case<21 and round_values[0]==3:buttons=16 if tick%2==0 else 0
  else:buttons=0
  store(0x6f82,buttons,1);execute(0x2002c2);execute(0x202bce);execute(0x201c25);execute(0x20054f)
  if load(0x400d,1)&80==80:execute(0x20067c);execute(0x200aa6)
  store(0x400d,load(0x400d,1)&63,1)
  reg(6,load(0x7008,4));reg(7,0x6be0);reg(8,0x20137c)
  for k in range(100000):
   pc=get(8)
   if pc==0x20137c and get(6)==0x7000:break
   if pc in (0x203f79,0x203eee):break # Stop before tee-shot/prior-selection allocation.
   if pc==0x250103:
    round_sounds.append(get(0)&65535)
    sp=get(7);reg(8,load(sp,4));reg(7,sp+4);continue # sound control request, not synthesis
   if pc==0x2501ba:
    sounds[get(0)&255]+=1;sp=get(7);reg(8,load(sp,4));reg(7,sp+4);continue
   step(0)
  else:raise RuntimeError(('dispatchlimit',case,tick,hex(get(8))))
  execute(0x202c77)
  if load(0x400d,1)&144==16:execute(0x201192);execute(0x200c7b);store(0x400d,load(0x400d,1)|192,1)
  result=app.bridge_update(buttons);assert result in (0,1),(case,tick,result);app.bridge_export(values);app.bridge_title_export(title_values);app.bridge_round_export(round_values)
  app.bridge_round_sounds(sound_values);assert list(sound_values)[1:1+sound_values[0]]==round_sounds,('sound control order',case,tick)
  compare_cursor((case,tick),True)
  assert bytes(controls)==data(0x40bf,16)
  assert rng[0]==load(0x400e,1),('shared random cursor',case,tick,rng[0],load(0x400e,1))
  assert values[48]==load(0x40fb,1);assert list(values[49:53])==[load(0x4545,2),load(0x454b,1),load(0x4113,1),load(0x4204,1)]
  assert list(values[53:57])==[load(0x8002+i,1) for i in range(4)];assert list(values[57:61])==[load(0x420b+i,1) for i in range(4)];assert list(values[61:64])==sounds
  assert values[45]==load(0x5706,4);assert values[47]==load(0x570a,4)
  current_records=obj(11,J.Records)
  for golfer in range(6):
   assert current_records.count[golfer]==load(0x594f+golfer*0x27+0x24,2)
   assert current_records.active[golfer]==load(0x594f+golfer*0x27+0x26,1)
  if values[66] and not round_values[0]:
   intro_node=next(0x7000+64*i for i in range(5) if values[i*9] in (0x2077d3,0x207809,0x20781a,0x2078da,0x207959,0x207990,0x2079a7))
   assert values[67]==load(intro_node+0x16,2)
   assert load(J.metadata[values[52]][0]+0,2)==values[67]
   assert J.metadata[values[52]][0]+values[68]==load(intro_node+0x18,4)
  free=load(0x5706,4);frees=set()
  while free!=0xffffffff:frees.add(free);free=load(free+8,4)
  for i in range(5):
   a=0x7000+64*i
   for j,(o,n) in enumerate(zip((0,4,8,12,16,17,18,19,20),(4,4,4,4,1,1,1,1,2))):
    expected=values[i*9+j]
    if j==2 and a in frees and expected==0:expected=0xffffffff
    assert expected==load(a+o,n),(case,tick,hex(a+o),hex(expected),hex(load(a+o,n)))
  I.compare(p);assert bytes(maps)==data(0x9000,4096);assert bytes(tiles)==data(0xa000,8192)
  assert s.cached_selection==load(0x40fd,2) and (round_values[4] if round_values[0] else title_values[4] if title_values[0] else s.cursor_x)==load(0x453f,2) and s.displayed_selection==load(0x447d,1)
  assert s.stamp_bytes==load(0x4d84,2);assert bytes(s.fill_status)==data(0x5782,8)
  if round_values[0]:
   assert bytes(prep.players[0])==data(0x4149,64) and bytes(prep.players[1])==data(0x4189,64),('preparation players',case,tick)
   assert bytes(prep.course_order)==data(0x441d,36),('preparation order',case,tick)
  updates+=1;tick+=1
  if result:
   boundaries[values[69]]=boundaries.get(values[69],0)+1
   if values[69]==5:
    courses_seen.add(values[52]);assert values[71]==values[70]+1 and values[72]==values[71]+2
    assert round_values[1]==title_values[3]+1 and round_values[2]==round_values[1]+2 and values[73]==round_values[3]+1, ('scheduled round timing',case,list(round_values))
   break
  assert tick<700
 count+=1
 if count%12==0:print("FLOW_PROGRESS",count,updates,file=sys.stderr,flush=True)
assert courses_seen=={0,1,2} and boundaries=={5:21,2:3},(courses_seen,boundaries)
print(json.dumps({'flows':count,'updates':updates,'courses':sorted(courses_seen),'boundaries':boundaries,'scope':'native selector, preparation, title, normal round record, nested drawing/blink and cleanup vs original; exact task ring, objects, input, tiles, maps, sprites, palettes and viewport; stops before tee-shot'}))
