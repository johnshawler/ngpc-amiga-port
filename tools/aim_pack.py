#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original bounded LZ encoder, paired with aim_unpack.c. No third-party code."""
from collections import defaultdict, deque

def pack(data):
    data=bytes(data);chains=defaultdict(deque);result=bytearray();literal=bytearray();at=0
    def flush():
        if literal:result.append(len(literal)-1);result.extend(literal);literal.clear()
    def insert(p):
        if p+3<=len(data):
            q=chains[data[p:p+3]];q.append(p)
            while len(q)>96:q.popleft()
    while at<len(data):
        best=0;distance=0
        if at+3<=len(data):
            q=chains[data[at:at+3]]
            while q and at-q[0]>65535:q.popleft()
            for p in reversed(q):
                n=3
                while n<130 and at+n<len(data) and data[p+n]==data[at+n]:n+=1
                if n>best:best=n;distance=at-p
                if best==130:break
        if best>=4:
            flush();result.extend((128+best-3,distance>>8,distance&255))
            for p in range(at,at+best):insert(p)
            at+=best
        else:
            literal.append(data[at]);insert(at);at+=1
            if len(literal)==128:flush()
    flush();return bytes(result)

def unpack(data,size):
    """Independent simple reference used only during generation/tests."""
    out=bytearray();at=0
    while at<len(data):
        code=data[at];at+=1;n=(code&127)+(3 if code&128 else 1)
        if len(out)+n>size:raise ValueError('output overflow')
        if code&128:
            if at+2>len(data):raise ValueError('truncated match')
            d=int.from_bytes(data[at:at+2],'big');at+=2
            if not 0<d<=len(out):raise ValueError('invalid distance')
            for _ in range(n):out.append(out[-d])
        else:
            if at+n>len(data):raise ValueError('truncated literal')
            out.extend(data[at:at+n]);at+=n
    if len(out)!=size:raise ValueError('output underflow')
    return bytes(out)
