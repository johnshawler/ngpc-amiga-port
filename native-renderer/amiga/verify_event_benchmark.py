#!/usr/bin/env python3
"""Verify explicit write dependencies versus packed and exact keys."""
import argparse,json,struct
from pathlib import Path
from verify_benchmark import fnv,planar,expected_pixels

def verify(ram,log):
    for setting in ['cpu_model=68020','cpu_cycle_exact=true','cpu_memory_cycle_exact=true','cpu_throttle=0.0','ntsc=false','chipmem_size=4','bogomem_size=0','chipset=aga','Model: "A1200OG".']:
        if setting not in log:raise ValueError('Missing setting '+setting)
    settings=[x for x in log.splitlines() if x.startswith('fastmem_size=')]
    if not settings or any(x!='fastmem_size=0' for x in settings):raise ValueError('Fast RAM differs')
    if len(ram)!=2097152:raise ValueError('Wrong Chip size')
    marker=ram.find(b'NGPDISP1')
    if marker<0:raise ValueError('Missing header')
    pointer,size,done=struct.unpack_from('>3I',ram,marker+32)
    if done!=0x444f4e45 or size!=512 or pointer+size>len(ram):raise ValueError('Incomplete benchmark')
    r=struct.unpack_from('>128I',ram,pointer)
    if r[:3]!=(0x4e474254,8,709379):raise ValueError('Wrong clock/protocol')
    calibration=r[3]*1000/r[2]
    if not 19.8<calibration<20.2:raise ValueError('Bad clock calibration')
    checks=[]
    for scroll in [251,250]:
        screen=expected_pixels(scroll)
        frame=bytes(screen[(y+52)*320+x+80] for y in range(152) for x in range(160))
        checks.append(fnv(planar(frame)))
    records=[]
    for i in range(16):
        mode,variant,frames,ticks,checksum,stat1,stat2=r[4+i*7:11+i*7]
        wanted_mode=0 if i<8 else 1 if i<12 else 2
        wanted_variant=i if i<4 else 7-i if i<8 else i%4
        counts=((608,0) if mode==1 or variant==3 else (152,456)) if variant else (1,587)
        if mode==2 and variant in (1,2):
            if stat1+stat2!=608 or not 152<=stat1<=608:raise ValueError("Bad event counts")
            counts=(stat1,stat2)
        if (mode,variant,frames)!=(wanted_mode,wanted_variant,4) or ticks==0 or checksum!=checks[int(mode==1)] or (stat1,stat2)!=counts:
            raise ValueError('Bad workload/output at record %d'%i)
        records.append(dict(mode=mode,variant=variant,frames=frames,ticks=ticks,ms_per_frame=ticks*1000/r[2]/frames,stat1=stat1,stat2=stat2))
    for a,b in [(0,7),(1,6),(2,5),(3,4)]:
        if abs(records[a]['ms_per_frame']/records[b]['ms_per_frame']-1)>.01:raise ValueError('Order sensitivity over 1%')
    return dict(result='PASS',pal_frame_calibration_ms=calibration,records=records,
                constraints=['same stock PAL A1200 emulator and full 320x256 eight-plane DMA display','160x152 synthetic redraw; no native game logic/audio','variants: packed baseline, exact keys, explicit writes, explicit writes forced invalid each frame','all variants render identical final pens; four frames per pass','no Blitter rendering or custom-chip tile acceleration is claimed','mode 2 includes 32 changed tile bytes per frame for every variant, with explicit notifications on event paths; not a replay of actual write events','not physical hardware or golf frame rate'])
def main():
    p=argparse.ArgumentParser();p.add_argument('--chip',required=True,type=Path);p.add_argument('--log',required=True,type=Path);p.add_argument('--report',type=Path);a=p.parse_args()
    text=json.dumps(verify(a.chip.read_bytes(),a.log.read_text()),indent=2)+'\n';print(text,end='')
    if a.report:a.report.write_text(text)
if __name__=='__main__':main()
