#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Unchanged-original20d1e9/20d633 arithmetic; no extracted data embedded."""
from screen_probe import *
import hashlib,os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_MARKER_LIBRARY',str(ROOT/'build/shot-marker-check/native.so')))
lib.ntm_shot_marker_multiply.argtypes=[C.c_uint32,C.c_uint32]
lib.ntm_shot_marker_multiply.restype=C.c_uint32
lib.ntm_shot_marker_direction.argtypes=[C.POINTER(C.c_uint8),C.c_uint16]
lib.ntm_shot_marker_direction.restype=C.c_uint32
quarter=(C.c_uint8*4100)(*[load(0x21d67b+i,1) for i in range(4100)])
r=random.Random(0x20d1e9)

def original(pc,a,b=0):
    setup();reg(0,a);reg(1,b);reg(8,pc)
    before=bytes(ram[:16384]);low=get(7)
    for _ in range(2000):
        if get(8)==0xef00:break
        step(0);low=min(low,get(7))
    else:raise AssertionError(('instruction budget',hex(pc),hex(get(8))))
    actual=bytes(ram[:16384])
    assert actual[:low-0x4000]==before[:low-0x4000] and actual[0x2be4:]==before[0x2be4:],('RAM',hex(pc))
    return get(0)

edges=(0,1,2,65535,65536,65537,0x7fffffff,0x80000000,0xffff0000,0xffff0001,0xfffffffe,0xffffffff)
pairs=[(a,b) for a in edges for b in edges]
pairs += [(r.randrange(1<<32),r.randrange(1<<32)) for _ in range(5000)]
pairs += [((0-a)&0xffffffff,b) for a in (1,2,127,255,256,32767,65535) for b in (1,2,127,255,256,32767,65535)]
for a,b in pairs:
    value=lib.ntm_shot_marker_multiply(a,b);expected=original(0x20d1e9,a,b)
    assert value==expected,('multiply',hex(a),hex(b),hex(value),hex(expected))
for angle in range(8192):
    a=angle if angle<4096 else angle+0xe000
    value=lib.ntm_shot_marker_direction(quarter,a);expected=original(0x20d633,a)
    assert value==expected,('direction',hex(a),hex(value),hex(expected))
print(json.dumps({'signed_multiply_cases':len(pairs),'direction_cases':8192,
    'scope':'exact register result and unchanged nonstack RAM; negative small-product rounding; wrapped angles',
    'source_sha256':hashlib.sha256((ROOT/'native-game/shot_markers.c').read_bytes()).hexdigest()}))
