# SPDX-License-Identifier: GPL-2.0-or-later
"""Resolve only the first-tee banner's four original animation selections."""
from screen_probe import load

def resolve(load,read):
    index=load(0x2197c8,2);assert index==56
    root=load(load(0x260024,4)+index,4)&0xbfffff
    shapes=load(load(0x260028,4)+index,4)
    records={};shape_end=0;tile_end=0;tiles=None
    for velocity in (60,148,152,156):
        pointer=load(root+velocity,4)&0xffffff;frame=0;end=0
        while True:
            component=load(pointer+frame+2,2)
            for k in range(2):
                raw=load(pointer+component,2);offset=raw&32767;end=max(end,component+4)
                if load(shapes+offset+1,1)==255:shape_end=max(shape_end,offset+2)
                else:
                    count=0
                    while True:
                        count+=1
                        if load(shapes+offset+2*count-2,1)&128:break
                        assert count<64
                    shape_end=max(shape_end,offset+4*count+4)
                    tr=load(load(shapes+offset+2*count,4),4)
                    assert tiles in (None,tr);tiles=tr
                    tile_end=max(tile_end,16*(max(load(shapes+offset+2*count+4+2*j,2) for j in range(count))+1))
                if raw&32768:break
                component+=4
            else:raise AssertionError('more than two components')
            frame+=4
            if load(pointer+frame-3,1)&128:break
            assert frame<256
        records[velocity]=(read(pointer,max(end,frame)),frame//4)
    assert tiles is not None
    return dict(index=index,animations=records,shape_address=shapes,tile_address=tiles,shapes=read(shapes,shape_end),tiles=read(tiles,tile_end))
