#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Privately export all54 aim worlds plus preceding exact scene resources.

Source overlaps share storage without changing identities or signed origins.
Own bounded LZ encoding is unpacked once into caller-owned storage at startup.
"""
import argparse,hashlib,importlib.util,json
from pathlib import Path
from aim_pack import pack,unpack
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('shot_export',ROOT/'tools/export-shot-assets.py')
shot=importlib.util.module_from_spec(spec);spec.loader.exec_module(shot)
class Resources(shot.Resources):
    def collect(self):
        super().collect();old=self.bindings;self.bindings=[];v=self.value
        def signed(n):return n if n<32768 else n-65536
        for course in range(3):
            for hole in range(18):
                i=18*course+hole;w=f'w[{i}]'
                offset=v(v(0x36000c)+4*course,2)+18*hole
                record=v(0x360000)+offset;blocks=v(record);layout=blocks+signed(v(record+4,2))+2
                indices=blocks+signed(v(record+6,2));tiles=v(record+8);terrain=v(record+14)
                command=v(0x360008)+signed(v(v(0x360004)+v(record+3,1)*2,2))
                for name,address,size,origin in [('layout',layout-32768,65568,32768),('blocks',blocks-32768,65568,32768),('tile_indices',indices,2+2*v(indices,1),0),('tiles',tiles,16*(1+max([v(indices+2+2*j,2) for j in range(v(indices,1))]+[0])),0),('commands',command,43,0),('colors',v(0x26001c),65536,0),('mono',v(0x260018),1024,0)]:
                    label=self.span(f'aim_world_{i}_{name}',address&0xffffff,size)
                    self.bindings += [f'{w}.{name}.data={label};{w}.{name}.size={size};{w}.{name}.origin={origin};']
                self.bindings += [f'{w}.layout_tag={layout};{w}.blocks_tag={blocks};{w}.terrain_tag={terrain};{w}.tiles_tag={tiles};{w}.width={v(layout-2,1)};{w}.height={v(layout-1,1)};']
                self.asset(f's->panorama[{course}][{hole}]',0xa0+4*course,4*hole)
                root=v(v(0x260024)+68+4*course)&0xbfffff;shapes=v(v(0x260028)+68+4*course)
                address=v(root+4*hole)&0xffffff;frame=end=shape_end=tile_end=0;tile_root=None
                while True:
                    component=v(address+frame+2,2)
                    for _ in range(2):
                        raw=v(address+component,2);shape=raw&32767;end=max(end,component+4)
                        if v(shapes+shape+1,1)==255:shape_end=max(shape_end,shape+2)
                        else:
                            for count in range(1,65):
                                if v(shapes+shape+2*count-2,1)&128:break
                            else:raise ValueError('Unterminated aim object shape')
                            shape_end=max(shape_end,shape+4*count+4);ptr=shapes+shape+2*count;tr=v(v(ptr))
                            if tile_root not in (None,tr):raise ValueError('Inconsistent aim object tile root')
                            tile_root=tr;tile_end=max(tile_end,16*(1+max(v(ptr+4+2*j,2) for j in range(count))))
                        if raw&32768:break
                        component+=4
                    else:raise ValueError('More than two aim components')
                    frame+=4
                    if v(address+frame-3,1)&128:break
                    if frame>=256:raise ValueError('Unterminated aim animation')
                label=self.span(f'aim_object_{i}_animation',address,max(end,frame))
                self.bindings += [f'o[{i}].animations[{hole}].data={label};o[{i}].animations[{hole}].size={max(end,frame)};o[{i}].animations[{hole}].frames={frame//4};']
                for name,address,size in [('shapes',shapes,shape_end),('tiles',tile_root,tile_end)]:
                    label=self.span(f'aim_object_{i}_{name}',address,size)
                    self.bindings += [f'o[{i}].{name}={label};o[{i}].{name}_size={size};']
                self.bindings += [f'r->objects[{i}]=o+{i};']
        for field,address,size in [('a->angles',0x21e67f,2050)]:
            label=self.span('aim_angles',address,size);self.bindings += [f'{field}={label};{field}_size={size};']
        for name,count,base,prefix,size in [('powers',12,0x21eeb7,0,52),('minimums',6,0x21f187,128,256)]:
            for i in range(count):
                label=self.span(f'aim_{name}_{i}',v(base+4*i)-prefix,size)
                sizefield='power_sizes' if name=='powers' else 'minimum_sizes'
                self.bindings += [f'a->{name}[{i}]={label};a->{sizefield}[{i}]={size};']
        self.asset('h->font',0xb4,0,True);self.asset('h->header',0x68,0x3c);self.asset('h->label',0x9c,20)
        for i in range(17):self.asset(f'h->wind[{i}]',0x9c,4*(28+i))
        self.bindings += ['h->label_id=4;']
        label=self.span('aim_panorama_metadata',0x219769,5)
        self.bindings += [f's->metadata={label};s->metadata_size=5;']
        packed=v(0x2198f3+12);desc=packed>>8
        if packed&255!=4:raise ValueError('Unsupported aim palette')
        count=v(desc,1);source=v(desc+2)
        offsets=self.span('aim_palette_offsets',source,2*count)
        size=max(v(source+2*j,2) for j in range(count))+8
        colors=self.span('aim_palette_colors',v(0x26001c),size)
        self.bindings += [f'r->palette.kind=1;r->palette.count={count};r->palette.offset={8*v(desc+1,1)};r->palette.offsets={offsets};r->palette.offsets_size={2*count};r->palette.colors={colors};r->palette.colors_size={size};', 'r->aim=a;r->worlds=w;r->world_count=54;r->hud=h;r->setup=s;']
        self.aim_bindings=self.bindings;self.bindings=[]
        for name,address,size,target in [('scene_metadata',0x219755,55,'s->metadata'),('power_scale',0x2217bb-128,256,'s->power_scale'),('remainders',0x21b82e,10,'s->remainders'),('quarter',0x21d67b,4100,'m->quarter'),('marker_metadata',0x21975f,10,'m->metadata')]:
            label=self.span('aim_open_'+name,address,size)
            self.bindings += [f'{target}={label};{target}_size={size};']
        for i in range(12):
            label=self.span(f'aim_club_power_{i}',v(0x21eeb7+4*i),56)
            self.bindings += [f'c->powers[{i}].data={label};c->powers[{i}].size=56;']
            root=v(0x221838+4*i)
            for terrain in range(15):
                for name,delta in [('lies',0),('overflow_lies',768)]:
                    label=self.span(f'aim_club_{name}_{i}_{terrain}',v(root+4*terrain)+delta,42)
                    self.bindings += [f'c->{name}[{i}][{terrain}].data={label};c->{name}[{i}][{terrain}].size=42;']
        for i in range(23):self.asset(f'c->labels[{i}]',0xb4,v(0x21b800+i,1))
        for i in range(11):self.asset(f'c->digits[{i}]',0xb4,4*(i+20))
        for i in range(19):self.asset(f'c->lie_icons[{i}]',0x9c,288+4*i)
        for i in range(23):self.asset(f'c->highlights[{i}]',0xac,36+v(0x21b817+i,1))
        root=v(v(0x260024)+56)&0xbfffff;shapes=v(v(0x260028)+56)
        shape_end=tile_end=0;tile_root=None
        for velocity in (8,44,40,48,120,52,56,128,124,132):
            address=v(root+velocity)&0xffffff;frame=end=0
            while True:
                component=v(address+frame+2,2)
                for _ in range(2):
                    raw=v(address+component,2);shape=raw&32767;end=max(end,component+4)
                    if v(shapes+shape+1,1)==255:shape_end=max(shape_end,shape+2)
                    else:
                        for count in range(1,65):
                            if v(shapes+shape+2*count-2,1)&128:break
                        else:raise ValueError('Unterminated scene object shape')
                        shape_end=max(shape_end,shape+4*count+4);ptr=shapes+shape+2*count;tr=v(v(ptr))
                        if tile_root not in (None,tr):raise ValueError('Inconsistent scene object tile root')
                        tile_root=tr;tile_end=max(tile_end,16*(1+max(v(ptr+4+2*j,2) for j in range(count))))
                    if raw&32768:break
                    component+=4
                else:raise ValueError('More than two scene components')
                frame+=4
                if v(address+frame-3,1)&128:break
                if frame>=256:raise ValueError('Unterminated scene animation')
            label=self.span(f'aim_scene_animation_{velocity}',address,max(end,frame))
            self.bindings += [f'o->animations[{velocity//4}].data={label};o->animations[{velocity//4}].size={max(end,frame)};o->animations[{velocity//4}].frames={frame//4};']
        for name,address,size in [('shapes',shapes,shape_end),('tiles',tile_root,tile_end)]:
            label=self.span('aim_scene_'+name,address,size)
            self.bindings += [f'o->{name}={label};o->{name}_size={size};']
        self.bindings += ['c->aim=a;c->scene=s;r->scene=s;r->club=c;r->markers=m;r->objects=o;']
        self.open_bindings=self.bindings;self.bindings=old

def array(name,data):
    return [f'static const uint8_t {name}[{len(data)}] __attribute__((aligned(2)))={{']+[','.join(map(str,data[i:i+24]))+',' for i in range(0,len(data),24)]+['};']
def export(rom,output):
    output=output.resolve()
    if not output.is_relative_to((ROOT/'build').resolve()):raise ValueError('Private output must stay under ignored build/')
    r=Resources(rom);r.collect();raw,offsets,ranges=r.pack();packed=pack(raw)
    if unpack(packed,len(raw))!=raw:raise ValueError('Pack round-trip failed')
    lines=['/* PRIVATE GENERATED CARTRIDGE ASSETS. Do not commit or distribute. */']+array('aim_packed',packed)
    lines += [f'static uint8_t tee_pool[{len(raw)}] __attribute__((aligned(2)));', 'static int aim_assets_unpack(void){return aim_unpack(aim_packed,sizeof aim_packed,tee_pool,sizeof tee_pool);}']
    for name,blob in r.assembled.items():lines+=array(name,blob)
    lines += [f'#define {name} (tee_pool+{offset})' for name,offset in offsets.items()]
    lines += ['static const ntm_hole_geometry tee_geometries[54]={']+r.geometries+['};']
    lines += ['static void tee_resources(ntm_tee_setup_resources *r,ntm_tee_object_assets *o,ntm_hole_prepare_resources *h){']+r.bindings+r.hole_bindings+['}']
    lines += ['static void shot_resources(ntm_shot_resources *s,ntm_shot_prepare_resources *p,ntm_terrain_resources *t,ntm_shot_banner_resources *b,ntm_tee_object_assets *o,ntm_shot_palette_job j[2]){']+r.shot_bindings+['}']
    lines += ['static void aim_resources(ntm_shot_aim_application_resources *r,ntm_shot_aim_resources *a,ntm_aim_world_resources w[54],ntm_shot_hud_resources *h,ntm_shot_aim_setup_resources *s,ntm_tee_object_assets o[54]){']+r.aim_bindings+['}']
    lines += ['#ifdef NTM_AIM_OPEN_TARGET','static void open_resources(ntm_shot_open_resources *r,ntm_shot_scene_object_resources *s,ntm_shot_club_resources *c,ntm_shot_marker_resources *m,ntm_tee_object_assets *o,const ntm_shot_aim_resources *a){']+r.open_bindings+['}','#endif']
    output.parent.mkdir(parents=True,exist_ok=True);output.write_text('\n'.join(lines)+'\n')
    output.with_suffix('.raw').write_bytes(raw);output.with_suffix('.lz').write_bytes(packed)
    report={'holes':54,'worlds':54,'panoramas':54,'aim_objects':54,'scene_animations':10,'club_power_tables':12,'lie_tables':360,'club_labels':23,'club_digits':11,'lie_icons':19,'club_highlights':23,'bounded_spans':len(r.spans),'raw_span_bytes':sum(n for _,n in r.spans.values()),'deduplicated_pool_bytes':len(raw),'packed_pool_bytes':len(packed),'startup_output_bytes':len(raw),'packed_sha256':hashlib.sha256(packed).hexdigest(),'unpacked_sha256':hashlib.sha256(raw).hexdigest(),'independent_python_roundtrip':True,'merged_ranges':ranges}
    output.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n');return report
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--rom',type=Path,default=ROOT/'private-inputs/neo-turf-masters.ngc');p.add_argument('--output',type=Path,default=ROOT/'build/native-aim/aim_assets.inc');a=p.parse_args();print(json.dumps(export(a.rom.read_bytes(),a.output),indent=2))
