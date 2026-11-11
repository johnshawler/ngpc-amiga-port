#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Separate exact target input replay, preserving the original application oracle.
# Private comparison only; no cartridge bytes or captures embedded.
from cursor_native_case_common import *
import course_init_case_common as I
import course_intro_case_common as J
import os
import cursor_native_case_common as native
import compare_course_title_cursor as title_object
import round_record_case_common as K
import tee_setup_case_common as TT
import hole_prepare_case_common as H
import compare_tee_objects as T
import shot_application_case_common as S
import shot_aim_application_case_common as A
import shot_open_application_case_common as OP
app=C.CDLL(os.environ.get('NTM_SHOT_APPLICATION_LIBRARY',str(ROOT/'build/native-control/oracle.so')));app.bridge_start.argtypes=[C.c_void_p]*17+[C.c_uint]+[C.c_void_p]*2;app.bridge_pointer.argtypes=[C.c_uint];app.bridge_pointer.restype=C.c_void_p;app.bridge_export.argtypes=[C.POINTER(U32)]
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
TT.tee_resources.objects=C.addressof(T.assets)
app.bridge_tee.argtypes=[C.c_void_p,C.c_void_p];app.bridge_tee(C.byref(TT.tee_resources),C.byref(H.resources))
app.bridge_tee_state.argtypes=[C.c_uint,C.c_uint,C.c_uint]
tee_values=(U32*13)();tee_sound_values=(U32*192)();app.bridge_tee_export.argtypes=[C.POINTER(U32)];app.bridge_tee_sounds.argtypes=[C.POINTER(U32)]
shot_values=(U32*19)();extra_values=(U32*36)()
app.bridge_shot_export.argtypes=[C.POINTER(U32)];app.bridge_extra_export.argtypes=[C.POINTER(U32)]
app.bridge_shot.argtypes=[C.c_void_p];app.bridge_shot(C.byref(S.shot_resources))
app.bridge_aim_pointer.argtypes=[C.c_uint];app.bridge_aim_pointer.restype=C.c_void_p
app.bridge_aim_enable.argtypes=[C.c_void_p]
def aim_obj(n,t):return C.cast(app.bridge_aim_pointer(n),C.POINTER(t)).contents
app.bridge_open_pointer.argtypes=[C.c_uint];app.bridge_open_pointer.restype=C.c_void_p
app.bridge_open_enable.argtypes=[C.c_void_p]
def open_obj(n,t):return C.cast(app.bridge_open_pointer(n),C.POINTER(t)).contents
app.bridge_control_pointer.argtypes=[C.c_uint];app.bridge_control_pointer.restype=C.c_void_p
def control_obj(n,t):return C.cast(app.bridge_control_pointer(n),C.POINTER(t)).contents
class CameraState(C.Structure):_fields_=[("saved_x",U16),("saved_y",U16)]
tee_shape_root=T.shape_root
selector_shapes=native.shapes
def obj(n,t):return C.cast(app.bridge_pointer(n),C.POINTER(t)).contents

def compare_cursor(label,head):
 if tee_values[0]:
  m=object.__new__(T.Native);m.b=bytearray(ram[:16384]);m.d=obj(2,T.Display);m.e=obj(25,T.Engine)
  m.c=obj(24,T.Cursor*32);m.x=obj(28,U16*32);m.global_=U8(obj(0,I.Init).flags)
  m.ts=obj(5,U8*8192);m.ss=obj(7,U8*256);m.ps=obj(8,U8*64)
  T.shape_root=OP.resolved_scene["shape_address"] if open_obj(5,U8).value else A.object_shapes[18*obj(27,H.State).hole[4]+obj(27,H.State).hole[5]] if aim_obj(3,U8).value else S.resolved["shape_address"] if shot_values[0] else tee_shape_root
  m.serial(oldjobs=0,head=head)
  for j in range(2):m.w(0x4d80+2*j,obj(0,I.Init).screen_words[j],2)
  m.compare(label);return
 m=object.__new__(Native);m.b=bytearray(ram[:16384]);m.d=obj(2,Display);m.e=obj(9,Engine)
 ni=obj(0,I.Init);m.c=obj(20,Cursor) if round_values[0] else obj(15,Cursor) if title_values[0] else C.cast(app.bridge_pointer(0)+I.Init.cursor.offset,C.POINTER(Cursor)).contents;m.global_=U8(ni.flags)
 native.shapes=K.round_shapes if round_values[0] else shape_roots[values[52]] if title_values[0] else selector_shapes
 m.ts=obj(5,U8*8192);m.ss=obj(7,U8*256);m.ps=obj(8,U8*64)
 m.serial(head=head)
 for k in range(m.d.zero_flags[10]):m.serial(newjob=k)
 m.compare(label)
