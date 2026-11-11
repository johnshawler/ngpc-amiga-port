#!/usr/bin/env python3
"""Verify synthetic native-control target timing, not game FPS."""
import argparse,json,struct
from pathlib import Path
from functools import lru_cache

@lru_cache(maxsize=4)
def expected(variant):
    s=[i*17 for i in range(16)];s[5]=[7,255,0,128][variant];s[6]=[32,128,0,127][variant]
    for tick in range(16384):
        sample=(tick>>5)&255;old=s[0];s[4]=old;s[0]=sample;s[1]=sample&(~old&255);s[2]=s[3]=0
        for i in range(8):
            if old&sample&(1<<i):
                s[8+i]=128 if s[8+i]==255 else s[8+i]+1
                masked=s[8+i]&s[5]
                if masked==0:s[2]|=1<<i
                a=masked if masked<128 else masked-256;b=s[6] if s[6]<128 else s[6]-256
                if a>=b:s[3]|=1<<i
            else:s[8+i]=0
    return bytes(s)

def verify(ram,log):
    for setting in ['cpu_model=68020','cpu_cycle_exact=true','cpu_memory_cycle_exact=true','cpu_throttle=0.0','ntsc=false','chipmem_size=4','bogomem_size=0','chipset=aga','Model: "A1200OG".']:
        if setting not in log:raise ValueError('Missing setting '+setting)
    fast=[x for x in log.splitlines() if x.startswith('fastmem_size=')]
    if not fast or any(x!='fastmem_size=0' for x in fast):raise ValueError('Wrong Fast RAM')
    if len(ram)!=2097152:raise ValueError('Wrong Chip size')
    marker=ram.find(b'NGPDISP1')
    if marker<0 or ram.find(b'NGPDISP1',marker+1)>=0:raise ValueError('Missing/duplicate header')
    ptr,size,done=struct.unpack_from('>3I',ram,marker+32)
    if size!=320 or done!=0x444f4e45 or ptr+size>len(ram):raise ValueError('Incomplete target')
    r=struct.unpack_from('>80I',ram,ptr)
    if r[:3]!=(0x4e435442,1,709379):raise ValueError('Wrong protocol')
    calibration=r[3]*1000/r[2]
    if not 19.8<calibration<20.2:raise ValueError('Wrong clock')
    records=[]
    for n in range(8):
        v,count,ticks,digest=r[4+n*8:8+n*8];wanted=n if n<4 else 7-n
        data=expected(wanted);h=2166136261
        for b in data:h=((h^b)*16777619)&0xffffffff
        actual=struct.pack('>4I',*r[8+n*8:12+n*8])
        if v!=wanted or count!=16384 or ticks==0 or digest!=h or actual!=data:raise ValueError('Wrong routine state/workload')
        records.append(dict(variant=v,calls=count,ticks=ticks,microseconds_per_call=ticks*1e6/r[2]/count))
    for a,b in [(0,7),(1,6),(2,5),(3,4)]:
        if abs(records[a]['ticks']/records[b]['ticks']-1)>.01:raise ValueError('Order sensitivity')
    return dict(result='PASS',pal_frame_calibration_ms=calibration,records=records,
        constraints=['stock PAL A1200 emulation, zero Fast, all Chip, full eight-plane display DMA',
        'native input-state routine plus call loop/sample arithmetic only; no rendering or other game work timed',
        'four synthetic configurations; no original TLCS speed ratio or whole-game FPS claim'])

def main():
    p=argparse.ArgumentParser();p.add_argument('--chip',type=Path,required=True);p.add_argument('--log',type=Path,required=True);p.add_argument('--report',type=Path);a=p.parse_args()
    text=json.dumps(verify(a.chip.read_bytes(),a.log.read_text()),indent=2)+'\n';print(text,end='')
    if a.report:a.report.write_text(text)
if __name__=='__main__':main()
