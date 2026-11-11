#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compare exported private bindings with independent original resource resolvers."""
import ctypes as C,subprocess
from pathlib import Path
import shot_application_case_common as S
import tee_setup_case_common as T
import hole_prepare_case_common as H
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'build/native-shot'
source='''#include "shot_application.h"
#include "shot_assets.inc"
static ntm_shot_resources s;static ntm_shot_prepare_resources p;static ntm_terrain_resources t;
static ntm_shot_banner_resources b;static ntm_tee_object_assets o,oo;static ntm_shot_palette_job j[2];
static ntm_tee_setup_resources r;static ntm_hole_prepare_resources h;
void *binding(unsigned n){tee_resources(&r,&oo,&h);shot_resources(&s,&p,&t,&b,&o,j);
switch(n){case0:return &p;case1:return &t;case2:return &b;case3:return &o;case4:return j;case5:return &r;default:return &h;}}
'''.replace('case0','case 0').replace('case1','case 1').replace('case2','case 2').replace('case3','case 3').replace('case4','case 4').replace('case5','case 5')
(OUT/'check-assets.c').write_text(source)
subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-shared','-fPIC','-I'+str(ROOT/'native-game'),'-I'+str(OUT),str(OUT/'check-assets.c'),'-o',str(OUT/'check-assets.so')],check=True)
lib=C.CDLL(str(OUT/'check-assets.so'));lib.binding.argtypes=[C.c_uint];lib.binding.restype=C.c_void_p
checks=0

def read(p,n):return C.string_at(p,n) if n else b''
def blob(a,b,name,size):
 global checks
 assert getattr(a,size)==getattr(b,size),(name,size)
 assert read(getattr(a,name),getattr(a,size))==read(getattr(b,name),getattr(b,size)),name
 checks+=1

def asset(a,b):blob(a,b,'data','size');blob(a,b,'tiles','tiles_size')
def job(a,b):
 for name in ('kind','pending_index','offset','count','color','control_offset'):assert getattr(a,name)==getattr(b,name),name
 blob(a,b,'offsets','offsets_size');blob(a,b,'colors','colors_size')
def obj(n,t):return C.cast(lib.binding(n),C.POINTER(t)).contents
p=obj(0,S.PreparationResources)
for n in ('traits','wind_low','wind_step','wind_limit'):blob(p,S.preparation,n,n+'_size')
t=obj(1,S.TerrainResources)
for n in ('masks','shapes'):blob(t,S.terrain,n,n+'_size')
assert not t.large.data and not t.close.data
b=obj(2,S.BannerResources)
for name in ('terrain','tee_shot','portrait'):asset(getattr(b,name),getattr(S.banner_resources,name))
for name,count in (('characters',6),('players',2),('hints',2)):
 for i in range(count):asset(getattr(b,name)[i],getattr(S.banner_resources,name)[i])
for i in range(2):job(b.palettes[i],S.banner_resources.palettes[i])
blob(b,S.banner_resources,'metadata','metadata_size')
o=obj(3,S.O.Assets)
for i in range(40):
 assert o.animations[i].frames==S.shot_objects.animations[i].frames
 blob(o.animations[i],S.shot_objects.animations[i],'data','size')
for n in ('shapes','tiles'):blob(o,S.shot_objects,n,n+'_size')
j=C.cast(lib.binding(4),C.POINTER(S.AnimatedJob*2)).contents
for i in range(2):
 for name in ('kind','speed','pending_index','offset','count','source_color','target_color','control_offset'):assert getattr(j[i],name)==getattr(S.exit_palettes[i],name)
 for name in ('source','target','blend'):blob(j[i],S.exit_palettes[i],name,name+'_size')
r=obj(5,T.TeeResources)
for name in ('font','header'):asset(getattr(r,name),getattr(T.tee_resources,name))
for name,count in (('hole_numbers',18),('pars',3)):
 for i in range(count):asset(getattr(r,name)[i],getattr(T.tee_resources,name)[i])
for c in range(3):
 for h in range(18):
  a,b=r.holes[c][h],T.tee_resources.holes[c][h];assert (a.marker_x,a.marker_y)==(b.marker_x,b.marker_y)
  for v in range(3):asset(a.views[v],b.views[v])
for i in range(4):assert read(r.metadata[i],r.metadata_size[i])==read(T.tee_resources.metadata[i],T.tee_resources.metadata_size[i])
for i in range(2):job(r.palettes[i],T.tee_resources.palettes[i])
print('shot target assets: independent resolver PASS',checks,'bounded spans,54holes,162views,40object slots,all banner and fade resources')
