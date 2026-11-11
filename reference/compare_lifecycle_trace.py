#!/usr/bin/env python3
"""Compare original/native lifecycle observations and unchanged game output."""
import argparse,copy,json
from pathlib import Path
from compare_course_trace import compare as compare_course
FIELDS=['lifecycle_calls','lifecycle_deferred','lifecycle_unlinked','lifecycle_recycled']
def compare(original,observed,repeated):
    if observed!=repeated:raise ValueError('Lifecycle repeat differs')
    stripped=copy.deepcopy(observed);previous=[0]*4
    for row in stripped[1:-1]:
        counts=[row.pop(k,None) for k in FIELDS]
        if any(type(v) is not int or v<p for v,p in zip(counts,previous)):raise ValueError('Invalid lifecycle counters')
        calls,deferred,unlinked,recycled=counts
        if deferred+unlinked!=calls or recycled>unlinked:raise ValueError('Invalid lifecycle outcomes')
        previous=counts
    if not previous[0]:raise ValueError('No lifecycle comparisons')
    result=compare_course(original,stripped,stripped)
    result.update(zip(FIELDS,previous));return result
if __name__=='__main__':
    p=argparse.ArgumentParser()
    for n in ['original','observed','repeated']:p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();data=[[json.loads(x) for x in path.read_text().splitlines()] for path in [a.original,a.observed,a.repeated]]
    print(json.dumps(compare(*data),sort_keys=True))
