#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private original204a88 oracle, using the existing unchanged screen-probe.
No cartridge bytes, derived assets, traces or hashes belong in source.
"""
from screen_probe import *
import os
import random

U8=C.c_uint8
class State(C.Structure):
    _fields_=[('players',(U8*64)*2),('course_order',U8*36),('course_record_flags',U8*3)]+[
        (name,U8) for name in ('active_player','mapped_course','course_order_kind','reset_kind','round_index','hole_index')]
lib=C.CDLL(os.environ.get('NTM_ROUND_PREPARE_LIBRARY',str(ROOT/'build/round-prepare-check/native_round_prepare.so')))
lib.ntm_round_prepare.argtypes=[C.POINTER(State),C.POINTER(U8),C.POINTER(U8)]
R=random.Random(0x204a88)
def data(address,size):return bytes(load(address+i,1) for i in range(size))
def put(address,value):C.memmove(C.addressof(ram.contents)+address-0x4000,bytes(value),len(value))
window=(U8*256).from_buffer_copy(data(load(0x260010,4)-128,256))
entries=(0x20dfe6,0x20d732,0x204aa5,0x21290b)
cases=0;draws=0;max_draws=0

def serialize(state,cursor,out):
    for bank in range(2):out[0x149+64*bank:0x189+64*bank]=bytes(state.players[bank])
    out[0x41d:0x441]=bytes(state.course_order)
    out[0x205]=state.round_index;out[0x206]=state.hole_index;out[0xe]=cursor.value

def compare(state,cursor,label):
    global cases,draws,max_draws
    # The original runs unchanged, including duplicate rejection and shared RNG.
    before=bytearray(ram[:16384]);expected=before.copy()
    assert lib.ntm_round_prepare(C.byref(state),C.byref(cursor),window)==0
    serialize(state,cursor,expected)
    reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,0x204a88)
    calls=[];random_calls=0
    for instruction in range(50000):
        pc=get(8)
        if pc==0xef00:break
        if pc in entries:calls.append(pc)
        if pc==0x20197b:random_calls+=1
        step(0)
    else:raise AssertionError(('instruction budget',label,hex(get(8))))
    assert calls==list(entries),(label,calls)
    actual=bytes(ram[:16384])
    # Only return-address stack slots6bd4..6be3 are excluded. All remaining
    # modeled RAM, including unrelated records and order tail, is compared.
    expected[0x2bd4:0x2be4]=actual[0x2bd4:0x2be4]
    if actual!=expected:
        diff=[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b]
        raise AssertionError((label,diff[:20]))
    assert random_calls>0 and ((cursor.value-before[0xe])&255)==(random_calls&255)
    cases+=1;draws+=random_calls;max_draws=max(max_draws,random_calls)

def run(course,cursor_value,bank,reset,repeat=False):
    setup();put(0x4000,R.randbytes(16384))
    store(0x4204,course,1);store(0x4209,0,1);store(0x4144,bank,1);store(0x4203,reset,1);store(0x400e,cursor_value,1)
    state=State();state.active_player=bank;state.mapped_course=course;state.course_order_kind=0;state.reset_kind=reset
    state.round_index=load(0x4205,1);state.hole_index=load(0x4206,1)
    for b in range(2):state.players[b][:]=data(0x4149+64*b,64)
    state.course_order[:]=data(0x441d,36)
    for c in range(3):
        state.course_record_flags[c]=(cursor_value+c*37)&255
        store(0x5847+66*c,state.course_record_flags[c],1)
    cursor=U8(cursor_value)
    graphics=data(0x8000,0x4000) if repeat else None
    compare(state,cursor,(course,cursor_value,bank,reset,0))
    if repeat:
        compare(state,cursor,(course,cursor_value,bank,reset,1))
        assert data(0x8000,0x4000)==graphics

for course in range(3):
    for cursor in range(256):
        for bank in (0,1,255):
            for reset in (0,1,255):run(course,cursor,bank,reset,cursor in (0,127,128,255))
    # Exhaust both whole-byte branch operands, not just Boolean representatives.
    for value in range(256):
        run(course,(value*73)&255,value,255)
        run(course,(value*157)&255,0,value)
    print('ROUND_PREPARE_PROGRESS',course+1,cases,file=sys.stderr,flush=True)
print(json.dumps({'cases':cases,'random_calls':draws,'maximum_draws':max_draws,
    'scope':'unchanged original204a88; all three courses/all256 RNG cursors/all bank and reset bytes; saved flags; exact non-stack RAM, order tail, call order and repeats; graphics preserved at108 repeated boundaries'}))
