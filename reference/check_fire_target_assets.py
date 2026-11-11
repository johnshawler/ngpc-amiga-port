#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compare exported private bindings with independent original resource resolvers."""
import argparse,ctypes as C,subprocess
from pathlib import Path
import shot_application_case_common as S
import shot_aim_application_case_common as A
import shot_scene_objects_case_common as SC
import shot_club_case_common as CL
import shot_markers_case_common as MK
from aim_object_assets import resolve_scene
import tee_setup_case_common as T
import hole_prepare_case_common as H
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--directory',type=Path,default=ROOT/'build/native-fire')
OUT=parser.parse_args().directory.resolve()
if not OUT.is_relative_to((ROOT/'build').resolve()):raise ValueError('Private generated assets must stay under ignored build/')
source='''#define NTM_AIM_OPEN_TARGET
#include "shot_fire_application.h"
#include "fire_stream.h"
#include <stdio.h>
#include "fire_assets.inc"
static long file_read(void *p,uint8_t *b,size_t n){size_t got=fread(b,1,n,p);return ferror(p)?-1:(long)got;}
static int file_rewind(void *p){return fseek(p,0,SEEK_SET);}
static int aim_assets_unpack(void){FILE *f=fopen("FIRE_POOL_PATH","rb");int result;fire_stream io;if(!f)return -1;io.context=f;io.read=file_read;io.rewind=file_rewind;result=fire_stream_unpack(&io,&fire_spec,tee_pool,sizeof tee_pool);fclose(f);return result;}
static ntm_shot_fire_application_resources fr;static ntm_shot_fire_resources fp;
static ntm_shot_spin_resources fs;static ntm_shot_spin_draw_resources fd;static ntm_shot_trajectory_resources ft;
static ntm_shot_golfer_resources fg;static ntm_shot_golfer_asset ga[FIRE_GOLFER_ASSET_COUNT];
static ntm_shot_golfer_animation_resources fan;static ntm_shot_golfer_drawing_resources fdraw[54];
static ntm_terrain_resources fterrain[54];static ntm_tee_object_assets fo[3];

static ntm_shot_resources s;static ntm_shot_prepare_resources p;static ntm_terrain_resources t;
static ntm_shot_banner_resources b;static ntm_tee_object_assets o,oo;static ntm_shot_palette_job j[2];
static ntm_shot_aim_application_resources ar;static ntm_shot_aim_resources aa;static ntm_aim_world_resources aw[54];
static ntm_shot_hud_resources ah;static ntm_shot_aim_setup_resources aset;static ntm_tee_object_assets ao[54];
static ntm_shot_open_resources opr;static ntm_shot_scene_object_resources scr;static ntm_shot_club_resources clr;static ntm_shot_marker_resources mkr;static ntm_tee_object_assets sor;
static ntm_tee_setup_resources r;static ntm_hole_prepare_resources h;
void *binding(unsigned n){if(aim_assets_unpack())return 0;aim_resources(&ar,&aa,aw,&ah,&aset,ao);open_resources(&opr,&scr,&clr,&mkr,&sor,&aa);tee_resources(&r,&oo,&h);shot_resources(&s,&p,&t,&b,&o,j);
switch(n){case0:return &p;case1:return &t;case2:return &b;case3:return &o;case4:return j;case5:return &r;case 7:return &ar;case 8:return &aa;case 9:return aw;case 10:return &ah;case 11:return &aset;case 12:return ao;case 13:return &scr;case 14:return &clr;case 15:return &mkr;case 16:return &sor;default:return &h;}}
'''.replace('case0','case 0').replace('case1','case 1').replace('case2','case 2').replace('case3','case 3').replace('case4','case 4').replace('case5','case 5')
source=source.replace('FIRE_POOL_PATH',str(OUT/'fire.pool'))
source += "void *fire_binding(void){(void)binding(7);fire_resources(&fr,&fp,&fs,&fd,&ft,&fg,ga,&fan,fdraw,fterrain,fo,&aa);return &fr;}"
(OUT/'check-assets.c').write_text(source)
subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-shared','-fPIC','-I'+str(ROOT/'native-game'),'-I'+str(OUT),'-I'+str(ROOT/'native-renderer/amiga'),str(ROOT/'native-renderer/amiga/fire_stream.c'),str(OUT/'check-assets.c'),'-o',str(OUT/'check-assets.so')],check=True)
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

