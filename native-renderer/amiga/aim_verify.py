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
    report=struct.unpack_from('>128I',ram,ram.index(b'NAIM0001'))
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
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);p.add_argument('--scenario',required=True);a=p.parse_args()
    log=(a.directory/'run.log').read_text()
    for setting in ('cpu_model=68020','chipmem_size=4','bogomem_size=0','chipset=aga','ntsc=false','Model: "A1200OG".'):
        assert setting in log,setting
    assert all(x=='fastmem_size=0' for x in log.splitlines() if x.startswith('fastmem_size='))
    assert 'AIM stage=8' in log
    final=(a.directory/'chip.finished.bin').read_bytes();timeline=struct.unpack_from('>128I',final,final.index(b'NAIM0001'))
    base.expected=expected;results={}
    for label in dict.fromkeys(re.findall(r'^STAGE ([^ ]+) ',log,re.M)):
        path=a.directory/f'chip.{label}.bin';ram=path.read_bytes();w=struct.unpack_from('>128I',ram,ram.index(b'NAIM0001'))
        # Source bytes receive independent original replay in compare_aim_target.
        palette=ram[w[26]:w[26]+512]
        results[label]=base.verify(a.directory/f'screen.{label}.ppm',path,palette,b'NAIM0001',512)
        results[label].update(snapshot_shot_phase=w[96],live_shot_phase=w[95],snapshot_update=w[19],live_update=w[7],row_scroll_values=list(dict.fromkeys(ram[w[100]:w[100]+152])))
    assert timeline[12]==0 and timeline[9]==0
    assert timeline[94]==(0 if a.scenario=='cancel' else 4)
    if a.scenario!="cancel":
        assert timeline[110]==2 and timeline[112]==1 and timeline[113]==0
        assert final[timeline[27]+8:timeline[27]+10]==bytes([160,152])
    assert timeline[95]==(0 if a.scenario=='cancel' else 5)
    assert [results[x]['displayed_selection'] for x in ('initial','right','left','japan','return','repeat','return2')]==[1,2,1,0,1,2,1]
    if a.scenario.startswith(('shotseen','shotskip','raster')) or a.scenario=='shotheld':assert 'SHOT_SKIP ' in log,'Required hardware skip did not occur'
    if a.scenario.startswith('raster'):assert any(x['snapshot_shot_phase']==4 and 0<x['row_scroll_values'][0]<180 for x in results.values()),'Visible partial-fade exit was not captured'
    original=json.loads((a.directory/'original-verification.json').read_text())
    if a.scenario!='cancel':assert original['original_gpu_pixels']['result']=='PASS' and original['original_gpu_pixels']['differences']==0
    assert original['result']=='PASS' and set(original['captures'])=={f'chip.{label}.bin' for label in results}
    report={'result':'PASS','scenario':a.scenario,'captures':results,'original_replay':original,'logical_updates':{str(i):timeline[i] for i in (7,52,53,54,58,59,60,63,64,65,66,70,72,73,74,75,76,91,92,93)},'limits':['Stops after2098af viewport opening with2098ee delay3 installed; no playable shot or audio synthesis','Source row plan uses observed VBlank task-write timing; whole-game CPU timing is not emulated','Display snapshots may lag or omit transient phases; timings are not whole-gameFPS']}
    (a.directory/'verification.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
