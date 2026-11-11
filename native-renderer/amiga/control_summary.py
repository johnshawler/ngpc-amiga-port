#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Summarize verified stock runs, startup cost and coherent display latency."""
import argparse,json,re,struct
from pathlib import Path

def summarize(directory):
    runs={}
    for path in sorted(directory.glob('*/verification.json')):
        report=json.loads(path.read_text());assert report['result']=='PASS'
        d=path.parent;ram=(d/'chip.finished.bin').read_bytes();w=struct.unpack_from('>128I',ram,ram.index(b'NCTR0001'));hz=w[29]
        timings=[]
        for name in report['captures']:
            b=(d/f'chip.{name}.bin').read_bytes();s=struct.unpack_from('>128I',b,b.index(b'NCTR0001'))
            timings.append({'label':name,'snapshot_update':s[19],'current_update':s[7],'age_updates':s[7]-s[19],'composition_seconds':s[14]/hz,'shot_phase':s[96]})
        runs[d.name]={'captures':len(report['captures']),'original_updates':report['original_replay']['updates'],'dropped_updates':w[12],'chip_free_before_display':w[40],'chip_free_after_display':w[41],'startup_unpack_seconds':w[109]/hz,'max_poll_gap_seconds':w[16]/hz,'max_composition_seconds':w[31]/hz,'final_boundary':w[94],'final_open_phase':w[110],'final_open_update':w[111],'ready_update':w[119],'still_running':bool(w[9]),'partial_boundary':w[120],'handlers_installed':w[112],'input_dispatch':w[113],'max_clock_batch':w[114],'multi_update_polls':w[115],'max_executed_batch':w[116],'presentations':timings}
    if not runs:raise ValueError('No verified runs')
    result={'machine':'PAL A1200 68EC020/AGA,2MiB Chip,zero Fast','flows':len(runs),'captures':sum(r['captures'] for r in runs.values()),'original_updates':sum(r['original_updates'] for r in runs.values()),'runs':runs,'limits':['Bounded ready/camera/club hardware input with live host stop; no playable swing or full-speed claim','No audio synthesis; requests remain metadata','Composition may omit transient phases despite no dropped logical updates']}
    (directory/'summary.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);summarize(p.parse_args().directory)
