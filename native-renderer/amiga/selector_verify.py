#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Independent per-pixel/depth verification of actual selector target captures.

Snapshots and captures contain private cartridge assets; all output stays local.
This proves the bounded selector display and host-injected hardware input path,
not whole-game equivalence, full speed, sound, or physical hardware behavior.
"""
import argparse
import json
import re
import struct
from pathlib import Path
from PIL import Image, ImageChops


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
    for y in range(152):
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


def palette_from_cartridge(rom, completed):
    """Resolve jobs independently of exporter/native staging to catch bad bindings."""
    def span(address, size):
        offset=address-0x200000
        assert 0<=offset<=len(rom)-size
        return rom[offset:offset+size]
    def value(address,size=4):return int.from_bytes(span(address,size),'little')
    palette=bytearray(512)
    group_count=value(0x21a045,1)
    assert 0<group_count<=64
    destination=value(0x21a046,1)*8
    offsets=value(0x21a047)
    colors=value(0x26001c)
    assert destination+group_count*8<=512
    for group in range(group_count):
        source=colors+value(offsets+group*2,2)
        palette[destination+group*8:destination+group*8+8]=span(source,8)
    for job in ((0x21a053,0x21a24e) if completed else (0x21a053,)):
        destination=480+value(job,1)
        assert destination<=510
        palette[destination:destination+2]=span(job+1,2)
    return bytes(palette)


def verify(capture,chip,source_palette,marker_bytes=b'NSEL0001',report_bytes=256):
    ram=chip.read_bytes();assert len(ram)==2097152
    marker=ram.find(marker_bytes);assert marker>=0 and ram.find(marker_bytes,marker+1)<0
    w=struct.unpack_from('>48I',ram,marker)
    assert w[2]==0x52454144 and w[3]==marker and w[12]==0
    assert report_bytes in (256,512)
    assert marker+170000<=len(ram) and w[4] in (marker+report_bytes,marker+report_bytes+81920)
    assert ram[w[26]:w[26]+512]==source_palette,'Source palette mismatch: target staging differs from independently resolved cartridge jobs'
    pixels=expected(ram,w);planes=bytearray(81920)
    for y in range(256):
        for x in range(320):
            for bit in range(8):
                if pixels[y*320+x]&(1<<bit):planes[bit*10240+y*40+x//8]|=128>>(x%8)
    assert ram[w[4]:w[4]+81920]==planes,'Complete displayed bitplanes differ from independent scene oracle'
    colors=[[None,None] for _ in range(256)];bank=low=0;controls={}
    for pos in range(w[5],w[5]+w[6],4):
        reg,val=struct.unpack_from('>HH',ram,pos)
        if reg==0xffff:assert val==0xfffe;break
        controls[reg]=val
        if reg==0x106:bank=(val>>13)&7;low=(val>>9)&1
        elif 0x180<=reg<=0x1be:colors[bank*32+(reg-0x180)//2][low]=val
    assert controls[0x100]==0x210 and controls[0x1e4]==0x2100
    rgb=[]
    for pen,(hi,lo) in enumerate(colors):
        source=int.from_bytes(ram[w[26]+pen*2:w[26]+pen*2+2],'little')
        if w[28]:source^=0xfff
        color=((source&15)<<8)|(source&0xf0)|((source>>8)&15)
        assert hi==lo==color,('palette',pen,hi,lo,color)
        rgb.append(((color>>8)*17,((color>>4)&15)*17,(color&15)*17))
    im=Image.open(capture).convert('RGB')
    frame=Image.new('RGB',(640,256));frame.putdata([rgb[v] for y in range(256) for v in pixels[y*320:(y+1)*320] for _ in range(2)])
    box=ImageChops.difference(im,Image.new('RGB',im.size,rgb[0])).getbbox()
    fbox=ImageChops.difference(frame,Image.new('RGB',frame.size,rgb[0])).getbbox()
    if box and fbox:
        origin=(box[0]-fbox[0],box[1]-fbox[1]);assert origin[0]>=0 and origin[1]>=0
    else:
        assert not box and not fbox;origin=(40,18)
    canvas=Image.new('RGB',im.size,rgb[0]);canvas.paste(frame,origin)
    assert im.tobytes()==canvas.tobytes(),'Every captured RGB pixel must match palette and coherent displayed snapshot'
    return {'selection':w[8],'displayed_selection':w[21],'updates':w[7],'displayed_update':w[19],'running':w[9],'outcome':w[10],
            'logical_updates_dropped':w[12],'complete_frames':w[13],'last_composition_seconds':w[14]/w[29],
            'max_row_ms':w[15]*1000/w[29],'max_row_or_cached_chunk_ms':w[15]*1000/w[29],'max_input_poll_gap_ms':w[16]*1000/w[29],
            'max_composition_seconds':w[31]/w[29],'rows_composed':w[32],'rows_reused':w[33],
            'random_delay_updates':w[38],'free_chip_before_display_allocation':w[40],'free_chip_after_display_allocation':w[41],
            'source_palette_bytes_verified':512,'planar_cache_hits':w[42],'planar_cache_misses':w[43],'max_cache_lookup_ms':w[44]*1000/w[29],'planar_cache_bytes':w[45],'last_cache_entry':w[46],'presented_cache_lookup_ms':w[47]*1000/w[29],
            'palette_inverted':bool(w[28]),'captured_pixels_verified':im.width*im.height,'origin':origin}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--directory',type=Path,required=True)
    p.add_argument('--scenario',choices=['accept','cancel','random'],default='accept')
    p.add_argument('--rom',type=Path,default=Path(__file__).resolve().parents[2]/'private-inputs/neo-turf-masters.ngc')
    a=p.parse_args();log=(a.directory/'run.log').read_text();rom=a.rom.read_bytes()
    for setting in ['cpu_model=68020','chipmem_size=4','bogomem_size=0','chipset=aga','ntsc=false','Model: "A1200OG".']:
        assert setting in log,setting
    assert all(x=='fastmem_size=0' for x in log.splitlines() if x.startswith('fastmem_size='))
    assert 'SELECTOR stage=8' in log,'Input flow did not complete'
    results={label:verify(a.directory/f'screen.{label}.ppm',a.directory/f'chip.{label}.bin',palette_from_cartridge(rom,label=='finished' and a.scenario!='cancel')) for label in ('initial','right','left','finished')}
    for label,selection in (('japan',0),('return',1),('repeat',2),('return2',1)):
        if (a.directory/f'chip.{label}.bin').exists():
            results[label]=verify(a.directory/f'screen.{label}.ppm',a.directory/f'chip.{label}.bin',palette_from_cartridge(rom,False))
            assert results[label]['displayed_selection']==selection
    latency={}
    for label,input_frame,reported,verified,reported_delta,verified_delta in re.findall(r'LATENCY (\w+) input_frame=(\d+) first_report_frame=(\d+) verified_frame=(\d+) reported_frames=(\d+) verified_frames=(\d+)',log):
        latency[label]={'input_frame':int(input_frame),'first_report_frame':int(reported),'verified_capture_frame':int(verified),'reported_frames':int(reported_delta),'verified_capture_frames':int(verified_delta),'verified_capture_ms_nominal_pal':int(verified_delta)*20}
    cache_enabled=results['initial']['planar_cache_bytes']>0
    if cache_enabled:
        assert all(v['last_cache_entry']<15 for label,v in results.items() if label!='finished'), 'Full selector captures must use exact cache hits'
        assert results['finished']['planar_cache_misses']>0, 'Fade/exit must exercise fallback'
    initial_frame=int(re.search(r'STAGE initial frame=(\d+)',log).group(1))
    assert [results[x]['displayed_selection'] for x in ('initial','right','left')]==[1,2,1]
    assert (results['finished']['random_delay_updates']>0)==(a.scenario=='random')
    assert results['finished']['running']==0 and results['finished']['outcome']==(252 if a.scenario=='cancel' else 0)
    result={'result':'PASS','scenario':a.scenario,'model':'PAL A1200 68EC020 AGA, 2 MiB Chip, zero Fast','explicit_display_allocation_bytes':170000,'captures':results,'navigation_latency':latency,'cache_enabled':cache_enabled,'initial_verified_capture_frame':initial_frame,'boot_to_verified_selector_ms_nominal_pal':initial_frame*20,
            'checks':['external RetroPad samples through emulated Amiga joystick registers','native application navigation and accept/cancel result','all displayed bitplane bytes against independent per-pixel/depth oracle','all 512 source palette bytes independently resolved from cartridge jobs','all 256 palette colors, both AGA nibble banks and inversion','every captured RGB pixel, including border'],
            'limits':['private bounded course-selector slice, no later gameplay or sound','coherent rendering lags independently paced input; animation frames can be skipped','navigation latency is button-to-oracle-verified capture, with two stable-frame capture guard; milliseconds use nominal PAL50Hz','cache is linked static data retained for executable lifetime; no whole-game memory proof','no full-speed or physical-hardware claim','takes over hardware until emulator reset, no OS return']}
    (a.directory/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if __name__=='__main__':main()
