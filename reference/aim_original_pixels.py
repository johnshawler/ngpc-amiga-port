#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Render the final independent original state using its unchanged GPU code.

Called only AFTER exact replay has finished. No Amiga snapshots are copied into
this renderer and no CPU instructions execute. Two source frames settle its real
register latches. Output stays with private captures, never source control.
"""
import ctypes as C,subprocess
from PIL import Image

def verify(core,load,root,directory):
    path=root/'reference/build/screen-probe/mednafen_ngp_libretro.so'
    symbols={p[2]:int(p[0],16) for line in subprocess.check_output(['nm',str(path)],text=True).splitlines() if len(p:=line.split())==3}
    for name in ('retro_reference_life_step','NGPGfx','ngpgfx_hint','ngpgfx_draw'):assert name in symbols,name
    base=C.cast(core.retro_reference_life_step,C.c_void_p).value-symbols['retro_reference_life_step']
    gfx=C.c_void_p.from_address(base+symbols['NGPGfx']).value
    hint=C.CFUNCTYPE(C.c_bool,C.c_void_p)(base+symbols['ngpgfx_hint'])
    draw=C.CFUNCTYPE(C.c_bool,C.c_void_p,C.c_void_p,C.c_bool)(base+symbols['ngpgfx_draw'])
    class Surface(C.Structure):
        _fields_=[('pixels',C.c_void_p)]+[(n,C.c_int32) for n in ('width','height','pitch','depth')]
    pixels=(C.c_uint16*(160*152))();surface=Surface(C.addressof(pixels),160,152,160,16)
    assert load(0x6fd4,4)==0x200471,'Static opening requires removed banner raster callback'
    period=load(0x8006,1);assert period==198
    rows=set()
    for _ in range(2*(period+1)):
        rows.add(load(0x8009,1));hint(gfx);draw(gfx,C.byref(surface),False)
    assert rows.issuperset(range(152))
    rgb=[]
    for v in pixels:
        # OriginalColorMap quantizes4-bit source channels toRGB565; each of the
        #16 possible values is unique, allowing exact inversion to4-bit RGB.
        r,g,b=v>>11,(v>>5)&63,v&31
        rn,gn,bn=(r*15+15)//31,(g*15+31)//63,(b*15+15)//31
        assert ((rn*17>>3)<<11)|((gn*17>>2)<<5)|(bn*17>>3)==v
        rgb.append((rn*17,gn*17,bn*17))
    original=Image.new('RGB',(160,152));original.putdata(rgb)
    original.save(directory/'original-opening.png')
    target=Image.open(directory/'screen.finished.ppm').convert('RGB')
    # Explicit PAL PUAE geometry:320×256 bitmap is horizontal2×,40/18 border;
    # the160×152 NGPC window is at bitmap(80,52). This is not image matching.
    assert target.size==(720,287),target.size
    crop=target.crop((200,70,520,222));crop.save(directory/'amiga-opening.png')
    actual=list(crop.getdata());differences=[]
    for y in range(152):
        for x in range(160):
            for repeat in range(2):
                at=y*320+x*2+repeat
                if actual[at]!=rgb[y*160+x]:differences.append((x,y,repeat,actual[at],rgb[y*160+x]))
    report={'original_renderer':'unchanged ngpgfx_hint/ngpgfx_draw from independently replayed original state','source_pixels':24320,'amiga_pixels':48640,'differences':len(differences),'first_differences':differences[:8],'panorama_and_world_included':True}
    assert not differences,report
    report['result']='PASS';return report
