#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Frozen-clock observation/noninterference check, separate from native tasks.

Requires existing original, screen-probe and (with --scene) scene reference
builds. Each run.py process creates a fresh save directory. Private traces stay
in ignored build/course-intro-check; no assets are copied into source.
"""
import argparse
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom', type=Path, default=ROOT/'private-inputs/neo-turf-masters.ngc')
p.add_argument('--frames', type=int, default=1300)
p.add_argument('--scene', action='store_true')
a = p.parse_args()
if a.frames < 1:
    p.error('frames must be positive')
out = ROOT/'build/course-intro-check'
out.mkdir(parents=True, exist_ok=True)
runs = []
kinds = ['clean', 'observed', 'repeated'] + (['scene'] if a.scene else [])
for name in kinds:
    cmd = [sys.executable, str(ROOT/'reference/run.py'), '--rom', str(a.rom.resolve()),
           '--frames', str(a.frames), '--inputs', str(ROOT/'reference/inputs.gameplay-fairway.json')]
    if name in ('observed', 'repeated'):
        cmd += ['--core', str(ROOT/'reference/build/screen-probe/mednafen_ngp_libretro.so')]
    elif name == 'scene':
        cmd += ['--core', str(ROOT/'reference/build/scene/mednafen_ngp_libretro.so'), '--scene-check']
    with (out/(name+'.jsonl')).open('w') as trace, (out/(name+'.log')).open('w') as log:
        subprocess.run(cmd, stdout=trace, stderr=log, check=True)
    rows = [json.loads(line) for line in (out/(name+'.jsonl')).read_text().splitlines()]
    assert len([r for r in rows if r['type'] == 'frame']) == a.frames
    runs.append(rows)
assert runs[0][1:] == runs[1][1:] == runs[2][1:], 'Original/probe/repeat output differs'
report = {'frames': a.frames, 'passes': len(kinds), 'frozen_clock': True,
          'scope': 'unchanged original/probe/repeat video, audio and final serialization; not native task replay'}
if a.scene:
    assert len(runs[0]) == len(runs[3])
    for original, observed in zip(runs[0][1:], runs[3][1:]):
        assert all(original[key] == observed[key] for key in original), 'Scene observer alters output'
    last = [row for row in runs[3] if row['type'] == 'frame'][-1]
    report.update(scene_rows=last['scene_rows'], scene_pixels=last['scene_pixels'])
(out/'frozen-reference.json').write_text(json.dumps(report)+'\n')
print(json.dumps(report))
