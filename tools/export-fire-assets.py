#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private file-backed fire resources. All 54 maps/features share source overlap.

Extends, but never changes, the preserved control exporter. No emulation, no
embedded original engine, no generated private bytes outside ignored build/.
"""
import argparse, hashlib, importlib.util, json, re, struct
from pathlib import Path
from aim_pack import pack, unpack
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('fire_base', ROOT/'tools/export-aim-assets.py')
base = importlib.util.module_from_spec(spec); spec.loader.exec_module(base)

def checksum(data):
    h = 2166136261
    for b in data: h = ((h ^ b) * 16777619) & 0xffffffff
    return h

def private(path):
    if not path.resolve().is_relative_to((ROOT/'build').resolve()):
        raise ValueError('Private output must stay under ignored build/')

def packed_file(path, raw):
    private(path); encoded = pack(raw)
    if unpack(encoded, len(raw)) != raw: raise ValueError('Lossless roundtrip failed')
    h = checksum(encoded)
    path.write_bytes(b'NTMFPK1\0' + struct.pack('>III', len(raw), len(encoded), h) + encoded)
    path.with_suffix('.raw').write_bytes(raw)
    return dict(raw_bytes=len(raw), packed_bytes=len(encoded), file_bytes=20+len(encoded),
                checksum=h, raw_sha256=hashlib.sha256(raw).hexdigest(),
                packed_sha256=hashlib.sha256(encoded).hexdigest())

class Resources(base.Resources):
    def field(self, target, source, size):
        label=self.span('fire_span_'+str(len(self.spans)), source, size)
        self.bindings += [f'{target}={label};{target}_size={size};']
    def palette(self, target, command):
        v=self.value; p=v(0x2198f3+command); kind=p&255; s=p>>8
        if kind==4:
            n=v(s,1); dest=8*v(s+1,1); ptr=v(s+2)
            if not 0<n<=64 or dest+8*n>512: raise ValueError('Palette group bounds')
            self.field(target+'.offsets',ptr,2*n)
            self.field(target+'.colors',v(0x26001c),max(v(ptr+2*i,2) for i in range(n))+8)
            self.bindings += [f'{target}.kind=1;{target}.offset={dest};{target}.count={n};']
        elif kind==32:
            dest=480+v(s,1); control=v(s+3,2); pending=v(s+5,1)
            if dest>510 or dest&1 or control not in (0x118,0x12) or pending>1: raise ValueError('Single palette bounds')
            self.bindings += [f'{target}.kind=2;{target}.offset={dest};{target}.color={v(s+1,2)};{target}.control_offset={control};{target}.pending_index={pending};']
        else: raise ValueError('Unsupported fire palette')
    def terminated(self, source, stride):
        for n in range(stride,513,stride):
            if self.value(source+n-stride,2)==65535:return n
        raise ValueError('Unterminated fire record table')
    def objects(self, target, selections, dense=False):
        v=self.value; root=v(v(0x260024)+60)&0xbfffff; shapes=v(v(0x260028)+60)
        shape_end=tile_end=0; tiles=None
        for velocity in sorted(selections):
            address=v(root+velocity)&0xffffff; frame=end=0
            while True:
                component=v(address+frame+2,2)
                for _ in range(2):
                    raw=v(address+component,2); shape=raw&32767; end=max(end,component+4)
                    if v(shapes+shape+1,1)==255:shape_end=max(shape_end,shape+2)
                    else:
                        for count in range(1,65):
                            if v(shapes+shape+2*count-2,1)&128:break
                        else:raise ValueError('Unterminated fire object shape')
                        shape_end=max(shape_end,shape+4*count+4); ptr=shapes+shape+2*count; tr=v(v(ptr))
                        if tiles not in (None,tr):raise ValueError('Inconsistent fire object tile root')
                        tiles=tr;tile_end=max(tile_end,16*(1+max(v(ptr+4+2*j,2) for j in range(count))))
                    if raw&32768:break
                    component+=4
                else:raise ValueError('Too many fire sprite components')
                frame+=4
                if v(address+frame-3,1)&128:break
                if frame>=256:raise ValueError('Unterminated fire animation')
            if dense:
                if not (64<=velocity<=400 and velocity%64<=16 and velocity%8==0):raise ValueError('Dense trajectory index')
                index=3*(velocity//64-1)+(velocity%64)//8
            else:index=velocity//4
            if index>=40:raise ValueError('Animation capacity')
            self.field(f'{target}.animations[{index}].data',address,max(end,frame))
            # field helper appends _size; animation uses size, not data_size.
            self.bindings[-1]=self.bindings[-1].replace('.data_size=', '.size=')
            self.bindings += [f'{target}.animations[{index}].frames={frame//4};']
        if tiles is None:raise ValueError('Missing object tiles')
        self.field(target+'.shapes',shapes,shape_end);self.field(target+'.tiles',tiles,tile_end)
    def terrain(self, record):
        v=self.value; root=v(0x360000)+record; delta=v(root+4,2);delta=delta if delta<32768 else delta-65536
        blocks=v(root)&0xffffff; width=4*v(blocks+delta,1);height=4*v(blocks+delta+1,1);source=v(root+14)&0xffffff;end=2*height
        if not width or not height:raise ValueError('Empty terrain map')
        for y in range(height):
            at=v(source+2*y,2);covered=0
            while covered<width:
                n=v(source+at,1)
                if not n or at>=65533:raise ValueError('Invalid terrain RLE')
                covered+=n;at+=3
            end=max(end,at)
        return source,end
    def collect(self):
        super().collect(); old=self.bindings;self.bindings=[];v=self.value
        self.palette('f->palette',0x10);self.field('sp->bounds',0x21ee9f,24)
        for i in range(9):
            self.asset(f'sd->indicators[{i}]',0x9c,4*(i+1));self.asset(f'sd->highlights[{i}]',0xac,4*i)
        for name,a,n in [('metadata',0x2197d7,5),('sprite_bases',0x21ee93,6),('hud_bases',0x21ee8d,6)]:self.field('tr->'+name,a,n)
        for i in range(6):
            self.palette(f'tr->palettes[{i}]',0x30+4*i)
            for k in range(3):self.asset(f'tr->hud[{i}][{k}]',0xac,4*v(0x21ee8d+i,1)+60*k)
        specs=[('course_selectors',0x20a62f,3),('ground_records',0x21ba11,60),('foreground_records',0x20a876,60),('frames',0x21ba65,64),('overrides',0x21ba4d,8),('green_records',0x21b93d,self.terminated(0x21b93d,6)),('pin_records',0x21b955,self.terminated(0x21b955,4)),('metadata',0x21984f,5)]
        for name,a,n in specs:self.field('g->'+name,a,n)
        for c in range(3):
            for k in range(2):self.palette(f'g->palettes[{c}][{k}]',4*(c+6+3*k))
        selectors={0,*self.data(0x20a62f,3)}
        for a,n,stride,offset in [(0x21ba11,60,4,0),(0x20a876,60,4,0),(0x21ba4d,8,2,0),(0x21b93d,specs[5][2],6,2)]:
            selectors.update(v(a+i+offset,2) for i in range(0,n,stride))
        self.golfer_asset_count=len(selectors)
        for i,selector in enumerate(sorted(selectors)):
            self.bindings += [f'ga[{i}].selector={selector};'];self.asset(f'ga[{i}].asset',0x6c,selector)
        self.bindings += [f'g->assets=ga;g->asset_count={len(selectors)};g->aim=a;']
        for row,table in enumerate((0x20acbf,0x20acc7,0x20accf,0x20acd7,0x20acdf)):
            for frame in range(4):self.asset(f'an->frames[{row}][{frame}]',0x6c,v(table+2*frame,2))
        self.asset('d[0].rim',0x6c,0x88)
        for i,sel in enumerate((0xa8,0xa4,0xa0,0x9c)):self.asset(f'd[0].green[{i}]',0x6c,sel)
        for name,a,n in [('eligible',0x21ba55,16),('depth_records',0x21b975,20),('clip_records',0x21b99d,116),('metadata',0x219854,5)]:self.field('d[0].'+name,a,n)
        self.bindings += ['d[0].clip_origin=56;d[0].animation=an;for(i=1;i<54;i++)d[i]=d[0];']
        for course in range(3):
            group=0x21bb4d+v(0x21bb4d+2*course,2)
            for hole in range(18):
                i=18*course+hole;address=group+v(group+2*hole,2);source=address+v(address+16,2)
                size=0
                while v(source+size,1)!=255:
                    size+=5
                    if size>=4096:raise ValueError('Unterminated feature list')
                self.field(f'd[{i}].features.features',source,size+1)
                self.bindings += [f'd[{i}].features.aim=a;']
                for name,delta in [('large',0),('close',12)]:
                    record=(v(v(0x36000c)+4*course+delta,2)+18*hole)&65535
                    source,n=self.terrain(record);self.field(f't[{i}].{name}.data',source,n)
                    self.bindings[-1]=self.bindings[-1].replace('.data_size=', '.size=')
                self.bindings += [f't[{i}].masks=shot_masks;t[{i}].masks_size=8;t[{i}].shapes=shot_terrain_shapes;t[{i}].shapes_size=2048;']
        self.objects('o[0]',{4*v(0x21ee93+i,1)+delta for i in range(6) for delta in (0,8,16)},True)
        self.objects('o[1]',{v(0x219851,2),*(v(0x21b955+i+2,2) for i in range(0,specs[6][2],4))})
        if v(0x219854,2)!=60 or {v(0x219856,2),v(0x20aa2b,2),v(0x20aaea,2)}!={16,20}:raise ValueError('Green object selections')
        self.objects('o[2]',{16,20})
        self.bindings += ['r->parent=f;r->spin=sp;r->spin_draw=sd;r->trajectory=tr;r->golfer=g;r->drawing=d;r->terrain=t;r->hole_count=54;r->trajectory_objects=o;r->pin_objects=o+1;r->green_objects=o+2;']
        self.fire_bindings=self.bindings;self.bindings=old

def export(rom, output):
    private(output);output.mkdir(parents=True,exist_ok=True)
    r=Resources(rom);r.collect();raw,offsets,ranges=r.pack();report=packed_file(output/'fire.pool',raw)
    lines=['/* PRIVATE GENERATED RESOURCES. Do not commit or distribute. */',f'static uint8_t tee_pool[{len(raw)}] __attribute__((aligned(2)));',f'static const fire_pack_spec fire_spec={{{len(raw)},{report["packed_bytes"]},{report["checksum"]}u}};']
    for name,blob in r.assembled.items():lines+=base.array(name,blob)
    lines += [f'#define {name} (tee_pool+{offset})' for name,offset in offsets.items()]
    lines += ['static const ntm_hole_geometry tee_geometries[54]={']+r.geometries+['};']
    for signature,body in [
        ('tee_resources(ntm_tee_setup_resources *r,ntm_tee_object_assets *o,ntm_hole_prepare_resources *h)',r.bindings+r.hole_bindings),
        ('shot_resources(ntm_shot_resources *s,ntm_shot_prepare_resources *p,ntm_terrain_resources *t,ntm_shot_banner_resources *b,ntm_tee_object_assets *o,ntm_shot_palette_job j[2])',r.shot_bindings),
        ('aim_resources(ntm_shot_aim_application_resources *r,ntm_shot_aim_resources *a,ntm_aim_world_resources w[54],ntm_shot_hud_resources *h,ntm_shot_aim_setup_resources *s,ntm_tee_object_assets o[54])',r.aim_bindings),
        ('open_resources(ntm_shot_open_resources *r,ntm_shot_scene_object_resources *s,ntm_shot_club_resources *c,ntm_shot_marker_resources *m,ntm_tee_object_assets *o,const ntm_shot_aim_resources *a)',r.open_bindings),
        ('fire_resources(ntm_shot_fire_application_resources *r,ntm_shot_fire_resources *f,ntm_shot_spin_resources *sp,ntm_shot_spin_draw_resources *sd,ntm_shot_trajectory_resources *tr,ntm_shot_golfer_resources *g,ntm_shot_golfer_asset *ga,ntm_shot_golfer_animation_resources *an,ntm_shot_golfer_drawing_resources d[54],ntm_terrain_resources t[54],ntm_tee_object_assets o[3],const ntm_shot_aim_resources *a)', ['unsigned i;']+r.fire_bindings)]:
        lines += ['static void '+signature+'{']+body+['}']
    lines += [f'#define FIRE_GOLFER_ASSET_COUNT {r.golfer_asset_count}']
    (output/'fire_assets.inc').write_text('\n'.join(lines)+'\n')
    report.update(holes=54,terrain_maps=108,feature_lists=54,bounded_spans=len(r.spans),merged_ranges=ranges,assembled_palette_bytes=sum(map(len,r.assembled.values())),spans={name:dict(source=a,size=n,offset=offsets[name]) for name,(a,n) in r.spans.items()})
    (output/'fire_assets.json').write_text(json.dumps(report,indent=2)+'\n')
    return {k:v for k,v in report.items() if k!='spans'}

def intro(source,output):
    private(source);private(output);raw=bytearray();offsets={};arrays=[]
    def replace(m):
        name,size,values=m.groups();b=bytes(int(v) for v in values.split(',') if v.strip())
        if len(b)!=int(size):raise ValueError('Bad intro array')
        if b not in offsets:
            if len(raw)&1:raw.append(0)
            offsets[b]=len(raw);raw.extend(b)
        arrays.append((name,len(b),offsets[b]))
        return f'#define {name} (*(const uint8_t (*)[{len(b)}])(intro_unpacked+{offsets[b]}))'
    body=re.sub(r'static const uint8_t (\w+)\[(\d+)\]=\{\s*([\d,\s]+)\};',replace,source.read_text())
    if not arrays:raise ValueError('No intro arrays')
    report=packed_file(output/'intro.pool',raw);report['bindings']=arrays
    lines=['/* PRIVATE GENERATED INTRO. Do not commit or distribute. */',f'static uint8_t intro_unpacked[{len(raw)}] __attribute__((aligned(2)));',f'static const fire_pack_spec intro_spec={{{len(raw)},{report["packed_bytes"]},{report["checksum"]}u}};',body]
    lines += [f'#undef {name}' for name,_,_ in arrays]
    (output/'fire_intro.inc').write_text('\n'.join(lines)+'\n');(output/'fire_intro.json').write_text(json.dumps(report,indent=2)+'\n')
    return {k:v for k,v in report.items() if k!='bindings'}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--rom',type=Path,default=ROOT/'private-inputs/neo-turf-masters.ngc');p.add_argument('--output',type=Path,default=ROOT/'build/native-fire');a=p.parse_args()
    print(json.dumps({'fire':export(a.rom.read_bytes(),a.output),'intro':intro(a.output/'intro_assets.inc',a.output)},indent=2))
