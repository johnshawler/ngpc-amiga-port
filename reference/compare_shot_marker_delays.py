#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Hint/camera initial slices against unchanged original, stopping at active.

Mode0 hint termination executes genuine20132b, including task/free-list state.
Mode1..255 starts its delay. Active20c786/20c8a5 are deliberately not executed.
"""
from shot_markers_case_common import *
import hashlib,os,random
lib=C.CDLL(os.environ.get('NTM_SHOT_MARKER_LIBRARY',str(ROOT/'build/shot-marker-check/native.so')))
lib.ntm_shot_hint_initialize.argtypes=[C.POINTER(Hint),C.POINTER(Task),C.POINTER(TaskContext),U8]
lib.ntm_shot_hint_tick.argtypes=[C.POINTER(Hint),C.POINTER(Task)]
lib.ntm_shot_camera_initialize.argtypes=[C.POINTER(Camera),C.POINTER(Task)]
lib.ntm_shot_camera_tick.argtypes=[C.POINTER(Camera),C.POINTER(Task)]
finish=C.cast(lib.ntm_task_finish,P).value
R=random.Random(0x20c744);comparisons=initializations=countdowns=exits=0

def start(mode):
    setup();put(0x4000,R.randbytes(16384));store(0x4148,mode,1)
    nodes=(Task*4)();addresses=(0x6100,0x6000,0x6200,0x6300)
    mapping={addr(n):a for n,a in zip(nodes,addresses)};mapping[None]=0xffffffff
    def old(p):return mapping[p]
    for i,n in enumerate(nodes):
        n.callback=R.randrange(0x200000,0x210000)
        n.previous=addr(nodes[(i+3)%4]);n.next=addr(nodes[(i+1)%4]);n.owner=addr(nodes[0])
        n.priority=R.randrange(256);n.result=R.randrange(256);n.flags=R.randrange(256);n.substate=R.randrange(256);n.delay=R.randrange(65536)
    nodes[0].next=addr(nodes[1]);nodes[1].previous=addr(nodes[0]);nodes[1].next=addr(nodes[2]);nodes[2].previous=addr(nodes[1])
    tc=TaskContext();tc.free_head=addr(nodes[3]);tc.current=addr(nodes[1]);tc.published=addr(nodes[1])
    return nodes,tc,old

def serialize(nodes,tc,old,state,b):
    for a,n in zip((0x6100,0x6000,0x6200,0x6300),nodes):
        write(b,a,0x20132b if n.callback==finish else n.callback,4)
        for name,off in (('previous',4),('next',8),('owner',12)):write(b,a+off,old(getattr(n,name)),4)
        for name,off in (('priority',16),('result',17),('flags',18),('substate',19)):write(b,a+off,getattr(n,name))
        write(b,a+20,n.delay,2)
    write(b,0x5706,old(tc.free_head),4)
    if isinstance(state,Hint):
        for offset,name in enumerate(('repeat','last','pending','changed')):write(b,0x6016+offset,getattr(state,name))

def run(pc,stop,expected,nodes,tc,old,state,label):
    global comparisons
    serialize(nodes,tc,old,state,expected)
    reg(6,0x6000);reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc);low=0x6be0
    for _ in range(1000):
        if get(8) in (0xef00,stop):break
        step(0);low=min(low,get(7))
    else:raise AssertionError(('budget',label,hex(get(8))))
    actual=bytes(ram[:16384]);expected[low-0x4000:0x2be4]=actual[low-0x4000:0x2be4]
    assert actual==expected,(label,[(hex(0x4000+i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:20])
    assert get(6)==old(tc.current),(label,'task cursor',hex(get(6)),hex(old(tc.current)))
    comparisons+=1
    return get(8)

for kind,cls,init_pc,delay_pc,active_pc,delay in (
        ('hint',Hint,0x20c744,0x20c775,0x20c786,30),
        ('camera',Camera,0x20c883,0x20c894,0x20c8a5,8)):
    modes=range(1,256) if kind=='hint' else range(32)
    for mode in modes:
        nodes,tc,old=start(mode);state=cls();state.delay=delay_pc;state.active=active_pc
        if kind=='hint':state.repeat=29;state.last=84;state.pending=41;state.changed=32
        nodes[1].callback=init_pc
        before=bytearray(ram[:16384]);serialize(nodes,tc,old,state,before);put(0x4000,before)
        fn=getattr(lib,'ntm_shot_'+kind+'_initialize')
        args=(C.byref(state),C.byref(nodes[1]))+((C.byref(tc),mode) if kind=='hint' else ())
        assert fn(*args)==0
        assert nodes[1].delay==delay-1 and state.phase==1
        assert run(init_pc,active_pc,before,nodes,tc,old,state,(kind,mode,'init'))==0xef00
        initializations+=1
        for remaining in range(delay-1,-1,-1):
            before=bytearray(ram[:16384]);result=getattr(lib,'ntm_shot_'+kind+'_tick')(C.byref(state),C.byref(nodes[1]))
            assert result==(2 if remaining==0 else 0)
            expected_pc=active_pc if remaining==0 else 0xef00
            assert run(delay_pc,active_pc,before,nodes,tc,old,state,(kind,mode,remaining))==expected_pc
            countdowns+=1
    # Delay0 wraps to65535 and activates. The original tests the sign of the
    # decremented word, not equality with0 or pre-decrement signed ordering.
    for value in (0,1,2,7,8,29,30,255,256,32767,32768,32769,65535):
        nodes,tc,old=start(1);state=cls();state.delay=delay_pc;state.active=active_pc;state.phase=1
        nodes[1].callback=delay_pc;nodes[1].delay=value
        before=bytearray(ram[:16384]);serialize(nodes,tc,old,state,before);put(0x4000,before)
        activates=bool((value-1)&32768)
        assert getattr(lib,'ntm_shot_'+kind+'_tick')(C.byref(state),C.byref(nodes[1]))==(2 if activates else 0)
        assert run(delay_pc,active_pc,before,nodes,tc,old,state,(kind,'arbitrary',value))==(active_pc if activates else 0xef00)
        countdowns+=1

for case in range(512):
    nodes,tc,old=start(0);state=Hint(0x20c775,0x20c786,29,84,41,32,0)
    if case&4:tc.free_head=None
    nodes[1].callback=0x20c744;nodes[1].flags=case&1
    nodes[1].priority=(0,255,128,128)[case%4];nodes[2].priority=(255,0,128,127)[case%4]
    before=bytearray(ram[:16384]);serialize(nodes,tc,old,state,before);put(0x4000,before)
    assert lib.ntm_shot_hint_initialize(C.byref(state),C.byref(nodes[1]),C.byref(tc),0)==0
    assert state.phase==0 and (state.repeat,state.last,state.pending,state.changed)==(29,84,41,32)
    assert run(0x20c744,0x20c786,before,nodes,tc,old,state,('finish',case))==0xef00
    exits+=1
print(json.dumps({'comparisons':comparisons,'initializations':initializations,'countdown_ticks':countdowns,'hint_exits':exits,
    'scope':'all nonstack RAM,task cursor,callback,task links,free list; mode0 finishes,mode1..255 delays; no active callbacks executed',
    'source_sha256':hashlib.sha256((ROOT/'native-game/shot_markers.c').read_bytes()).hexdigest()}))
