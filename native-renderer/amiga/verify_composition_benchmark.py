#!/usr/bin/env python3
"""Verify exact-key composition cache against its retained baseline."""
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
    if done!=0x444f4e45 or size!=384 or pointer+size>len(ram):raise ValueError('Incomplete benchmark')
    r=struct.unpack_from('>96I',ram,pointer)
    if r[:3]!=(0x4e474254,4,709379):raise ValueError('Wrong clock/protocol')
    calibration=r[3]*1000/r[2]
    if not 19.8<calibration<20.2:raise ValueError('Bad clock calibration')
    checks=[]
    for scroll in [251,250]:
        screen=expected_pixels(scroll)
        frame=bytes(screen[(y+52)*320+x+80] for y in range(152) for x in range(160))
        checks.append(fnv(planar(frame)))
    checks.append(fnv(bytes([253])*160))
    records=[]
    for i in range(8):
        mode,variant,frames,ticks,checksum,composed,reused=r[4+i*7:11+i*7]
        wanted_mode=0 if i<4 else 1 if i<6 else 2
        wanted_variant=int(i in [1,2,5,7])
        counts=(608,0) if mode==1 else (152,456)
        if not variant:counts=(0,0)
        if (mode,variant,frames)!=(wanted_mode,wanted_variant,4) or ticks==0 or checksum!=checks[mode] or (composed,reused)!=counts:
            raise ValueError('Bad workload/output at record %d'%i)
        records.append(dict(mode=mode,variant=variant,frames=frames,ticks=ticks,ms_per_frame=ticks*1000/r[2]/frames,composed_rows=composed,reused_rows=reused))
    for a,b in [(0,3),(1,2)]:
        if abs(records[a]['ms_per_frame']/records[b]['ms_per_frame']-1)>.01:raise ValueError('Order sensitivity over 1%')
    return dict(result='PASS',pal_frame_calibration_ms=calibration,records=records,
                constraints=['same stock PAL A1200 emulator and full 320x256 eight-plane DMA display','160x152 synthetic redraw; no native game logic/audio','baseline uses sprite-cached composition; candidate uses exact dependency keys','both full redraw variants use identical dirty planar conversion','candidate includes one cold frame out of four; scroll-change case forces misses','not physical hardware or golf frame rate'])
def main():
    p=argparse.ArgumentParser();p.add_argument('--chip',required=True,type=Path);p.add_argument('--log',required=True,type=Path);p.add_argument('--report',type=Path);a=p.parse_args()
    text=json.dumps(verify(a.chip.read_bytes(),a.log.read_text()),indent=2)+'\n';print(text,end='')
    if a.report:a.report.write_text(text)
if __name__=='__main__':main()
