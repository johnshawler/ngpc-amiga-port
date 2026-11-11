# SPDX-License-Identifier: GPL-2.0-or-later
"""Resolve bounded initial-aim objects without embedding cartridge assets."""
from screen_probe import load

SCENE_SELECTIONS={1:8,2:44,3:40,4:48,6:120,7:52,8:56,9:128,10:124,11:132}

def _resolve_index(load,read,index,selections):
    root=load(load(0x260024,4)+index,4)&0xbfffff
    shapes=load(load(0x260028,4)+index,4)
    records={};shape_end=0;tile_end=0;tiles=None
    for velocity in selections:
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

def resolve(load,read,course=0,hole=0):
    """The existing tag5 course/hole marker; caller owns the returned spans."""
    assert 0<=course<3 and 0<=hole<18
    return _resolve_index(load,read,68+4*course,(4*hole,))

def resolve_scene(load,read):
    """Exact descriptor selections used by209f5c/20a170/20a214/20aee1.

    Tag1 can also be registered in slot17 for the other player's ball. The
    resolved resource is independent of the object's slot and screen position.
    """
    for tag,velocity in SCENE_SELECTIONS.items():
        descriptor=0x219750+5*tag
        assert load(descriptor,2)==56 and load(descriptor+2,2)==velocity
    return _resolve_index(load,read,56,SCENE_SELECTIONS.values())
