#!/usr/bin/env python3
"""Compare private original, scene-instrumented and repeat JSONL traces."""
import argparse
import json
from pathlib import Path

def compare(original, observed, repeated):
    if observed != repeated:
        raise ValueError('Instrumented repeat differs')
    if len(original) != len(observed) or len(original) < 3:
        raise ValueError('Trace length mismatch or no frames')
    a, b = dict(original[0]), dict(observed[0])
    if a.get('type') != 'metadata' or b.get('type') != 'metadata':
        raise ValueError('Missing metadata')
    a.pop('core_sha256', None); b.pop('core_sha256', None)
    if a != b:
        raise ValueError('Inputs, clock, cartridge or core metadata differ')
    frames = len(original) - 2
    for n, (a, b) in enumerate(zip(original[1:-1], observed[1:-1])):
        b = dict(b)
        if b.pop('scene_rows', None) != (n+1)*152 or b.pop('scene_pixels', None) != (n+1)*152*160:
            raise ValueError('Incomplete scanline coverage')
        if a != b or a.get('frame') != n or a.get('type') != 'frame':
            raise ValueError('Frame video/audio/input mismatch')
    if original[-1] != observed[-1] or original[-1].get('type') != 'state' or original[-1].get('after_frames') != frames:
        raise ValueError('Final serialized state mismatch')
    return dict(result='PASS', frames=frames, rows=frames*152, pixels=frames*152*160,
                original_video_audio_state_match=True, instrumented_repeat_match=True)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['original','observed','repeated']:p.add_argument('--'+name,required=True,type=Path)
    a=p.parse_args()
    data=[[json.loads(line) for line in path.read_text().splitlines()] for path in [a.original,a.observed,a.repeated]]
    print(json.dumps(compare(*data),sort_keys=True))
if __name__=='__main__':main()
