#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Execute unchanged original204a77 and compare complete native hole state."""
from hole_prepare_case_common import *
import os,random
lib=C.CDLL(os.environ.get('NTM_HOLE_LIBRARY',str(ROOT/'build/hole-prepare-check/native.so')))
lib.ntm_hole_prepare.argtypes=[C.POINTER(State),C.POINTER(Resources),C.POINTER(U8),C.POINTER(U8)]
R=random.Random(0x204a77);cases=0;lowest_stack=0x6be0

def run(course,hole,cursor_value,difficulty=0,old=0,index=0):
    global cases,lowest_stack
    setup();put(0x4000,R.randbytes(16384));store(0x4206,index,1)
    store(0x441d+2*index,course,1);store(0x441e+2*index,hole,1)
    for a,v in ((0x4146,difficulty),(0x4147,old),(0x41ec,0),(0x41ed,0),(0x400e,cursor_value)):store(a,v,1)
    s=read_state();cursor=U8(cursor_value);expected=bytearray(ram[:16384]);gfx=data(0x8000,16384) if cases%100==0 else None
    assert lib.ntm_hole_prepare(C.byref(s),C.byref(resources),C.byref(cursor),window)==0
    serialize(s,cursor,expected)
    reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x204a77);calls=[];draws=0;minimum_sp=0x6be0
    for instruction in range(10000):
        pc=get(8)
        if pc==0xef00:break
        if pc in (0x204b76,0x20d74b,0x204f57,0x204fa8):calls.append(pc)
        if pc==0x20197b:draws+=1
        step(0);minimum_sp=min(minimum_sp,get(7))
    else:raise AssertionError(('budget',hex(get(8))))
    actual=bytes(ram[:16384]);assert minimum_sp>=0x6b80;lowest_stack=min(lowest_stack,minimum_sp)
    expected[minimum_sp-0x4000:0x2be4]=actual[minimum_sp-0x4000:0x2be4]
    if actual!=expected:
        diff=[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b]
        raise AssertionError(((course,hole,cursor_value,difficulty,old,index),diff[:30]))
    assert calls==[0x204b76,0x20d74b,0x204f57,0x204fa8] and draws==6
    if gfx is not None:assert data(0x8000,16384)==gfx
    cases+=1
for course in range(3):
    for hole in range(18):
        for cursor in range(256):run(course,hole,cursor,index=hole)
    print('HOLE_PREPARE_PROGRESS',course+1,cases,file=sys.stderr,flush=True)
for course in range(3):
    for difficulty in range(8):
        for old in range(8):
            for index in range(18):run(course,index,(index*53+old*7)&255,difficulty,old,index)
report={'cases':cases,'lowest_stack':hex(lowest_stack),'scope':'all54 holes/all256 RNG starts, all8x8 difficulties/all18 order indices; complete nonstack RAM, ordered prerequisites, six shared draws and sampled graphics preservation'}

# Direct arithmetic entry comparisons include signed/fractional positions not
# present in the bounded actual course descriptors, without patching the ROM.
lib.ntm_hole_test_scale.argtypes=[U32];lib.ntm_hole_test_scale.restype=U32
lib.ntm_hole_test_direction.argtypes=[P,C.c_uint];lib.ntm_hole_test_direction.restype=U32
math_cases=0
for value in [0,1,65535,65536,0x7fffffff,0x80000000,0xffffffff]+[R.randrange(1<<32) for _ in range(4096)]:
    setup();reg(0,value);reg(8,0x2051e9)
    for i in range(2000):
        if get(8)==0xef00:break
        step(0)
    else:raise AssertionError('scale budget')
    assert get(0)==lib.ntm_hole_test_scale(value),('scale',hex(value),hex(get(0)),hex(lib.ntm_hole_test_scale(value)))
    math_cases+=1
for angle in range(4096):
    setup();reg(0,angle);reg(8,0x20d633)
    for i in range(200):
        if get(8)==0xef00:break
        step(0)
    else:raise AssertionError('direction budget')
    assert get(0)==lib.ntm_hole_test_direction(resources.quarter,angle),('direction',angle,hex(get(0)))
    math_cases+=1
report.update(arithmetic_cases=math_cases,arithmetic_scope='signed32bit/fractional scaling and all4096 direction angles against unchanged original helpers');print(json.dumps(report))