import argparse,struct
parser=argparse.ArgumentParser();parser.add_argument('--directory',type=Path,required=True);args=parser.parse_args()
final=(args.directory/'chip.finished.bin').read_bytes();timeline=struct.unpack_from('>128I',final,final.index(b'NCTR0001'))
trace=final[timeline[101]:timeline[101]+timeline[102]]
captures={}
for path in args.directory.glob('chip.*.bin'):
 raw=path.read_bytes();w=struct.unpack_from('>128I',raw,raw.index(b'NCTR0001'));captures.setdefault(w[19],[]).append((path,raw,w))
verified=[];original_pictures={};control_states=[]
count=updates=0;courses_seen=set();boundaries={}
for case in range(1):
 setup();initial=I.Init();s=I.Screen();p=I.Palette();disp=I.Display();controls=buf(bytes([0,0,0,0,0,7,32,0]+[0]*8));rng=buf(bytes([0]));tiles=buf(bytes(8192));maps=buf(bytes(4096));sprites=buf(bytes(256));spal=buf(bytes(64));sel=1
 for i in range(8):s.fill_status[i]=255;p.slots[i]=0xff00+4*i;store(0x5782+i,255,1);store(0x66bf+2*i,p.slots[i],2)
 s.displayed_selection=sel;store(0x447d,sel,1);store(0x40fa,16,1);store(0x87e2,0,1);p.pending[0]=p.pending[1]=255;store(0x69f1,255,1);store(0x69f2,255,1)
 for a,v in ((0x8118,0x81),(0x8012,7)):store(a,v,1)
 p.background=0x81;p.window_control=7
 for i in range(512):store(0x8200+i,0,1)
 for a,blob in ((0xa000,tiles),(0x9000,maps),(0x8800,sprites),(0x8c00,spal)):
  put(a,blob)
 reg(6,0x7040);execute(0x209215);execute(0x202c77);execute(0x201192);execute(0x200c7b);store(0x400d,load(0x400d,1)|192,1)
 records=J.Records()
 app.bridge_control_reset()
 assert app.bridge_start(C.byref(initial),C.byref(s),C.byref(disp),C.byref(p),I.assets,palettejobs,I.descriptor,fi,fo,rngtable,rng,C.byref(assets),tiles,maps,sprites,spal,controls,0,course_resources,C.byref(records))==0
 assert app.bridge_aim_enable(C.byref(A.resources))==0
 assert app.bridge_open_enable(C.byref(OP.open_resources))==0
 assert app.bridge_control_enable()==0
 store(0x573c,0x9800,2);store(0x573e,0x8110,2);store(0x5740,0x8300,2)
 app.bridge_tee_state(1 if 24<=case<30 else 0,case%3,case>=30)
 s=obj(1,I.Screen);p=obj(3,I.Palette);disp=obj(2,I.Display);ni=obj(0,I.Init)
 controls=obj(4,U8*16);tiles=obj(5,U8*8192);maps=obj(6,U8*4096)
 app.bridge_title_export(title_values);app.bridge_round_export(round_values);app.bridge_tee_export(tee_values);app.bridge_shot_export(shot_values);app.bridge_extra_export(extra_values);
 prep=obj(18,K.Prepare);put(0x4149,bytes(prep.players[0]));put(0x4189,bytes(prep.players[1]));
 for bank in range(2):store(0x4149+64*bank+1,records.golfer[bank],1)
 for course in range(3):
  store(0x5847+66*course,255,1);put(0x586b+66*course,bytes([1 if case>=30 else 10]*18))
 store(0x4146,2,1);store(0x4147,0,1);store(0x40f9,1,1);store(0x453a,1 if 24<=case<30 else 0,1);store(0x6ad8,case%3,1)
 compare_cursor((case,'start'),False);I.compare(p)
 app.bridge_export(values)
 for i in range(5):
  a=0x7000+64*i;v=values[i*9:i*9+9]
  for o,x,n in zip((0,4,8,12,16,17,18,19,20),v,(4,4,4,4,1,1,1,1,2)):store(a+o,0xffffffff if o==8 and not x else x,n)

 for i in range(4):
  a=0x7180+64*i;v=extra_values[9*i:9*i+9]
  for o,x,n in zip((0,4,8,12,16,17,18,19,20),v,(4,4,4,4,1,1,1,1,2)):store(a+o,0xffffffff if o==8 and not x else x,n)
 rs=obj(35,S.Raster);rs.timer_run=136;rs.interrupt_shadow=0;rs.gpu_control=128
 store(0x20,rs.timer_run,1);store(0x7248,0xffffffff,4);store(0x6c27,rs.interrupt_shadow,1);store(0x6fd4,0x200471,4);store(0x8000,rs.gpu_control,1);store(0x5706,values[45],4);store(0x40fb,sel,2);store(0x4148,0,1);store(0x400e,rng[0],1);put(0x40bf,controls)
 for i,x in enumerate((80,0,0,152)):store(0x8002+i,x,1)
 sounds=[0,0,0];round_sounds=[];tee_sounds=[];tick=0
 while True:
  assert tick<len(trace), ("Input trace exhausted",tick)
  buttons=trace[tick]
  store(0x6f82,buttons,1);execute(0x2002c2);execute(0x202bce);execute(0x201c25);execute(0x20054f)
  if load(0x400d,1)&80==80:execute(0x20067c);execute(0x200aa6)
  store(0x400d,load(0x400d,1)&63,1)
  if aim_obj(3,U8).value and load(0x400d,1)&8:execute(0x201608)
  if open_obj(6,U8).value:execute(0x200472)
  reg(6,load(0x7008,4));reg(7,0x6be0);reg(8,0x20137c)
  for k in range(100000):
   pc=get(8)
   if pc==0x20137c and get(6)==0x7000:break
   if pc in (0x203eee,):break # Stop before unconverted scene/aim setup or prior selection.
   if pc in (0x250103,0x2500ea):
    service=int(pc==0x250103);command=get(0)&65535;parameter=get(1)&65535
    sp=get(7);caller=load(sp,4)
    if 0x20bf8e<=caller<=0x20bfa2 or 0x20ca45<=caller<=0x20cac8 or 0x20534a<=caller<=0x205376:
     tee_sounds.append((service,command,0 if 0x20bf8e<=caller<=0x20bfa2 else parameter))
     if command==65535:store(0x6ad8,(parameter+16*service)&255,1)
    else:round_sounds.append(command)
    reg(8,caller);reg(7,sp+4);continue # requests plus query-byte metadata, not synthesis
   if pc==0x2501d1:
    tee_sounds.append((2,get(0)&65535,get(1)&255));sp=get(7);reg(8,load(sp,4));reg(7,sp+4);continue
   if pc==0x2501ba:
    sounds[get(0)&255]+=1;sp=get(7);reg(8,load(sp,4));reg(7,sp+4);continue
   step(0)
  else:raise RuntimeError(('dispatchlimit',case,tick,hex(get(8))))
  execute(0x202c77)
  if load(0x400d,1)&144==16:execute(0x201192);execute(0x200c7b);store(0x400d,load(0x400d,1)|192,1)
  result=app.bridge_update(buttons);assert result in (0,1),(case,tick,result);app.bridge_export(values);app.bridge_title_export(title_values);app.bridge_round_export(round_values);app.bridge_tee_export(tee_values);app.bridge_shot_export(shot_values);app.bridge_extra_export(extra_values)
  app.bridge_round_sounds(sound_values);assert list(sound_values)[1:1+sound_values[0]]==round_sounds,('sound control order',case,tick)
  count_sounds=app.bridge_tee_sounds(tee_sound_values);assert [tuple(tee_sound_values[3*i:3*i+3]) for i in range(count_sounds)]==tee_sounds,('tee request order',case,tick)
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
  assert s.cached_selection==load(0x40fd,2) and (obj(28,U16*32)[0] if tee_values[0] else round_values[4] if round_values[0] else title_values[4] if title_values[0] else s.cursor_x)==load(0x453f,2) and s.displayed_selection==load(0x447d,1)
  assert s.stamp_bytes==load(0x4d84,2);assert bytes(s.fill_status)==data(0x5782,8)
  if round_values[0] and not shot_values[0]:
   assert bytes(prep.players[0])==data(0x4149,64) and bytes(prep.players[1])==data(0x4189,64),('preparation players',case,tick)
   assert bytes(prep.course_order)==data(0x441d,36),('preparation order',case,tick)
  if tee_values[2]:
   hole=obj(27,H.State)
   assert bytes(hole.players[0])==data(0x4149,64) and bytes(hole.players[1])==data(0x4189,64),('hole players',case,tick)
   if tee_values[2]:
    assert bytes(hole.hole)==data(0x4200,256),('hole state',case,tick,[(hex(0x4200+i),x,load(0x4200+i,1)) for i,x in enumerate(hole.hole) if x!=load(0x4200+i,1)][:12])
    assert bytes(hole.auxiliary)==data(0x4400,17),('hole auxiliary',case,tick)
    assert hole.difficulty==load(0x4146,1) and hole.selected_difficulty==load(0x4147,1) and hole.ready_flag==load(0x41e3,1)
    assert tee_values[10]==load(0x6ad8,1),('music control',case,tick)
   if tee_values[2]>=2 and not shot_values[0]:
    overview_node=next(0x7000+64*i for i in range(5) if values[i*9] in (0x20cd28,0x20cd5e,0x20cd81,0x20cdac))
    assert tee_values[8]==load(overview_node+22,2) and tee_values[9]==load(overview_node+24,2)
  for path,raw,w in captures.get(tick+1,[]):
   for field,address,size in ((22,0xa000,8192),(23,0x9000,4096),(24,0x8800,256),(25,0x8c00,64),(26,0x8200,512)):
    expected=data(address,size);actual=raw[w[field]:w[field]+size]
    assert actual==expected,(path.name,'original source mismatch',hex(address),next((i for i,(x,y) in enumerate(zip(actual,expected)) if x!=y),None))
   tm=struct.unpack_from('>128I',raw,w[81]);assert tm[11]==rng[0],(path.name,'original shared RNG cursor',tm[11],rng[0])
   assert w[21]==load(0x40fb,1),(path.name,'original selection')
   sm=struct.unpack_from('>64I',raw,w[97])
   assert list(sm[:4])==list(shot_values[:4]),(path.name,'shot phases',sm[:4],list(shot_values[:4]))
   rowplan=raw[w[100]:w[100]+152]
   rs=obj(35,S.Raster)
   expected_rows=bytes([rs.upper_scroll]*73+[rs.lower_scroll]*79) if rs.callback else bytes([load(0x8032,1)]*152)
   assert rowplan==expected_rows,(path.name,'per-row source plan')
   assert raw[w[27]:w[27]+4]==bytes(load(a,1) for a in (0x8032,0x8034,0x8033,0x8035)),(path.name,'snapshot scroll scalars')
   cm=struct.unpack_from('>24I',raw,w[117])
   if cm[19]==2:
    expected_control=[load(a,n) for a,n in ((0x41d6,2),(0x41dc,1),(0x5710,2),(0x5712,2),(0x4416,1),(0x4417,2),(0x4419,2),(0x41e1,1),(0x41da,1),(0x41dd,1))]
    assert list(cm[3:13])==expected_control,(path.name,'control metadata',list(cm[3:13]),expected_control)
    assert list(cm[15:19])==[load(a,1) for a in (0x40bf,0x40c0,0x8034,0x8035)]
    assert cm[2]==0 and cm[20:24]==(1,0,1,0)
    original_pictures[path.stem[5:]]=data(0x8000,0x4000)
   verified.append(path.name)
  updates+=1;tick+=1
  if shot_values[0]:
   assert shot_values[8:10]==[0,1]
   assert shot_values[7]==load(0x41ea,2) and shot_values[12]==load(0x41db,1)
   assert load(0x41c9,4)==0x4149 and load(0x41cd,4)==0x4189
   assert shot_values[10]==load(0x41e7,1) and shot_values[11]==load(0x4411,2)
  for i in range(4):
   a=0x7180+64*i
   for j,(o,n) in enumerate(zip((0,4,8,12,16,17,18,19,20),(4,4,4,4,1,1,1,1,2))):
    expected=extra_values[i*9+j]
    if j==2 and a in frees and expected==0:expected=0xffffffff
    assert expected==load(a+o,n),(case,tick,hex(a+o),hex(expected),hex(load(a+o,n)))
  if shot_values[17]==1:
   animation_node=next((0x7000+64*i if i<5 else 0x7180+64*(i-5)) for i,cb in enumerate(list(values[::9])[:5]+list(extra_values[::9])) if cb==0x20c503)
   assert shot_values[15]==load(animation_node+22,2) and shot_values[16]==load(animation_node+24,1)
  if shot_values[3]:
   banner_node=next(((0x7000+64*i if i<5 else 0x7180+64*(i-5)) for i,cb in enumerate(list(values[::9])[:5]+list(extra_values[::9])) if cb in (0x20c0f7,0x20c13e,0x20c158,0x20c199)),None)
   if banner_node is not None:assert shot_values[13]==load(banner_node+22,1) and shot_values[14]==load(banner_node+23,1)
   else:assert open_obj(5,U8).value and shot_values[3]==5 # Real children have reused the completed banner node.
   rs=obj(35,S.Raster)
   assert rs.upper_scroll==load(0x4443,1) and rs.lower_scroll==load(0x4441,1) and rs.interrupt_shadow==load(0x6c27,1)
   assert load(0x6fd4,4)==(0x20c620 if rs.callback else 0x200471)
   assert rs.gpu_control==load(0x8000,1) and rs.timer_run==load(0x20,1),(case,tick,rs.gpu_control,load(0x8000,1),rs.timer_run,load(0x20,1))
   ap=obj(36,S.Animated)
   for j in range(8):
    if ap.jobs[j].kind:
     assert ap.fraction[j]==load(0x66cf+32*j+7,1) and ap.level[j]==load(0x66cf+32*j+16,1)
  if aim_obj(3,U8).value:
   aim=aim_obj(0,A.AM.Aim)
   for field,(address,size) in A.AM.AIM_FIELDS.items():assert getattr(aim,field)==load(address,size),(case,tick,field)
   assert bytes(aim_obj(1,A.WorldState).bytes)[:50]==data(0x5710,50),(case,tick,"world state")
   assert bytes(aim_obj(2,U8*4))==data(0x451f,4)
   # Every live deferred screen job retains its original asset identity.
   for j in range(s.stamp_bytes//8):
    st=s.stamps[j];source=next(src for src,asset in TT.tee_assets if C.cast(asset.data,C.c_void_p).value==C.cast(st.asset.data,C.c_void_p).value)
    assert (source,0x9000+st.offset,st.tile)==(load(0x4d86+8*j,4),load(0x4d8a+8*j,2),load(0x4d8c+8*j,2)),(case,tick,"pending stamp",j)
   for j in range(8):
    if s.fill_status[j]!=255:
     f=s.fills[j]
     assert (0x9000+f.offset,f.height,f.width,f.fill)==(load(0x5742+8*j,4),load(0x5746+8*j,1),load(0x5747+8*j,1),load(0x5748+8*j,2))
  if open_obj(5,U8).value:
   club=open_obj(0,OP.Club);markers=open_obj(1,OP.Markers);hint=open_obj(2,OP.Hint)
   for n,a in [('label',0x41dd),('delta',0x41de),('restore_delay',0x41e1),('change_flag',0x41da)]:assert getattr(club,n)==load(a,1),(case,tick,n)
   assert bytes(open_obj(4,U8*32))==data(0x44ff,32)
   callbacks=list(values[::9])[:5]+list(extra_values[::9])
   for n,cb in enumerate(callbacks):
    node=0x7000+64*n if n<5 else 0x7180+64*(n-5)
    if cb==0x20ad1a:assert (club.maximum,club.minimum)==(load(node+22,1),load(node+23,1))
    if cb==0x20af8a:assert (markers.step,markers.limit)==(load(node+22,1),load(node+23,2))
    if cb==0x20c775:assert (hint.repeat,hint.last,hint.pending,hint.changed)==tuple(load(node+22+j,1) for j in range(4))
  if open_obj(5,U8).value==2:
   control_states.append({'update':tick,'held':load(0x40bf,1),'pressed':load(0x40c0,1),'angle':load(0x41d6,2),'club':load(0x41dc,1),'x':load(0x5710,2),'y':load(0x5712,2),'camera':load(0x4416,1),'highlight':load(0x41e1,1)})
   cam=control_obj(0,CameraState)
   assert (cam.saved_x,cam.saved_y)==(load(0x4417,2),load(0x4419,2)),(case,tick,'camera saved')
   assert list(control_obj(4,U8*2))==[load(0x8032,1),load(0x8034,1)]
   assert list(control_obj(5,U8*2))==[load(0x8033,1),load(0x8035,1)]
  if tick==len(trace) and not result:
   assert control_obj(1,U8).value==1 and control_obj(2,U8).value==0
   assert load(0x413b,1)==0 and load(0x413c,4)==0x204786 and load(0x4140,4)==0x20485a
   courses_seen.add(values[52]);boundaries['host-stopped-running']=1;break
  if result:
   boundary=shot_values[1] if shot_values[0] else tee_values[1] if tee_values[0] else -values[69]
   boundaries[boundary]=boundaries.get(boundary,0)+1
   if boundary==4:
    assert open_obj(5,U8).value==2 and open_obj(6,U8).value==1
    assert load(0x413c,4)==0x204786 and load(0x4140,4)==0x20485a and load(0x413b,1)==open_obj(7,U8).value==0
    courses_seen.add(values[52]);assert shot_values[2]==2 and shot_values[3]==5
    assert shot_values[4]==tee_values[7]+1 and shot_values[5]==shot_values[4]
    assert shot_values[6]==values[73]
   break
  assert tick<=4096
 count+=1
 if count%12==0:print("FLOW_PROGRESS",count,updates,file=sys.stderr,flush=True)
assert len(verified)==sum(map(len,captures.values())),('Unverified capture',set(captures)-set(range(1,tick+1)))
assert tick==len(trace)
report={'result':'PASS','updates':updates,'captures':verified,'courses':sorted(courses_seen),'boundaries':boundaries,'source_bytes_per_capture':13120,'scope':'Exact recorded hardware input replay through unchanged original routines and independent cartridge resources; maps,tiles,sprites,spritepalettes,palettes,selection,shot phases and bounded row plan'}
report['control_states']=control_states
if original_pictures:
 from control_original_pixels import verify as verify_original_pixels
 report['original_gpu_pixels']=verify_original_pixels(core,load,put,ROOT,args.directory,original_pictures)
(args.directory/'original-verification.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='control_states'}))
