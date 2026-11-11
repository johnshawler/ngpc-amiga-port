#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Strict read-only standard DD OFS/FFS executable recovery and equality check."""
import argparse,json,struct
from pathlib import Path

def recover(image,name):
    if len(image)!=901120 or image[:4] not in (b'DOS\0',b'DOS\1'):
        raise ValueError('Expected standard880KiB DD OFS/FFS image')
    ffs=image[3]==1
    def block(index,kind=None):
        if not 2<=index<1760:raise ValueError('Invalid disk block')
        raw=image[index*512:(index+1)*512];v=struct.unpack('>128I',raw)
        if kind is not None and (v[0]!=kind or sum(v)&0xffffffff):raise ValueError('Bad metadata block or checksum')
        return raw,v
    _,root=block(880,2)
    if root[-1]!=1:raise ValueError('Not a root block')
    matches=[];seen=set()
    for first in root[6:78]:
        index=first
        while index:
            if index in seen:raise ValueError('Directory hash loop')
            seen.add(index);raw,v=block(index,2)
            length=raw[432]
            if length>30:raise ValueError('Invalid file name')
            if raw[433:433+length].lower()==name.encode('ascii').lower():matches.append((index,v))
            index=v[124]
    if len(matches)!=1:raise ValueError('Executable missing or repeated in directory')
    header,v=matches[0];size=v[81]
    if v[-1]!=0xfffffffd or v[125]!=880 or size>len(image):raise ValueError('Invalid executable header')
    indices=[];headers=set();index=header
    while index:
        if index in headers:raise ValueError('Extension block loop')
        headers.add(index);_,v=block(index,2 if index==header else 16)
        n=v[2]
        if not 0<n<=72 or v[-1]!=0xfffffffd:raise ValueError('Invalid data block table')
        indices.extend(reversed(v[78-n:78]));index=v[126]
    payload=512 if ffs else 488
    if len(indices)!=(size+payload-1)//payload or len(set(indices))!=len(indices):raise ValueError('Missing or repeated executable blocks')
    output=bytearray()
    for sequence,index in enumerate(indices,1):
        raw,v=block(index,None if ffs else 8)
        if ffs:output.extend(raw)
        else:
            expected=min(488,size-len(output))
            next_index=indices[sequence] if sequence<len(indices) else 0
            if (v[1],v[2],v[3],v[4])!=(header,sequence,expected,next_index):raise ValueError('Invalid OFS data chain')
            output.extend(raw[24:24+expected])
    return bytes(output[:size]),{'filesystem':'FFS' if ffs else 'OFS','image_bytes':len(image),'executable_bytes':size,'data_blocks':len(indices),'file_header_blocks':len(headers)}

def verify(image,executable):
    recovered,report=recover(image.read_bytes(),executable.name)
    if recovered!=executable.read_bytes():raise ValueError('Recovered executable differs from source')
    report['byte_identical']=True;return report
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path);p.add_argument('executable',type=Path);a=p.parse_args();print(json.dumps(verify(a.image,a.executable),indent=2))
