#!/usr/bin/env python3
"""Verify equivalent scalar/packed/dirty synthetic target measurements."""
import argparse,json,struct
from pathlib import Path
from verify_benchmark import fnv,planar,expected_pixels

def verify(ram,log):
    for setting in ['cpu_model=68020','cpu_cycle_exact=true','cpu_memory_cycle_exact=true','cpu_throttle=0.0','ntsc=false','chipmem_size=4','bogomem_size=0']:
        if setting not in log:raise ValueError('Missing setting '+setting)
    settings=[x for x in log.splitlines() if x.startswith('fastmem_size=')]
    if not settings or any(x!='fastmem_size=0' for x in settings):raise ValueError('Fast RAM differs')
    if len(ram)!=2097152:raise ValueError('Wrong Chip size')
    marker=ram.find(b'NGPDISP1')
    if marker<0:raise ValueError('Missing header')
    pointer,size,done=struct.unpack_from('>3I',ram,marker+32)
    if done!=0x444f4e45 or size!=512 or pointer+size>len(ram):raise ValueError('Incomplete benchmark')
    r=struct.unpack_from('>128I',ram,pointer)
    if r[:3]!=(0x4e474254,3,709379):raise ValueError('Wrong clock/protocol')
    calibration=r[3]*1000/r[2]
    if not 19.8<calibration<20.2:raise ValueError('Bad clock calibration')
    screen=expected_pixels();frame=bytes(screen[(y+52)*320+x+80] for y in range(152) for x in range(160))
    checks=[fnv(planar(frame)),fnv(planar(bytes((x*7)&255 for y in range(152) for x in range(160)))),fnv(planar(bytes(((x*7)&255)^1 for y in range(152) for x in range(160))))]
    records=[]
    for i in range(12):
        mode,variant,frames,ticks,checksum,converted,skipped=r[4+i*7:11+i*7]
        wanted_mode=0 if i<6 else 1 if i<9 else 2
        wanted_variant=i if i<3 else 5-i if i<6 else i%3
        counts=(12160,0) if mode==2 else (3040,9120)
        if variant!=2:counts=(0,0)
        if (mode,variant,frames)!=(wanted_mode,wanted_variant,4) or ticks==0 or checksum!=checks[mode] or (converted,skipped)!=counts:
            raise ValueError('Bad workload/output at record %d'%i)
        records.append(dict(mode=mode,variant=variant,frames=frames,ticks=ticks,ms_per_frame=ticks*1000/r[2]/frames,converted_blocks=converted,skipped_blocks=skipped))
    for a,b in [(0,5),(1,4),(2,3)]:
        if abs(records[a]['ms_per_frame']/records[b]['ms_per_frame']-1)>.01:raise ValueError('Order sensitivity over 1%')
    return dict(result='PASS',pal_frame_calibration_ms=calibration,records=records,
                constraints=['same stock PAL A1200 emulator and full 320x256 eight-plane DMA display','160x152 synthetic redraw; no native game logic/audio','all full-redraw variants use the same sprite-cached compositor','dirty passes include one cold frame out of four; no skipped composition','conversion-only modes include identical row generation overhead','not physical hardware or golf frame rate'])
def main():
    p=argparse.ArgumentParser();p.add_argument('--chip',required=True,type=Path);p.add_argument('--log',required=True,type=Path);p.add_argument('--report',type=Path);a=p.parse_args()
    text=json.dumps(verify(a.chip.read_bytes(),a.log.read_text()),indent=2)+'\n';print(text,end='')
    if a.report:a.report.write_text(text)
if __name__=='__main__':main()
