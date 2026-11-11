#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Losslessly pack generated intro byte arrays once, retaining all bindings."""
import argparse,hashlib,json,re
from pathlib import Path
from aim_pack import pack,unpack
ROOT=Path(__file__).resolve().parents[1]
def generate(source,destination):
    for p in (source,destination):
        if not p.resolve().is_relative_to((ROOT/'build').resolve()):raise ValueError('Private files must stay under ignored build/')
    raw=bytearray();offsets={};arrays=[]
    def replace(m):
        name,size,values=m.groups();b=bytes(int(v) for v in values.split(',') if v.strip())
        if len(b)!=int(size):raise ValueError('Bad generated array')
        if b not in offsets:
            if len(raw)&1:raw.append(0)
            offsets[b]=len(raw);raw.extend(b)
        arrays.append((name,len(b),offsets[b]))
        return f'#define {name} (*(const uint8_t (*)[{len(b)}])(intro_unpacked+{offsets[b]}))'
    body=re.sub(r'static const uint8_t (\w+)\[(\d+)\]=\{\s*([\d,\s]+)\};',replace,source.read_text())
    if not arrays:raise ValueError('No generated arrays found')
    encoded=pack(raw)
    if unpack(encoded,len(raw))!=raw:raise ValueError('Lossless equality failed')
    lines=['/* PRIVATE GENERATED STARTUP RESOURCES. Do not commit or distribute. */',f'static uint8_t intro_unpacked[{len(raw)}] __attribute__((aligned(2)));',f'static const uint8_t intro_packed[{len(encoded)}]={{']
    lines += [','.join(map(str,encoded[i:i+24]))+',' for i in range(0,len(encoded),24)]+['};','static int intro_assets_unpack(void){return aim_unpack(intro_packed,sizeof intro_packed,intro_unpacked,sizeof intro_unpacked);}',body]
    lines += [f'#undef {name}' for name,_,_ in arrays]
    destination.write_text('\n'.join(lines)+'\n');destination.with_suffix('.raw').write_bytes(raw);destination.with_suffix('.lz').write_bytes(encoded)
    report={'arrays':len(arrays),'source_bytes':sum(n for _,n,_ in arrays),'unpacked_bytes':len(raw),'packed_bytes':len(encoded),'unpacked_sha256':hashlib.sha256(raw).hexdigest(),'packed_sha256':hashlib.sha256(encoded).hexdigest(),'bindings':arrays}
    destination.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='bindings'},indent=2))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('destination',type=Path);a=p.parse_args();generate(a.source,a.destination)
