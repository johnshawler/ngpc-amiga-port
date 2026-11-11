#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Verify every captured bitplane/palette/RGB pixel with immutable row scroll."""
import argparse,json,re,struct
from pathlib import Path
import selector_verify as base
# Reuse the independent per-pixel depth oracle with source scroll per row.
# This is a standalone copy, so legacy verification semantics do not change.
def expected(ram, w):
    tiles=ram[w[22]:w[22]+8192];maps=ram[w[23]:w[23]+4096]
    sprite=ram[w[24]:w[24]+256];pal=ram[w[25]:w[25]+64]
    sx0,sx1,sy0,sy1,spx,spy,wx,wy,ww,wh,swap,bg,out=ram[w[27]:w[27]+13]
    def le16(data,at):return int.from_bytes(data[at:at+2],'little')
    def texel(d,x,y):
        if d&0x8000:x=7-x
        if d&0x4000:y=7-y
        return (le16(tiles,(d&511)*16+y*2)>>(14-x*2))&3
    sprites=[];lx=ly=0
    for i in range(64):
        d=le16(sprite,i*4);x=sprite[i*4+2];y=sprite[i*4+3]
        if d&0x400:x+=lx
        if d&0x200:y+=ly
        lx,ly=x,y;x=(x+spx)&255;y+=spy
        if x>248:x-=256
        y=y-256 if 248<y<256 else y&255
        sprites.append((d,x,y,(pal[i]&15)*4))
    pixels=bytearray(320*256)
    report=struct.unpack_from('>128I',ram,ram.index(b'NFIR0001'))
    rows=ram[report[100]:report[100]+152]
    assert len(rows)==152
    for y in range(152):
        sx0=rows[y]
        for x in range(160):
            pen=248+(out&7);depth=0
            if wx<=x<wx+ww and wy<=y<wy+wh:
                pen=240+(bg&7)
                for n in range(2):
                    sx=(x+(sx0,sx1)[n])&255;sy=(y+(sy0,sy1)[n])&255
                    d=le16(maps,n*2048+(sy//8*32+sx//8)*2);t=texel(d,sx%8,sy%8)
                    z=3+2*n if swap else 5-2*n
                    if t and z>depth:pen=64+n*64+((d>>9)&15)*4+t;depth=z
                for d,px,py,palette in sprites:
                    z=((d>>11)&3)*2
                    if z>depth and px<=x<px+8 and py<=y<py+8:
                        t=texel(d,x-px,y-py)
                        if t:pen=palette+t;depth=z
            pixels[(y+52)*320+x+80]=pen
    return pixels


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);p.add_argument('--original',type=Path,required=True);a=p.parse_args()
    log=(a.directory/'run.log').read_text()
    for setting in ('cpu_model=68020','chipmem_size=4','bogomem_size=0','chipset=aga','ntsc=false','Model: "A1200OG".'):
        assert setting in log,setting
    fast=[x for x in log.splitlines() if x.startswith('fastmem_size=')]
    assert fast and all(x=='fastmem_size=0' for x in fast)
    assert 'CONTROL stage=8' in log and 'FIRE_INPUT ' in log
    original=json.loads(a.original.read_text());assert original['result']=='PASS'
    final=(a.directory/'chip.finished.bin').read_bytes();timeline=struct.unpack_from('>128I',final,final.index(b'NFIR0001'))
    live=struct.unpack_from('>16I',final,timeline[127])
    assert timeline[9]==1 and timeline[12]==0 and timeline[94]==0 and timeline[120]==0
    assert live[0]>0 and live[1]>=live[0] and live[2]==3 and live[3]==0 and live[5:9]==(1,1,1,1)
    assert timeline[102]==original['attempts']==timeline[7]
    assert timeline[16]<timeline[29] and 0<timeline[116]<=8 and timeline[40]-timeline[41]==170000
    assert timeline[41]>0
    base.expected=expected;results={}
    for label in dict.fromkeys(re.findall(r'^STAGE ([^ ]+) ',log,re.M)):
        path=a.directory/f'chip.{label}.bin';ram=path.read_bytes();w=struct.unpack_from('>128I',ram,ram.index(b'NFIR0001'))
        palette=ram[w[26]:w[26]+512]
        results[label]=base.verify(a.directory/f'screen.{label}.ppm',path,palette,b'NFIR0001',512)
        state=struct.unpack_from('>16I',ram,w[127]);f=struct.unpack_from('>25I',ram,state[4])
        results[label].update(snapshot_update=w[19],live_update=w[7],snapshot_attempt=f[24],fire_phase=f[2],drawing_phase=f[19])
    assert set(original['captures'])=={f'chip.{label}.bin' for label in results}
    assert original['original_gpu_pixels']['result']=='PASS' and original['original_gpu_pixels']['differences']==0
    assert results['finished']['fire_phase']==3 and results['finished']['drawing_phase']==1
    assert [results[x]['displayed_selection'] for x in ('initial','right','left','japan','return','repeat','return2')]==[1,2,1,0,1,2,1]
    stack=final[live[11]:live[11]+live[12]];assert len(stack)==16384
    untouched=next((i for i,v in enumerate(stack) if v!=0xa5),len(stack));assert untouched>0
    result={'result':'PASS','model':'PAL A1200 68EC020/AGA,2MiB Chip,zero Fast','package':'hard-disk directory',
      'captures':results,'original_replay':original,'explicit_display_allocation_bytes':170000,
      'stack_reserved_bytes':live[12],'stack_observed_highwater_bytes':len(stack)-untouched,
      'application_struct_bytes':live[9],'fire_drawing_terrain_objects_struct_bytes':live[10],
      'checks':['captured real joystick input replayed through unchanged original CPU','source state and immutable metadata independently compared before GPU reuse','all81920 bitplane bytes,256 Copper colors in both nibble banks,and every RGB pixel including borders','original GPU covers all24320 source pixels and both Amiga copies for every post-opening capture','512-byte streaming resource buffer; all54 map/feature sets retained'],
      'limits':['first fire only; power/cancel transitions and ball flight remain unconverted','hard-disk package,not a DD fit or physical hardware result','composition can lag and omit pictures; no full-speed claim','no native audio synthesis','stack highwater is measured only over these runs,not a universal worst-case bound']}
    (a.directory/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ('captures','original_replay')},indent=2))
if __name__=='__main__':main()
