#!/usr/bin/env python3
"""Compare private original/control-observed/repeated traces without publishing them."""
import argparse,json
from pathlib import Path

def compare(original,observed,repeated):
    if observed!=repeated:raise ValueError('Control repeat differs')
    if len(original)!=len(observed) or len(original)<3:raise ValueError('Wrong trace length')
    a,b=dict(original[0]),dict(observed[0])
    if a.get('type')!='metadata' or b.get('type')!='metadata':raise ValueError('Missing metadata')
    a.pop('core_sha256',None);b.pop('core_sha256',None)
    if a!=b:raise ValueError('Input/clock/cartridge metadata differs')
    previous=0
    for n,(a,b) in enumerate(zip(original[1:-1],observed[1:-1])):
        b=dict(b);count=b.pop('control_calls',None)
        if not isinstance(count,int) or count<previous:raise ValueError('Invalid control coverage')
        previous=count
        if a!=b or a.get('frame')!=n or a.get('type')!='frame':raise ValueError('Video/audio/input mismatch')
    if not previous:raise ValueError('No native control comparisons')
    frames=len(original)-2
    if original[-1]!=observed[-1] or original[-1].get('type')!='state' or original[-1].get('after_frames')!=frames:raise ValueError('Final state mismatch')
    return dict(result='PASS',frames=frames,control_calls=previous,original_video_audio_state_match=True,repeat_match=True)

def main():
    p=argparse.ArgumentParser()
    for name in ['original','observed','repeated']:p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();data=[[json.loads(x) for x in path.read_text().splitlines()] for path in [a.original,a.observed,a.repeated]]
    print(json.dumps(compare(*data),sort_keys=True))
if __name__=='__main__':main()
