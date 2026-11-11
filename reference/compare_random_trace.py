#!/usr/bin/env python3
"""Compare native random-provider observations and unchanged original output."""
import argparse,copy,json
from pathlib import Path
from compare_allocation_trace import compare as compare_allocation
def compare(original,observed,repeated):
    if observed!=repeated:raise ValueError('Random repeat differs')
    stripped=copy.deepcopy(observed);previous=0
    for row in stripped[1:-1]:
        count=row.pop('random_calls',None)
        if type(count) is not int or count<previous:raise ValueError('Invalid random counter')
        previous=count
    result=compare_allocation(original,stripped,stripped);result['random_calls']=previous;return result
if __name__=='__main__':
    p=argparse.ArgumentParser()
    for n in ['original','observed','repeated']:p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();data=[[json.loads(x) for x in path.read_text().splitlines()] for path in [a.original,a.observed,a.repeated]]
    print(json.dumps(compare(*data),sort_keys=True))
