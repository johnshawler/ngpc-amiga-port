#!/usr/bin/env python3
"""Verify input-only versus native active-course pipeline target timing."""
import argparse,json,struct
from pathlib import Path
from functools import lru_cache
from verify_control_benchmark import expected as controls_expected
@lru_cache(maxsize=4)
def expected(variant):
    selection,result,sounds,finishes,previous=1,165,0,0,0
    if variant&1:
        for tick in range(16384):
            current=(tick>>5)&255;pressed=current&(~previous&255);previous=current
            if variant<2 and pressed&32:result=2;finishes+=1
            elif pressed&16:result=3 if current&64 else 1;finishes+=1
            else:
                d=pressed&12
                candidate=(selection-1)%256 if d==4 else (selection+1)%256 if d==8 else 256
                if candidate<3:selection=candidate;sounds+=1
    return controls_expected(0),selection,result,sounds,finishes

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
    if r[:3]!=(0x4e4c5442,1,709379):raise ValueError('Wrong protocol')
    calibration=r[3]*1000/r[2]
    if not 19.8<calibration<20.2:raise ValueError('Wrong clock')
    records=[]
    for n in range(8):
        v,count,ticks,digest=r[4+n*12:8+n*12];wanted=n if n<4 else 7-n
        data,selection,result,sounds,finishes=expected(wanted|1)
        deferred=0
        if wanted&1:
            previous=0
            for tick in range(16384):
                current=(tick>>5)&255;pressed=current&(~previous&255);previous=current
                if ((wanted<2 and pressed&32) or pressed&16) and (tick>>9)&1:deferred+=1
        h=2166136261
        for b in data+bytes([selection,result]):h=((h^b)*16777619)&0xffffffff
        actual=struct.pack('>4I',*r[8+n*12:12+n*12])
        extra=r[12+n*12:16+n*12]
        if v!=wanted or count!=16384 or not ticks or digest!=h or actual!=data or extra!=(selection*256+result,sounds,finishes,deferred):raise ValueError('Wrong pipeline state/effect workload')
        records.append(dict(variant=v,calls=count,ticks=ticks,microseconds_per_call=ticks*1e6/r[2]/count,sound_requests=sounds,finish_requests=finishes,deferred=deferred))
    for a,b in [(0,7),(1,6),(2,5),(3,4)]:
        if abs(records[a]['ticks']/records[b]['ticks']-1)>.01:raise ValueError('Order sensitivity')
    return dict(result='PASS',pal_frame_calibration_ms=calibration,records=records,
        constraints=['stock PAL A1200 emulation, zero Fast, all Chip, full eight-plane display DMA',
        'even variants input plus course; odd add task lifecycle, single dispatch and per-call synthetic task rearming',
        'gate0 in variants0/1, gate1 in2/3; same sample stream; sound requests counted, task completions dispatched',
        'repeated-call microbenchmark, not allocation, a game loop, cache-cold bound or whole-game FPS'])
def main():
    p=argparse.ArgumentParser();p.add_argument('--chip',type=Path,required=True);p.add_argument('--log',type=Path,required=True);p.add_argument('--report',type=Path);a=p.parse_args()
    text=json.dumps(verify(a.chip.read_bytes(),a.log.read_text()),indent=2)+'\n';print(text,end='')
    if a.report:a.report.write_text(text)
if __name__=='__main__':main()
