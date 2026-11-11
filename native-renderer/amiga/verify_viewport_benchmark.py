#!/usr/bin/env python3
"""Verify authored viewport service through native composition and planar output."""
import argparse,json,struct
from pathlib import Path
from functools import lru_cache
from verify_benchmark import fnv,planar
from scene_expected import expected_pixels
@lru_cache(maxsize=7)
def checksum(stage):
    screen=expected_pixels(window=(stage*10,2,160-stage*20,147))
    return fnv(planar(bytes(screen[(y+52)*320+x+80] for y in range(152) for x in range(160))))
def verify(ram,log):
    for setting in ['cpu_model=68020','cpu_cycle_exact=true','cpu_memory_cycle_exact=true','cpu_throttle=0.0','ntsc=false','chipmem_size=4','bogomem_size=0','chipset=aga','Model: "A1200OG".']:
        if setting not in log:raise ValueError('Missing setting '+setting)
    fast=[x for x in log.splitlines() if x.startswith('fastmem_size=')]
    if not fast or any(x!='fastmem_size=0' for x in fast):raise ValueError('Wrong Fast RAM')
    if len(ram)!=2097152:raise ValueError('Wrong Chip size')
    marker=ram.find(b'NGPDISP1')
    if marker<0 or ram.find(b'NGPDISP1',marker+1)>=0:raise ValueError('Missing/duplicate header')
    ptr,size,done=struct.unpack_from('>3I',ram,marker+32)
    if size!=512 or done!=0x444f4e45 or ptr+size>len(ram):raise ValueError('Incomplete target')
    r=struct.unpack_from('>128I',ram,ptr)
    if r[:3]!=(0x4e564642,1,709379):raise ValueError('Wrong protocol')
    calibration=r[3]*1000/r[2]
    if not 19.8<calibration<20.2:raise ValueError('Wrong clock')
    records=[]
    for n in range(14):
        stage,ticks,digest,x,width,substate,busy,errors=r[4+n*8:12+n*8]
        wanted=n%7
        if stage!=wanted or not ticks or digest!=checksum(wanted) or [x,width,substate,busy,errors]!=[wanted*10,160-wanted*20,wanted*2+2,int(wanted<6),0]:raise ValueError('Wrong viewport/planar workload')
        records.append(dict(stage=stage,pass_number=n//7,ticks=ticks,milliseconds_per_redraw=ticks*1000/r[2],window_x=x,window_width=width))
    for n in range(7):
        if abs(records[n]['ticks']/records[n+7]['ticks']-1)>.01:raise ValueError('Repeat order sensitivity')
    return dict(result='PASS',pal_frame_calibration_ms=calibration,records=records,constraints=[
      'stock PAL A1200 emulation, 2MiB Chip, zero Fast, full eight-plane display DMA',
      'authored synthetic viewport table, not extracted game assets; two seven-step sequences',
      'native fade update -> row-latched scene bridge -> cached compositor -> packed planar Chip buffer',
      'offscreen output while static display remains active; first stage includes cold sprite cache',
      'full redraw cost, not game FPS; no game control pacing or actual sound/course-screen drawing'])
def main():
    p=argparse.ArgumentParser();p.add_argument('--chip',type=Path,required=True);p.add_argument('--log',type=Path,required=True);p.add_argument('--report',type=Path);a=p.parse_args()
    text=json.dumps(verify(a.chip.read_bytes(),a.log.read_text()),indent=2)+'\n';print(text,end='')
    if a.report:a.report.write_text(text)
if __name__=='__main__':main()
