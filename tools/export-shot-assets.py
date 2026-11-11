#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Resolve the bounded first-tee banner and all54 overview resources privately.

No CPU emulation or bundled cartridge. Every input span is bounds checked and
shared overlaps retain original indices. Output must stay in ignored build/.
"""
import argparse
import importlib.util
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('tee_export',ROOT/'tools/export-tee-assets.py')
tee=importlib.util.module_from_spec(spec);spec.loader.exec_module(tee)

class Resources(tee.Resources):
    def collect(self):
        super().collect()
        self.overview_bytes=len(self.pack()[0])
        old=self.bindings;self.bindings=[]
        v=self.value
        self.asset('b->terrain',12,v(0x21b85c+8,2))
        for i in range(6):self.asset(f'b->characters[{i}]',12,4*(i+1))
        for i in range(2):self.asset(f'b->players[{i}]',12,4*(i+7))
        self.asset('b->tee_shot',12,36);self.asset('b->portrait',0x68,88)
        for i in range(2):self.asset(f'b->hints[{i}]',0x78+4*i,0x3c)
        md=self.span('shot_asset_metadata',0x2197c8,5)
        self.bindings += [f'b->metadata={md};b->metadata_size=5;']
        for i,command in enumerate((0x64,0x68)):
            packed=v(0x2198f3+command);kind,source=packed&255,packed>>8;job=f'b->palettes[{i}]'
            if kind==4:
                count,dest=v(source,1),v(source+1,1)*8
                if not 0<count<=64 or dest+count*8>512:raise ValueError('Invalid banner palette groups')
                ptr=v(source+2);size=max(v(ptr+2*j,2) for j in range(count))+8
                offsets=self.span(f'shot_offsets_{i}',ptr,count*2);colors=self.span(f'shot_colors_{i}',v(0x26001c),size)
                self.bindings += [f'{job}.kind=1;{job}.count={count};{job}.offset={dest};{job}.offsets={offsets};{job}.offsets_size={count*2};{job}.colors={colors};{job}.colors_size={size};']
            elif kind==32:
                dest,control,pending=480+v(source,1),v(source+3,2),v(source+5,1)
                if dest>510 or dest&1 or control not in (0x118,0x12) or pending>1:raise ValueError('Invalid banner single palette')
                self.bindings += [f'{job}.kind=2;{job}.offset={dest};{job}.color={v(source+1,2)};{job}.control_offset={control};{job}.pending_index={pending};']
            else:raise ValueError('Unsupported banner palette')
        for name,address,size,field in [('traits',0x21ee81-128,256,'p->traits'),('wind_low',0x2050b9,16,'p->wind_low'),('wind_step',0x2050d9,16,'p->wind_step'),('wind_limit',0x2050a7,18,'p->wind_limit'),('masks',0x223104,8,'t->masks'),('terrain_shapes',0x22310c,2048,'t->shapes')]:
            label=self.span('shot_'+name,address,size)
            self.bindings += [f'{field}={label};{field}_size={size};']
        # The true first shot returns terrain4 at the prepared tee before map
        # lookup. No fabricated resource for still-uninitialized original5730.
        root=v(v(0x260024)+56)&0xbfffff;shapes=v(v(0x260028)+56)
        if v(0x2197c8,2)!=56:raise ValueError('Unsupported banner object')
        shape_end=tile_end=0;tiles=None
        for velocity in (60,148,152,156):
            address=v(root+velocity)&0xffffff;frame=end=0
            while True:
                component=v(address+frame+2,2)
                for _ in range(2):
                    raw=v(address+component,2);shape=raw&32767;end=max(end,component+4)
                    if v(shapes+shape+1,1)==255:shape_end=max(shape_end,shape+2)
                    else:
                        for count in range(1,65):
                            if v(shapes+shape+2*count-2,1)&128:break
                        else:raise ValueError('Unterminated banner shape')
                        shape_end=max(shape_end,shape+4*count+4);ptr=shapes+shape+2*count;tr=v(v(ptr))
                        if tiles not in (None,tr):raise ValueError('Inconsistent banner tile root')
                        tiles=tr;tile_end=max(tile_end,16*(1+max(v(ptr+4+2*j,2) for j in range(count))))
                    if raw&32768:break
                    component+=4
                else:raise ValueError('More than two banner components')
                frame+=4
                if v(address+frame-3,1)&128:break
                if frame>=256:raise ValueError('Unterminated banner animation')
            label=self.span(f'shot_animation_{velocity}',address,max(end,frame))
            self.bindings += [f'o->animations[{velocity//4}].data={label};o->animations[{velocity//4}].size={max(end,frame)};o->animations[{velocity//4}].frames={frame//4};']
        for name,address,size in [('shapes',shapes,shape_end),('tiles',tiles,tile_end)]:
            label=self.span('shot_object_'+name,address,size);self.bindings += [f'o->{name}={label};o->{name}_size={size};']
        blend=self.span('shot_blend',0x20319d,512)
        self.assembled={}
        for i,command in enumerate((0x6c,0x70)):
            packed=v(0x2198f3+command);kind,source=packed&255,packed>>8;job=f'j[{i}]'
            self.bindings += [f'{job}.kind={kind};{job}.blend={blend};{job}.blend_size=512;']
            if kind==24:
                count=v(source,1)
                if not 0<count<=64 or 8*v(source+1,1)+8*count>512:raise ValueError('Invalid animated palette groups')
                for field,offset in [('source',7),('target',3)]:
                    table=v(source+offset);label=f'shot_fade_{i}_{field}'
                    self.assembled[label]=b''.join(self.data(v(0x26001c)+v(table+2*k,2),8) for k in range(count))
                    self.bindings += [f'{job}.{field}={label};{job}.{field}_size={count*8};']
                self.bindings += [f'{job}.offset={8*v(source+1,1)};{job}.count={count};{job}.speed={v(source+2,1)};']
            elif kind==40:
                if v(source,1)>30 or v(source,1)&1 or v(source+8,1)>1 or v(source+6,2) not in (0x118,0x12):raise ValueError('Invalid animated single palette')
                self.bindings += [f'{job}.speed={v(source+1,1)};{job}.pending_index={v(source+8,1)};{job}.offset={480+v(source,1)};{job}.source_color={v(source+4,2)};{job}.target_color={v(source+2,2)};{job}.control_offset={v(source+6,2)};']
            else:raise ValueError('Unsupported animated palette')
        self.bindings += ['s->preparation=p;s->terrain=t;s->banner=b;s->objects=o;s->exit_palettes=j;']
        self.shot_bindings=self.bindings;self.bindings=old

def export(rom,output):
    output=output.resolve()
    if not output.is_relative_to((ROOT/'build').resolve()):raise ValueError('Private generated assets must stay under ignored build/')
    r=Resources(rom);r.collect();data,offsets,ranges=r.pack()
    lines=['/* PRIVATE GENERATED CARTRIDGE ASSETS. Do not commit or distribute. */']
    for label,blob in [('tee_pool',data),*r.assembled.items()]:
        lines += [f'static const uint8_t {label}[{len(blob)}] __attribute__((aligned(2)))={{']
        lines += [','.join(map(str,blob[at:at+24]))+',' for at in range(0,len(blob),24)]+['};']
    lines += [f'#define {name} (tee_pool+{offset})' for name,offset in offsets.items()]
    lines += ['static const ntm_hole_geometry tee_geometries[54]={']+r.geometries+['};']
    lines += ['static void tee_resources(ntm_tee_setup_resources *r,ntm_tee_object_assets *o,ntm_hole_prepare_resources *h) {']+r.bindings+r.hole_bindings+['}']
    lines += ['static void shot_resources(ntm_shot_resources *s,ntm_shot_prepare_resources *p,ntm_terrain_resources *t,ntm_shot_banner_resources *b,ntm_tee_object_assets *o,ntm_shot_palette_job j[2]) {']+r.shot_bindings+['}']
    output.parent.mkdir(parents=True,exist_ok=True);output.write_text('\n'.join(lines)+'\n')
    report={'holes':54,'overview_views':162,'overview_animations':8,'banner_animations':4,'banner_velocities':[60,148,152,156],'bounded_spans':len(r.spans),'raw_span_bytes':sum(n for _,n in r.spans.values()),'deduplicated_pool_bytes':len(data),'merged_ranges':ranges,'assembled_palette_bytes':sum(map(len,r.assembled.values())),'overview_only_pool_bytes':r.overview_bytes,'added_resource_bytes':len(data)+sum(map(len,r.assembled.values()))-r.overview_bytes,'terrain_map_unread_at_actual_tee':True}
    output.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n');return report
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--rom',type=Path,default=ROOT/'private-inputs/neo-turf-masters.ngc');p.add_argument('--output',type=Path,default=ROOT/'build/native-shot/shot_assets.inc');a=p.parse_args();print(json.dumps(export(a.rom.read_bytes(),a.output),indent=2))
