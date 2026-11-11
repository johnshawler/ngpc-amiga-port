#!/usr/bin/env python3
"""Private course-state/effect trace comparison against the original game."""
import argparse,copy,json
from pathlib import Path
from compare_control_trace import compare as compare_control
FIELDS=['course_calls','course_sounds','course_finishes','course_confirm','course_cancel','course_option_confirm']
def compare(original,observed,repeated):
    if observed!=repeated:raise ValueError('Course repeat differs')
    stripped=copy.deepcopy(observed);previous=[0]*6
    for row in stripped[1:-1]:
        counts=[row.pop(k,None) for k in FIELDS]
        if any(not isinstance(v,int) or v<p for v,p in zip(counts,previous)):raise ValueError('Invalid course counters')
        calls,sounds,finishes,confirm,cancel,option=counts
        if sounds+finishes>calls or confirm+cancel+option!=finishes:raise ValueError('Invalid course effects')
        previous=counts
    if not previous[0]:raise ValueError('No course comparisons')
    result=compare_control(original,stripped,stripped)
    result.update(zip(FIELDS,previous));return result

def main():
    p=argparse.ArgumentParser()
    for n in ['original','observed','repeated']:p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();data=[[json.loads(x) for x in path.read_text().splitlines()] for path in [a.original,a.observed,a.repeated]]
    print(json.dumps(compare(*data),sort_keys=True))
if __name__=='__main__':main()
