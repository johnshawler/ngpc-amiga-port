#!/usr/bin/env python3
"""Verify synthetic native parent-flow target state and CIA timing."""
import argparse,json,struct
from pathlib import Path
def verify(ram,log):
    for setting in ['cpu_model=68020','cpu_cycle_exact=true','cpu_memory_cycle_exact=true','cpu_throttle=0.0','ntsc=false','chipmem_size=4','bogomem_size=0','chipset=aga','Model: "A1200OG".']:
        if setting not in log:raise ValueError('Missing setting '+setting)
    fast=[x for x in log.splitlines() if x.startswith('fastmem_size=')]
    if not fast or any(x!='fastmem_size=0' for x in fast):raise ValueError('Wrong Fast RAM')
    if len(ram)!=2097152:raise ValueError('Wrong Chip size')
    marker=ram.find(b'NGPDISP1')
    if marker<0 or ram.find(b'NGPDISP1',marker+1)>=0:raise ValueError('Missing/duplicate header')
    ptr,size,done=struct.unpack_from('>3I',ram,marker+32)
    if size!=448 or done!=0x444f4e45 or ptr+size>len(ram):raise ValueError('Incomplete target')
    r=struct.unpack_from('>112I',ram,ptr)
    if r[:3]!=(0x4e504642,1,709379):raise ValueError('Wrong protocol')
    calibration=r[3]*1000/r[2]
    if not 19.8<calibration<20.2:raise ValueError('Wrong clock')
    records=[]
    for n in range(6):
        v,flows,ticks,calls,draws,renders,sounds,digest,*state=r[4+n*16:20+n*16]
        wanted=n if n<3 else 5-n
        selection=0 if wanted==2 else 1;remaining=0 if wanted==2 else 85
        flags=53 if wanted==1 else 181;mapped=204 if wanted==1 else 2 if wanted==2 else 0
        substate=0 if wanted==1 else 165;word=0xaabb if wanted==1 else 4
        h=2166136261
        for b in [selection,17,remaining,flags,mapped,substate,255,255]:h=((h^b)*16777619)&0xffffffff
        counts=([31,0,1,1],[7,0,1,1],[91,30,31,31])[wanted]
        if v!=wanted or flows!=128 or not ticks or [calls,draws,renders,sounds]!=[x*128 for x in counts] or digest!=h or state!=[selection,17,remaining,word*256+flags,65535,mapped,substate,0]:raise ValueError('Wrong parent flow state/workload')
        records.append(dict(variant=v,flows=flows,ticks=ticks,calls=calls,random_draws=draws,render_requests=renders,sound_requests=sounds,milliseconds_per_flow=ticks*1000/r[2]/flows,microseconds_per_callback_average=ticks*1e6/r[2]/calls))
    for a,b in [(0,5),(1,4),(2,3)]:
        if abs(records[a]['ticks']/records[b]['ticks']-1)>.01:raise ValueError('Order sensitivity')
    return dict(result='PASS',pal_frame_calibration_ms=calibration,records=records,constraints=[
      'stock PAL A1200 emulation, 2MiB Chip, zero Fast, full eight-plane display DMA',
      '128 initialized parent flows each; ordinary accept, cancel, modified accept; authored synthetic table',
      'sound/render requests stubbed; stop before fade/course-setup service; no real-time frame pacing',
      'timing includes synthetic graph setup and checks; hot repeated workload, not whole-game FPS or original speed ratio'])
def main():
    p=argparse.ArgumentParser();p.add_argument('--chip',type=Path,required=True);p.add_argument('--log',type=Path,required=True);p.add_argument('--report',type=Path);a=p.parse_args()
    text=json.dumps(verify(a.chip.read_bytes(),a.log.read_text()),indent=2)+'\n';print(text,end='')
    if a.report:a.report.write_text(text)
if __name__=='__main__':main()
