#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private unchanged-original banner raster/IRQ/latch comparison.

Uses existing unstripped screen-probe core symbols by name, without rebuilding
or patching its instructions. Local functions expose the existing original
renderer/timer sequence to the isolated routine harness. No target state patch,
cartridge byte, capture, firmware, or private hash is embedded in this source.
"""
import ctypes as C
import json
import os
import subprocess
from screen_probe import *

U8 = C.c_uint8
class Raster(C.Structure):
    _fields_ = [(n,U8) for n in ('lower_scroll','upper_scroll','timer_run','timer_mode','timer_divisor','gpu_control','callback','counter','interrupt_shadow')]
class Surface(C.Structure):
    _fields_ = [('pixels',C.c_void_p)]+[(n,C.c_int32) for n in ('width','height','pitch','depth')]
path = ROOT/'reference/build/screen-probe/mednafen_ngp_libretro.so'
symbols = {p[2]:int(p[0],16) for line in subprocess.check_output(['nm',str(path)],text=True).splitlines() if len(p:=line.split())==3}
required = ('retro_reference_life_step','NGPGfx','TRUN','T01MOD','timer_threshold','timer','IntPrio','reset_timers','reset_int','ngpgfx_reset','ngpgfx_draw','interrupt','updateTimers','TLCS900h_interpret')
assert all(n in symbols for n in required), 'Existing unstripped probe core lacks required named local symbols'
base = C.cast(core.retro_reference_life_step,C.c_void_p).value-symbols['retro_reference_life_step']
def scalar(name,kind=U8):return kind.from_address(base+symbols[name])
def function(name,result,*args):return C.CFUNCTYPE(result,*args)(base+symbols[name])
gfx = scalar('NGPGfx',C.c_void_p).value
# Checked ngpgfx_t byte fields before raster_line: latched controls, VRAM and
# palette arrays. No offset into an instruction stream or unseen file is used.
raster_offset = 14+4096+8192+256+64+512+18
assert U8.from_address(gfx+raster_offset).value == load(0x8009,1)
reset_timers = function('reset_timers',None)
reset_int = function('reset_int',None)
reset_gfx = function('ngpgfx_reset',None,C.c_void_p)
draw = function('ngpgfx_draw',C.c_bool,C.c_void_p,C.c_void_p,C.c_bool)
irq = function('interrupt',None,U8,U8)
timers = function('updateTimers',C.c_bool,C.c_void_p,C.c_int)
interpret = function('TLCS900h_interpret',C.c_int)
lib = C.CDLL(os.environ.get('NTM_RASTER_LIBRARY',str(ROOT/'build/shot-raster-check/native.so')))
lib.ntm_shot_raster_install.argtypes = [C.POINTER(Raster)]
lib.ntm_shot_raster_remove.argtypes = [C.POINTER(Raster)]
lib.ntm_shot_raster_irq.argtypes = [C.POINTER(Raster),U8,C.POINTER(U8)]
lib.ntm_shot_raster_step.argtypes = [C.POINTER(Raster),C.c_uint,C.c_uint,C.POINTER(U8),C.POINTER(U8),C.POINTER(U8)]
def call(address):
    reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,address)
    for _ in range(1000):
        if get(8)==0xef00:return
        step(0)
    raise AssertionError(('original budget',hex(get(8))))
def original_state():
    return (scalar('TRUN').value,scalar('T01MOD').value,scalar('timer_threshold').value,load(0x8000,1),int(load(0x6fd4,4)==0x20c620),scalar('timer').value)
def assert_state(s):
    assert original_state()==tuple(getattr(s,n) for n in ('timer_run','timer_mode','timer_divisor','gpu_control','callback','counter')), (original_state(),list(bytes(s)))
    assert load(0x6c27,1)==s.interrupt_shadow
    assert load(0x6fd4,4)==(0x20c620 if s.callback else 0x200471)
    assert ((C.c_int32*11).from_address(base+symbols['IntPrio'])[3]&7)==(3 if s.callback else 0)
registration = 0
for run in range(256):
    for control in (0,64,128,192):
        setup();reset_timers();reset_int();reset_gfx(gfx)
        store(0x20,run,1);store(0x24,173,1);store(0x22,37,1);store(0x8000,control,1)
        store(0x8032,91,1);store(0x4441,17,1);store(0x4443,193,1)
        shadow=(run*37+control)&255;store(0x6c27,shadow,1)
        s=Raster(17,193,run,173,37,control,0,0,shadow)
        reg(0,1);reg(4,0x20c620);call(0x20042e)
        assert lib.ntm_shot_raster_install(C.byref(s))==0
        assert_state(s);assert load(0x8032,1)==91
        call(0x200453);assert lib.ntm_shot_raster_remove(C.byref(s))==0
        assert_state(s);assert load(0x8032,1)==91
        registration+=1
# Every raster byte and every value of both independent source bytes. The full
#65536cross-product per raster byte is additionally exhausted by the public C test.
setup();reset_timers();reset_int();reset_gfx(gfx);store(0x8006,255,1)
store(0x6fd4,0x20c620,4);irq_cases=0
for line in range(256):
    assert load(0x8009,1)==line
    for upper in range(256):
        lower=upper^165;s=Raster(lower,upper,1,0,1,64,1,0,0);raw=U8(91)
        store(0x4441,lower,1);store(0x4443,upper,1);store(0x8032,91,1)
        for i in range(7):reg(i,0x14253647+0x100*i+upper)
        reg(7,0x6be0);reg(8,0xef00);reg(9,0)
        before=[get(i) for i in range(10)];memory=C.string_at(C.addressof(ram.contents),16384)
        irq(7,3);low=get(7)
        for _ in range(20):
            if get(8)==0xef00:break
            step(0);low=min(low,get(7))
        else:raise AssertionError('Original IRQ did not return')
        assert [get(i) for i in range(10)]==before
        actual=C.string_at(C.addressof(ram.contents),16384)
        assert actual[:low-0x4000]==memory[:low-0x4000] and actual[0x2be0:]==memory[0x2be0:]
        assert lib.ntm_shot_raster_irq(C.byref(s),line,C.byref(raw))==0
        assert load(0x8032,1)==raw.value
        irq_cases+=1
    draw(gfx,None,True)
# Run actual original timer-triggered IRQs, not manually forced callbacks, and
# compare every latched row and subsequent raw register through two full frames.
pixels=(C.c_uint16*(160*152))();surface=Surface(C.addressof(pixels),160,152,160,16)
clock_rows=0;max_irq_clocks=0
for period in (151,152,198,255):
    for seed in range(8):
        setup();reset_timers();reset_int();reset_gfx(gfx);store(0x8006,period,1)
        s=Raster(seed*31,255-seed*29,128,0,0,128,0,0,0)
        store(0x20,s.timer_run,1);store(0x8000,s.gpu_control,1)
        store(0x4441,s.lower_scroll,1);store(0x4443,s.upper_scroll,1)
        raw=U8(91+seed);latched=U8(0);shown=U8(0);store(0x8032,raw.value,1)
        reg(0,1);reg(4,0x20c620);call(0x20042e);lib.ntm_shot_raster_install(C.byref(s))
        # Unrelated VBlank has no implementation in this isolated task harness.
        # It is disabled by reset_int; its pending status cannot change Timer0.
        completed=0
        while completed<2*(period+1):
            line=load(0x8009,1);timers(C.byref(surface),1)
            if load(0x8009,1)==line:continue
            clocks=0
            for _ in range(20):
                if get(8)==0xef00:break
                cycles=interpret();clocks+=cycles;timers(C.byref(surface),cycles)
            else:raise AssertionError(('Original clock callback',line,hex(get(8))))
            max_irq_clocks=max(max_irq_clocks,clocks)
            assert lib.ntm_shot_raster_step(C.byref(s),line,period,C.byref(raw),C.byref(latched),C.byref(shown))==0
            assert shown.value==U8.from_address(gfx+4).value and raw.value==load(0x8032,1),(period,seed,line,shown.value,raw.value)
            assert_state(s);clock_rows+=1;completed+=1
            # Change source bytes between boundaries, proving IRQ-time reads.
            if line in (0,71,72,150):
                s.upper_scroll=(s.upper_scroll+37)&255;s.lower_scroll=(s.lower_scroll-19)&255
                store(0x4441,s.lower_scroll,1);store(0x4443,s.upper_scroll,1)
report={'result':'PASS','registration_pairs':registration,'original_irq_cases':irq_cases,'clock_sequences':32,'original_timer_latch_rows':clock_rows,'maximum_original_irq_cpu_clocks':max_irq_clocks,'scope':['register/unregister preserve raw scroll and own exact6c27 shadow writes','all256 raster bytes and each source-byte value','IRQ preserves all CPU registers and nonstack RAM','real original timer-triggered callback and delayed-latch sequence','screen periods151,152,198,255 and live source changes'],'limits':['bounded normal Timer0 divisor1 callback only','not whole-game raster timing or an A1200 performance measurement','existing unstripped probe core required; no core rebuild']}
print(json.dumps(report,indent=2))