a=obj(8,A.AM.AimResources)
blob(a,A.AM.aim_resources,'angles','angles_size')
for name,sizefield,count in (('powers','power_sizes',12),('minimums','minimum_sizes',6)):
 for i in range(count):
  n=getattr(a,sizefield)[i];assert n==getattr(A.AM.aim_resources,sizefield)[i]
  assert read(getattr(a,name)[i],n)==read(getattr(A.AM.aim_resources,name)[i],n);checks+=1
worlds=obj(9,A.WorldResources*54)
for i in range(54):
 a,b=worlds[i],A.worlds[i]
 for name in ('layout_tag','blocks_tag','terrain_tag','tiles_tag','width','height'):assert getattr(a,name)==getattr(b,name),(i,name)
 for name in ('layout','blocks','tile_indices','tiles','commands','colors','mono'):
  x,y=getattr(a,name),getattr(b,name)
  assert (x.size,x.origin)==(y.size,y.origin),(i,name)
  assert read(x.data,x.size)==read(y.data,y.size),(i,name);checks+=1
h=obj(10,A.hud_resources.__class__)
for name in ('font','header','label'):asset(getattr(h,name),getattr(A.hud_resources,name))
for i in range(17):asset(h.wind[i],A.hud_resources.wind[i])
assert h.label_id==A.hud_resources.label_id
s=obj(11,A.AimSetupResources)
for c in range(3):
 for h in range(18):asset(s.panorama[c][h],A.aim_setup_resources.panorama[c][h])
blob(s,A.aim_setup_resources,'metadata','metadata_size')
o=obj(12,S.O.Assets*54)
for i in range(54):
 for j in range(40):
  assert o[i].animations[j].frames==A.object_resources[i].animations[j].frames
  blob(o[i].animations[j],A.object_resources[i].animations[j],'data','size')
 for name in ('shapes','tiles'):blob(o[i],A.object_resources[i],name,name+'_size')
r=obj(7,A.AimApplicationResources);job(r.palette,A.palette)
print('aim target assets: independent resolver PASS',checks,'bounded spans; all54 worlds, origins, identities, panoramas and objects')
# Compile the original and packed declarations side by side. This catches both
# byte corruption and a changed C array type/sizeof (pointer macros are wrong).
import re,json
manifest=json.loads((OUT/'fire_intro.json').read_text())
packed=(OUT/'fire_intro.inc').read_text()
for name,_,_ in manifest['bindings']:packed=re.sub(r'\b'+name+r'\b','check_'+name,packed)
for name in ('intro_resources','intro_title_resources','intro_round_resources'):packed=re.sub(r'\b'+name+r'\b','check_'+name,packed)
packed=re.sub(r'^#undef check_.*$', '',packed,flags=re.M)
packed += '\nstatic long file_read(void *p,uint8_t *b,size_t n){size_t got=fread(b,1,n,p);return ferror(p)?-1:(long)got;}\nstatic int file_rewind(void *p){return fseek(p,0,SEEK_SET);}\nstatic int intro_assets_unpack(void){FILE *f=fopen("'+str(OUT/'intro.pool')+'","rb");fire_stream io;int r;if(!f)return -1;io.context=f;io.read=file_read;io.rewind=file_rewind;r=fire_stream_unpack(&io,&intro_spec,intro_unpacked,sizeof intro_unpacked);fclose(f);return r;}\n'
checks_c=['#include "shot_aim_application.h"','#include "fire_stream.h"','#include <stdio.h>','#include <string.h>',packed,'#include "intro_assets.inc"','int main(void){if(intro_assets_unpack())return 1;']
for name,n,_ in manifest['bindings']:checks_c += [f'if(sizeof {name}!=sizeof check_{name} || sizeof {name}!={n} || memcmp({name},check_{name},{n}))return 2;']
checks_c+=['return 0;}'];(OUT/'check-intro.c').write_text('\n'.join(checks_c))
subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-I'+str(ROOT/'native-game'),'-I'+str(OUT),'-I'+str(ROOT/'native-renderer/amiga'),str(ROOT/'native-renderer/amiga/fire_stream.c'),str(OUT/'check-intro.c'),'-o',str(OUT/'check-intro')],check=True)
subprocess.run([str(OUT/'check-intro')],check=True)
print('intro startup packing:',len(manifest['bindings']),'arrays exact original bytes, sizes and C sizeof PASS')

