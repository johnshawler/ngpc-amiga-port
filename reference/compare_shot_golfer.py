#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original20a58d initializer with actual terrain sampling and deferred art."""
from shot_golfer_case_common import *
import os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_GOLFER_LIBRARY',str(ROOT/'build/shot-golfer-check/native.so')));lib.ntm_shot_golfer_initialize.argtypes=[P,P,P,P];lib.ntm_course_screen_stamp_queue.argtypes=[P,P,P]
r=random.Random(0x20a58d);cases=0;drains=0;terrain_cache={};terrain_keep=[]
masks=scene_span(0x223104,8);shapes=scene_span(0x22310c,2048)
for case in range(648):
 setup();put(0x4000,r.randbytes(16384));store(0x40fa,16,1)
 # Author a valid64x64 terrain map. The original classifier and ROM per-pixel
 # shapes execute unchanged; both nibble terrains are selected identically.
 terrain=(case//3)%15;mapbytes=bytearray(128)
 for row in range(64):mapbytes[2*row:2*row+2]=len(mapbytes).to_bytes(2,'little');mapbytes+=bytes((64,17*terrain,r.randrange(256)))
 mb=blob(mapbytes);put(0x7000,mapbytes);put(0x7800,mapbytes)
 tr=TerrainResources(TerrainMap(addr(mb),len(mb)),TerrainMap(addr(mb),len(mb)),masks,shapes,8,2048)
 d=Display.from_buffer_copy(r.randbytes(C.sizeof(Display)));objects=(Cursor*32).from_buffer_copy(r.randbytes(C.sizeof(Cursor)*32));x=(U16*32)(*[r.randrange(65536) for _ in range(32)])
 full=case%5==0;ids=[j for j in range(32) if j!=5 or full];r.shuffle(ids);d.zero_flags[8]=len(ids);d.zero_words[0]=2*len(ids)
 for j in range(32):d.list[j]=ids[j] if j<len(ids) else r.randrange(65536);objects[j].id=d.object_ids[j]=j
 screen=Screen();screen.stamp_bytes=(0,248,256)[case%3]
 palette=Palette.from_buffer_copy(r.randbytes(C.sizeof(Palette)));occupied=case%9
 for j in range(8):palette.slots[j]=4*j+(0x400 if j<occupied else 0xff00)
 h=AM.State.from_buffer_copy(r.randbytes(C.sizeof(AM.State)));h.hole[4]=case%3;h.hole[0x70]=case%15
 # Special terrain6/special tee branches retain caller BC as an image selector;
 # the real first-fire queue is empty. Other branches also cover queue overflow.
 if h.hole[0x70]==6:screen.stamp_bytes=0
 for off,v in ((0x2a,300),(0x2c,300),(0x2e,400),(0x30,400),(0x3c,200),(0x3e,200),(0x6a,(80,160,161,300)[case%4])):h.hole[off:off+2]=v.to_bytes(2,'little')
 h.hole[0x40:0x44]=(0x7800).to_bytes(4,'little');h.auxiliary[7:9]=(65535).to_bytes(2,'little')
 for off,v in ((0x44,50<<16),(0x48,50<<16),(0x50,50<<16),(0x54,50<<16),(0x18,230<<16),(0x1c,230<<16),(0xd4,65536),(0xd8,0)):h.hole[off:off+4]=v.to_bytes(4,'little')
 if case%17==0 and h.hole[0x70]==4:h.auxiliary[7:9]=(10).to_bytes(2,'little');h.auxiliary[11:13]=(0).to_bytes(2,'little');screen.stamp_bytes=0
 if case>=540:
  course=(case-540)//36;hole_index=((case-540)//2)%18
  store(0x4206,hole_index,1);store(0x441d+2*hole_index,course,1);store(0x441e+2*hole_index,hole_index,1)
  for a,v in ((0x4146,0),(0x4147,0),(0x41ec,0),(0x41ed,0),(0x400e,(case*17)&255)):store(a,v,1)
  def helper(pc,registers=()):
   for index,value in enumerate(registers):reg(index,value)
   reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc)
   for _ in range(15000):
    if get(8)==0xef00:return
    step(0)
   raise AssertionError(('setup helper budget',case,hex(get(8))))
  helper(0x204a77);h=AM.read_state();h.hole[0x44:0x50]=h.players[0][4:16];h.hole[0x50:0x58]=h.players[0][4:12];h.hole[0x70]=4
  def hw(off,n=2):return int.from_bytes(bytes(h.hole[off:off+n]),'little')
  helper(0x20d024,(hw(0x44,4),hw(0x48,4),hw(0x18,4),hw(0x1c,4)));h.hole[0x68:0x6c]=get(0).to_bytes(4,'little')
  helper(0x20d5ca,(hw(0x46),hw(0x4a),hw(0x1a),hw(0x1e)));actual_angle=((get(0)&65535)+(0 if case%2 else 240))&4095
  helper(0x20d676,(actual_angle,));h.hole[0xd4:0xd8]=get(0).to_bytes(4,'little')
  helper(0x20d633,(actual_angle,));h.hole[0xd8:0xdc]=get(0).to_bytes(4,'little')
  key=(course,hole_index)
  if key not in terrain_cache:
   large,large_source=resolve_terrain(hw(0x28),terrain_keep);close,close_source=resolve_terrain(hw(0x3a),terrain_keep)
   terrain_cache[key]=(TerrainResources(large,close,masks,shapes,8,2048),large_source,close_source)
  tr,large_source,close_source=terrain_cache[key];assert hw(0x40,4)==close_source
 else:large_source=0x7000;actual_angle=r.randrange(4096)
 aim=AM.read_aim();aim.angle=actual_angle;words=(U16*2)(r.randrange(65536),r.randrange(65536));scrollx=(U8*2)(r.randrange(256),r.randrange(256));scrolly=(U8*2)(r.randrange(256),r.randrange(256));detail=U16(load(0x4411,2))
 ic=Context();ic.display=addr(d);ic.palette=addr(palette);ic.screen=addr(screen);ic.screen_words=addr(words);ic.scroll_x=addr(scrollx);ic.scroll_y=addr(scrolly)
 tc=TeeContext();tc.intro=addr(ic);tc.objects=addr(objects);tc.x=addr(x);scene=SceneObjectContext(addr(tc),addr(h),None,addr(aim),None);ctx=GolferContext(addr(scene),addr(tr),addr(detail),0)
 s=Golfer.from_buffer_copy(r.randbytes(C.sizeof(Golfer)));s.drawing=0x20a5aa;s.phase=0;task=Task();task.callback=0x20a58d
 def serial(b):
  serial_display(d,b);serial_objects(objects,x,b);serial_palette(palette,b);b[0x200:0x300]=bytes(h.hole);b[0x400:0x411]=bytes(h.auxiliary);AM.serialize_aim(aim,b)
  write(b,0x41db,ctx.player_status);write(b,0x4411,detail.value,2);write(b,0x5730,large_source,4);write(b,0x6000,task.callback,4);b[0x2016:0x201e]=bytes(s.task_bytes);write(b,0x4d84,screen.stamp_bytes,2)
  for j in range(2):write(b,0x4d80+2*j,words[j],2)
  for name,a in [('delta_x',0x4523),('delta_y',0x4527),('sample_x',0x452b),('sample_y',0x452f),('direction_x',0x4333),('direction_y',0x4337),('depth_scratch',0x5bcf)]:write(b,a,getattr(s,name),4)
  write(b,0x4533,s.attempts);write(b,0x4534,s.surface)
 b=bytearray(ram[:16384]);serial(b);put(0x4000,b);video=bytearray(data(0x8000,0x4000));before=screen.stamp_bytes//8
 for i in range(2):store(0x8032+2*i,scrollx[i],1);store(0x8033+2*i,scrolly[i],1);video[0x32+2*i]=scrollx[i];video[0x33+2*i]=scrolly[i]
 result=lib.ntm_shot_golfer_initialize(C.byref(s),C.byref(task),C.byref(ctx),C.byref(golfer_resources));assert result==0,(case,'native',result)
 assert d.object_tags[5]==objects[5].tag;serial(b)
 for k in range(min(2,8-occupied)):write_job(b,occupied+k,palette.jobs[occupied+k],golfer_palette_descriptors[h.hole[4]][k])
 for j in range(before,screen.stamp_bytes//8):
  st=screen.stamps[j];source=next(src for src,a in tee_assets if C.cast(a.data,P).value==C.cast(st.asset.data,P).value);write(b,0x4d86+8*j,source,4);write(b,0x4d8a+8*j,0x9000+st.offset,2);write(b,0x4d8c+8*j,st.tile,2)
 video[0x34]=scrollx[1];video[0x35]=scrolly[1]
 reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x20a58d);low=0x6be0
 for _ in range(20000):
  if get(8)==0xef00:break
  step(0);low=min(low,get(7))
 else:raise AssertionError(('budget',case,hex(get(8))))
 actual=bytes(ram[:16384]);b[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
 assert actual==b,(case,'RAM',[(hex(0x4000+i),a,v) for i,(a,v) in enumerate(zip(actual,b)) if a!=v][:20])
 actual=data(0x8000,0x4000);assert actual==video,(case,'video',[(hex(0x8000+i),a,v) for i,(a,v) in enumerate(zip(actual,video)) if a!=v][:10]);cases+=1
 if before==0:
  maps=blob(video[0x1000:0x2000]);tiles=blob(video[0x2000:0x4000])
  assert lib.ntm_course_screen_stamp_queue(C.byref(screen),tiles,maps)==0,(case,'native drain')
  write(b,0x4d84,screen.stamp_bytes,2);video[0x1000:0x2000]=bytes(maps);video[0x2000:0x4000]=bytes(tiles)
  reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x20054f);low=0x6be0
  for _ in range(30000):
   if get(8)==0xef00:break
   step(0);low=min(low,get(7))
  else:raise AssertionError(('drain budget',case,hex(get(8))))
  actual=bytes(ram[:16384]);b[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
  assert actual==b,(case,'drain RAM',[(hex(0x4000+i),a,v) for i,(a,v) in enumerate(zip(actual,b)) if a!=v][:20])
  actual=data(0x8000,0x4000);assert actual==video,(case,'drain video',[(hex(0x8000+i),a,v) for i,(a,v) in enumerate(zip(actual,video)) if a!=v][:20]);drains+=1
print(json.dumps({'golfer_cases':cases,'drawing_drains':drains,'actual_terrain_map_pairs':len(terrain_cache),'bounded_terrain_bytes':sum(len(b) for b in terrain_keep),'scope':'full nonstack RAM/video, actual sampler/projection,15 terrain classes,three courses,ground/foreground resources,palette/stamp/object saturation'}))
