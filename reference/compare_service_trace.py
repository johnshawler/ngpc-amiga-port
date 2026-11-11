#!/usr/bin/env python3
"""Compare converted service observations and unchanged original output."""
import argparse,copy,json
from pathlib import Path
from compare_random_trace import compare as compare_random
FIELDS=['service_calls','service_fade','service_setup','service_queue','service_reset']
def compare(original,observed,repeated):
    if observed!=repeated:raise ValueError('Service repeat differs')
    stripped=copy.deepcopy(observed);previous=[0]*5
    for row in stripped[1:-1]:
        counts=[row.pop(k,None) for k in FIELDS]
        if any(type(v) is not int or v<p for v,p in zip(counts,previous)):raise ValueError('Invalid service counters')
        if counts[0]!=sum(counts[1:]):raise ValueError('Invalid service totals')
        previous=counts
    if not previous[0]:raise ValueError('No service comparisons')
    result=compare_random(original,stripped,stripped);result.update(zip(FIELDS,previous));return result
if __name__=='__main__':
    p=argparse.ArgumentParser()
    for n in ['original','observed','repeated']:p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();data=[[json.loads(x) for x in path.read_text().splitlines()] for path in [a.original,a.observed,a.repeated]]
    print(json.dumps(compare(*data),sort_keys=True))