s=obj(13,SC.SceneObjectResources)
for name in ('metadata','power_scale','remainders'):blob(s,SC.scene_object_resources,name,name+'_size')
c=obj(14,CL.ClubResources)
for i in range(12):
 blob(c.powers[i],CL.club_resources.powers[i],'data','size')
 for j in range(15):
  for name in ('lies','overflow_lies'):blob(getattr(c,name)[i][j],getattr(CL.club_resources,name)[i][j],'data','size')
for name,count in (('labels',23),('digits',11),('lie_icons',19),('highlights',23)):
 for i in range(count):asset(getattr(c,name)[i],getattr(CL.club_resources,name)[i])
m=obj(15,MK.MarkerResources)
for name in ('quarter','metadata'):blob(m,MK.marker_resources,name,name+'_size')
o=obj(16,S.O.Assets);resolved=resolve_scene(S.load,S.data)
for velocity,(b,n) in resolved['animations'].items():
 x=o.animations[velocity//4];assert x.frames==n and x.size==len(b) and read(x.data,x.size)==b;checks+=1
for name in ('shapes','tiles'):
 assert read(getattr(o,name),getattr(o,name+'_size'))==resolved[name];checks+=1
print('aim opening and club highlight resources:',checks,'total independent exact span checks PASS')

import shot_fire_application_case_common as F
lib.fire_binding.restype=C.c_void_p
fr=C.cast(lib.fire_binding(),C.POINTER(F.FireApplicationResources)).contents
def fobj(name,t):return C.cast(getattr(fr,name),C.POINTER(t)).contents
job(fobj('parent',F.FireResources).palette,F.fire_resources.palette)
blob(fobj('spin',F.SpinResources),F.spin_resources,'bounds','bounds_size')
sd=fobj('spin_draw',F.SpinDrawResources)
for field in ('indicators','highlights'):
 for i in range(9):asset(getattr(sd,field)[i],getattr(F.spin_draw_resources,field)[i])
tr=fobj('trajectory',F.trajectory_resources.__class__)
for field in ('metadata','sprite_bases','hud_bases'):blob(tr,F.trajectory_resources,field,field+'_size')
for i in range(6):
 job(tr.palettes[i],F.trajectory_resources.palettes[i])
 for k in range(3):asset(tr.hud[i][k],F.trajectory_resources.hud[i][k])
g=fobj('golfer',F.GolferResources)
for field in ('course_selectors','ground_records','foreground_records','frames','overrides','green_records','pin_records','metadata'):blob(g,F.golfer_resources,field,field+'_size')
for i in range(3):
 for k in range(2):job(g.palettes[i][k],F.golfer_resources.palettes[i][k])
assert g.asset_count==F.golfer_resources.asset_count
for a,b in zip(C.cast(g.assets,C.POINTER(F.GolferAsset*g.asset_count)).contents,F.golfer_assets):
 assert a.selector==b.selector;asset(a.asset,b.asset)
assert fr.hole_count==54
for a,b in zip(fobj('terrain',F.TerrainResources*54),F.terrains):
 for field in ('large','close'):blob(getattr(a,field),getattr(b,field),'data','size')
 for field in ('masks','shapes'):blob(a,b,field,field+'_size')
for a,b in zip(fobj('drawing',F.GolferDrawingResources*54),F.drawings):
 blob(a.features,b.features,'features','features_size')
 asset(a.rim,b.rim)
 for i in range(4):asset(a.green[i],b.green[i])
 for field in ('eligible','depth_records','clip_records','metadata'):blob(a,b,field,field+'_size')
 assert a.clip_origin==b.clip_origin
 an=C.cast(a.animation,C.POINTER(F.GolferAnimationResources)).contents
 for row in range(5):
  for frame in range(4):asset(an.frames[row][frame],F.golfer_animation_resources.frames[row][frame])
for field,original in [('trajectory_objects',F.trajectory_objects),('pin_objects',F.pin_objects),('green_objects',F.green_objects)]:
 a=fobj(field,F.O.Assets)
 for i in range(40):
  assert a.animations[i].frames==original.animations[i].frames
  blob(a.animations[i],original.animations[i],'data','size')
 for field in ('shapes','tiles'):blob(a,original,field,field+'_size')
print('fire target assets: independent resolver PASS',checks,'bounded checks; all54 terrain/features,18 trajectory animations,pin/green,all fire art')
