#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Live original first-tee raster phase, checked against full retro_run replay."""
import sys,ctypes as C,subprocess,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from screen_probe import *
from run import read_inputs
import argparse
parser=argparse.ArgumentParser();parser.add_argument('--course',type=int,choices=range(3),default=0);parser.add_argument('--automatic',action='store_true');args=parser.parse_args()
OUT=ROOT/'build/shot-raster-live'/f"course{args.course}-{'auto' if args.automatic else 'input'}";OUT.mkdir(parents=True,exist_ok=True)
subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC',str(ROOT/'reference/shot_raster_live_bridge.c'),'-o',str(OUT/'trace.so')],check=True)
mask=0
@INPUT
def input(p,d,i,b):return int(d==1 and p==0 and bool(mask&(1<<b)))
core.retro_set_input_state(input)
syms={p[2]:int(p[0],16) for l in subprocess.check_output(['nm',str(ROOT/'reference/build/screen-probe/mednafen_ngp_libretro.so')],text=True).splitlines() if len(p:=l.split())==3}
base=C.cast(core.retro_reference_life_step,C.c_void_p).value-syms['retro_reference_life_step']
class Api(C.Structure):_fields_=[(n,C.c_void_p) for n in ('cpu','timers','z80','flush','pc','z80time','soundtime','gpu','ram')]
class Surface(C.Structure):_fields_=[('pixels',C.c_void_p)]+[(n,C.c_int32) for n in ('width','height','pitch','depth')]
gpu=C.c_void_p.from_address(base+syms['NGPGfx']).value
api=Api(*[base+syms[n] for n in ('TLCS900h_interpret','updateTimers','Z80_RunOP','MDFNNGPCSOUND_Flush','pc','z80_runtime','ngpc_soundTS')],gpu,C.addressof(ram.contents))
pixels=(C.c_uint16*(160*152))();surface=Surface(C.addressof(pixels),160,152,160,16)
lib=C.CDLL(str(OUT/'trace.so'));lib.frame.argtypes=[C.POINTER(Api),C.POINTER(Surface),C.c_uint,C.c_void_p]
libc=C.CDLL(None);libc.fopen.argtypes=[C.c_char_p,C.c_char_p];libc.fopen.restype=C.c_void_p;libc.fclose.argtypes=[C.c_void_p]
core.retro_serialize_size.restype=C.c_size_t
state_size=core.retro_serialize_size();core.retro_serialize.argtypes=[C.c_void_p,C.c_size_t];core.retro_unserialize.argtypes=[C.c_void_p,C.c_size_t]
def save():
 b=C.create_string_buffer(state_size);assert core.retro_serialize(b,state_size);return b
f=libc.fopen(str(OUT/'trace.jsonl').encode(),b'w');assert f
events=read_inputs(ROOT/'reference/inputs.gameplay-fairway.json');started=None
for frame in range(2500):
 if frame==1090:
  current=load(0x40fb,1);desired=(1,2,0)[args.course]
  for step_index in range(abs(desired-current)):
   for at in (1100+20*step_index,1101+20*step_index):events[at]=1<<(7 if desired>current else 6)
  print('COURSE_INPUT',current,desired,flush=True)
 mask=events.get(frame,0)
 if started is None:
  memory=C.string_at(C.addressof(ram.contents),16384);at=memory.find(bytes.fromhex('58c12000'))
  if at>=0 and int.from_bytes(memory[at+20:at+22],'little')<=149:
   started=frame;checkpoint=save();print('START',frame,hex(at+0x4000),load(at+0x4000+20,2),flush=True)
   assert load(0x8006,1)==198
   assert load(0x4204,1)==args.course,('Hardware course selection',load(0x4204,1),args.course)
 if started is None:core.retro_run()
 else:
  if args.automatic:mask=0
  store(0x6f82, sum(v for bit,v in ((4,1),(5,2),(6,4),(7,8),(0,16),(8,32),(3,64)) if mask&(1<<bit)),1)
  assert lib.frame(C.byref(api),C.byref(surface),frame,f)>0
  if frame-started>380:break
libc.fclose(f);assert started is not None,'Genuine input did not reach banner hold'
manual=save();assert core.retro_unserialize(checkpoint,state_size)
for again in range(started,frame+1):
 mask=0 if args.automatic else events.get(again,0);core.retro_run()
ordinary=save();diff=[i for i,(x,y) in enumerate(zip(manual.raw,ordinary.raw)) if x!=y]
print('DONE',started,frame,'state_bytes',state_size,'manual_vs_retro_differences',len(diff),diff[:20])
assert not diff


rows=[json.loads(line) for line in (OUT/'trace.jsonl').read_text().splitlines()]
events=[x for x in rows if 'pc' in x and x['pc']!=0x20c620]
assert len(events)==21 and [x['upper_after'] for x in events[1:-1]]==list(range(12,229,12))
assert events[0]['pc']==0x20042e and events[-1]['pc']==0x200453
assert all(152<=x['line']<198 for x in events),'Source writes cross visible or priming boundary'
visible=[x for x in rows if 'drawn_line' in x and x['drawn_line']<152]
assert len(visible)==42*152,'Incomplete visible row observation'
assert all(x['row_scroll']==(x['upper'] if x['drawn_line']<=72 else 0) for x in visible)
report={'result':'PASS','course':args.course,'exit':'automatic' if args.automatic else 'hardware_input','first_observed_frame':started,'last_observed_frame':frame,'full_state_replay_bytes':state_size,'state_differences':len(diff),'source_writes':events,'visible_rows_verified':len(visible),'register_line':events[0]['line'],'remove_line':events[-1]['line'],'source_update_lines':sorted(set(x['line'] for x in events[1:-1])),'scope':'Unmodified interpreter/timer/Z80 functions after genuine original input; complete resulting serializer bytes equal ordinary retro_run replay. All first-tee writes precede final blanking row198, proving visible row0-72 new upper/73-151 lower in these flows, not arbitrary gameplay.'}
(OUT/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
