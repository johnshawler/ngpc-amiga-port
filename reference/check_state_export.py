#!/usr/bin/env python3
"""Compare clean original and instrumented runs; generated data stays private."""
import argparse, hashlib, json, subprocess, sys, tempfile
from pathlib import Path
from state_format import decode, SIZE
ROOT=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--rom',required=True,type=Path);p.add_argument('--frames',type=int,default=600);p.add_argument('--report',type=Path)
a=p.parse_args()
if a.frames < 1:p.error('frames must be positive')
with tempfile.TemporaryDirectory(prefix='ngpc-state-check-') as temp:
    temp=Path(temp);runs=[]
    for instrumented in (False,True):
        cmd=[sys.executable,str(ROOT/'run.py'),'--rom',str(a.rom.resolve()),'--frames',str(a.frames),'--inputs',str(ROOT/'inputs.example.json')]
        if instrumented:cmd+=['--core',str(ROOT/'build/instrumented/mednafen_ngp_libretro.so'),'--snapshot-dir',str(temp/'snapshots')]
        output=subprocess.check_output(cmd,text=True)
        rows=[json.loads(x) for x in output.splitlines()]
        runs.append(rows)
    originals=[r for r in runs[0] if r['type']=='frame'];observed=[r for r in runs[1] if r['type']=='frame']
    assert len(originals)==len(observed)==a.frames
    chain=hashlib.sha256()
    for original,other in zip(originals,observed):
        other=dict(other);blob=(temp/'snapshots'/('frame-%06d.ngr1'%other['frame'])).read_bytes()
        assert other.pop('cpu_ram_gpu_sha256')==hashlib.sha256(blob).hexdigest()
        assert original==other,('output mismatch',original['frame'])
        decoded=decode(blob);assert len(decoded['ram_4000_7fff'])==len(decoded['gpu_8000_bfff'])==16384
        chain.update(blob)
    states=[[r for r in rows if r['type']=='state'] for rows in runs];assert states[0]==states[1]
    report={'result':'PASS','frames':a.frames,'snapshot_schema':'NGR1-v1','snapshot_bytes':SIZE,'checks':['original vs instrumented per-frame video and audio','identical final core serialization','snapshot size/header/register widths','system RAM equals libretro RAM each frame','repeated export stable each frame','undersized output rejected without mutation each frame'],'snapshot_stream_sha256':chain.hexdigest(),'original_core_sha256':runs[0][0]['core_sha256'],'instrumented_core_sha256':runs[1][0]['core_sha256'],'input_sha256':runs[0][0]['input_sha256'],'rom_sha256':runs[0][0]['rom_sha256'],'limitations':['frame-boundary observation, not full hardware/save state','GPU MMIO reads include source raster position; internal latch state not exported','no Amiga execution or native equivalence','sampled input path, not gameplay coverage']}
    text=json.dumps(report,indent=2)+'\n';print(text,end='')
    if a.report:a.report.write_text(text)
