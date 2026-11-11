#!/usr/bin/env python3
"""Validate the completed synthetic PAL benchmark; no emulator wall-time timing."""
import argparse,json,struct
from pathlib import Path
from scene_expected import expected_pixels

def fnv(data):
    value=2166136261
    for b in data:value=((value^b)*16777619)&0xffffffff
    return value

def planar(pixels):
    out=bytearray(24320)
    for y in range(152):
        for x in range(160):
            for p in range(8):
                if pixels[y*160+x]&(1<<p):out[p*3040+y*20+x//8]|=128>>(x&7)
    return out

def verify(ram,log):
    for setting in ['cpu_model=68020','cpu_cycle_exact=true','cpu_memory_cycle_exact=true','cpu_throttle=0.0','ntsc=false','chipmem_size=4','bogomem_size=0']:
        if setting not in log:raise ValueError('Missing setting '+setting)
    if not any(x=='fastmem_size=0' for x in log.splitlines()) or any(x!='fastmem_size=0' for x in log.splitlines() if x.startswith('fastmem_size=')):
        raise ValueError('Fast RAM settings differ')
    if len(ram)!=2097152:raise ValueError('Not 2 MiB Chip')
    marker=ram.find(b'NGPDISP1')
    if marker<0:raise ValueError('No fixture header')
    pointer,size,done=struct.unpack_from('>3I',ram,marker+32)
    if done!=0x444f4e45 or size!=384 or pointer+size>len(ram):raise ValueError('Incomplete benchmark')
    r=struct.unpack_from('>96I',ram,pointer)
    if r[:3]!=(0x4e474254,2,709379):raise ValueError('Unsupported benchmark clock/protocol')
    calibration=r[3]*1000/r[2]
    if not 19.8<calibration<20.2:raise ValueError('PAL frame timer calibration failed')
    screen=expected_pixels()
    frame=bytes(screen[(y+52)*320+x+80] for y in range(152) for x in range(160))
    expected=[fnv(planar(frame))]*6+[fnv(bytes([253])*160)]*2+[fnv(planar(bytes((x*7)&255 for y in range(152) for x in range(160))))]
    rows=[]
    for i in range(9):
        mode,cached,frames,ticks,checksum,rebuilds,hits=r[4+i*7:11+i*7]
        wanted_mode=0 if i<4 else 1 if i<6 else 2 if i<8 else 3
        wanted_cached=int(i in [1,2,5,7])
        if (mode,cached,frames)!=(wanted_mode,wanted_cached,1 if mode==1 else 4) or ticks==0 or checksum!=expected[i]:
            raise ValueError('Workload/output mismatch in record %d'%i)
        wanted_counts=(147,0) if i==5 else (1,587) if cached else (0,0)
        if (rebuilds,hits)!=wanted_counts:raise ValueError('Unexpected cache workload')
        rows.append(dict(mode=mode,cached=bool(cached),frames=frames,ticks=ticks,ms_per_frame=ticks*1000/r[2]/frames,rebuilds=rebuilds,hits=hits))
    for a,b in [(0,3),(1,2)]:
        if abs(rows[a]['ms_per_frame']/rows[b]['ms_per_frame']-1)>.01:raise ValueError('A/B versus B/A differs by over 1%')
    return dict(result='PASS',pal_frame_calibration_ms=calibration,records=rows,
                constraints=['emulated stock PAL A1200, not physical hardware','synthetic 160x152 full redraw with eight displayed AGA planes active','composition plus planar stores, no game logic or audio','CIA-B chained timers count PAL E-clocks; no host wall-time measurement','forced per-row change is deliberately worst case, not observed game cadence'])

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--chip',type=Path,required=True);p.add_argument('--log',type=Path,required=True);p.add_argument('--report',type=Path)
    a=p.parse_args();text=json.dumps(verify(a.chip.read_bytes(),a.log.read_text()),indent=2)+'\n';print(text,end='')
    if a.report:a.report.write_text(text)
if __name__=='__main__':main()
