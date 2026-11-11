# SPDX-License-Identifier: GPL-2.0-or-later
"""Cartridge-free tee exporter, lossless packing and frontend checks."""
import ctypes as C
import importlib.util
import random
import re
from pathlib import Path
import subprocess
import sys
import unittest
ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT/'build/tee-public-tests'
sys.path.insert(0,str(ROOT/'native-renderer/amiga'))
from tee_pack_cache import pack
from tee_verify import TeeCartridge, animation_clock, object_frame
spec = importlib.util.spec_from_file_location('tee_export',ROOT/'tools/export-tee-assets.py')
EXPORT = importlib.util.module_from_spec(spec)
spec.loader.exec_module(EXPORT)


def cartridge():
    rom = bytearray(0x180000)
    def put(at,value,size=4):
        rom[at-0x200000:at-0x200000+size] = value.to_bytes(size,'little')
    put(0x260020,0x230000)
    for slot,group in enumerate((0xb4,0x64,0x8c,0x90,0x94,0x98)):
        root = 0x231000 + 0x100 * slot
        put(0x230000+group,root)
        for entry in range(0,0xd8,4):
            put(root+entry,0x240000)
    put(0x240000,1,2);put(0x240002,0x250000);put(0x240006,0,2)
    put(0x240008,129,1);put(0x240009,0,2);put(0x24000b,65535,2)
    for course in range(3):
        put(0x21ba69+4*course,0x241000+0x100*course)
        for hole in range(18):
            put(0x241000+0x100*course+4*hole,40+hole,2)
            put(0x241002+0x100*course+4*hole,80,2)
    for i,at in enumerate((0x2197e6,0x2197e1,0x2197eb,0x2197f0)):
        put(at,52,2);put(at+2,(12,0,4,16)[i],2);put(at+4,3,1)
    put(0x2198f3+0x84,(0x242000<<8)|4)
    put(0x242000,1,1);put(0x242001,0,1);put(0x242002,0x242020)
    put(0x242020,0,2);put(0x26001c,0x243000)
    put(0x2198f3+0x88,(0x242030<<8)|32)
    put(0x242030,0,1);put(0x242031,0x123,2);put(0x242033,0x118,2);put(0x242035,0,1)
    put(0x260024,0x245000);put(0x260028,0x245100)
    put(0x245034,0x246000);put(0x245134,0x248000)
    for i in range(8):
        at=0x247000+32*i;put(0x246000+4*i,at)
        put(at,2,1);put(at+1,192,1);put(at+2,4,2);put(at+4,0x8000,2)
        put(at+6,0,1);put(at+7,0,1)
    put(0x248000,0x83,1);put(0x248001,0,1);put(0x248002,0x249000);put(0x249000,0x250000);put(0x248006,0,2)
    rom[0x50000:0x50010]=bytes(range(16))
    put(0x36000c,0x270000);put(0x360000,0x271000)
    for course in range(3):
        group=0x21bc00+course*0x100;put(0x21bb4d+2*course,group-0x21bb4d,2)
        put(0x270000+4*course,course*18*18,2);put(0x27000c+4*course,(course+3)*18*18,2)
        for hole in range(18):
            index=course*18+hole;at=0x21e000+index*128
            put(group+2*hole,at-group,2);put(at,0,1);put(at+1,4,1);put(at+2,400+hole,2)
            put(at+14,20,2);put(at+16,84,2)
            for pin in range(16):put(at+20+pin*4,1,2);put(at+22+pin*4,2,2)
            put(0x271000+18*index,0x272000+4*index);put(0x271004+18*index,0,2)
            put(0x272000+4*index,4,1);put(0x272001+4*index,6,1)
            put(0x271000+18*(index+54)+14,0x280000)
    put(0x204d81,65535,2)
    return rom


class Entry(C.Structure):
    _fields_=[('keys',C.c_uint8*12688),('packed',C.c_void_p),('packed_size',C.c_uint32)]


class TeeFrontend(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        BUILD.mkdir(parents=True,exist_ok=True)
        subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC',str(ROOT/'native-renderer/amiga/tee_planar_cache.c'),str(ROOT/'native-renderer/amiga/selector_memory.c'),'-o',str(BUILD/'cache.so')],check=True)
        cls.lib=C.CDLL(str(BUILD/'cache.so'))
        cls.lib.tee_planar_unpack.argtypes=[C.POINTER(Entry),C.c_void_p,C.c_void_p]

    def test_all_holes_export_and_range_deduplication(self):
        rom=cartridge();r=EXPORT.Resources(rom);r.collect();pool,offsets,_=r.pack()
        self.assertEqual(len(r.geometries),54)
        for name,(address,size) in r.spans.items():
            self.assertEqual(pool[offsets[name]:offsets[name]+size],r.data(address,size))
        report=EXPORT.export(rom,BUILD/'assets.inc')
        self.assertEqual((report['holes'],report['views'],report['animations']),(54,162,8))
        self.assertLess(report['deduplicated_pool_bytes'],report['raw_span_bytes'])
        text=(BUILD/'assets.inc').read_text()
        for course in range(3):
            for hole in range(18):
                for view in range(3):self.assertIn(f'r->holes[{course}][{hole}].views[{view}]',text)
        self.assertNotIn('r->handicap[',text)
        with self.assertRaisesRegex(ValueError,'ignored build'):
            EXPORT.export(rom,ROOT/'bad.inc')

    def test_fail_closed_resources(self):
        for at,value,size,reason in ((0x240000,0,2,'tile count'),(0x240008,0,1,'map rows'),(0x240009,65535,2,'Empty map'),(0x2197e1,56,2,'graphics index'),(0x242001,64,1,'palette group'),(0x242030,1,1,'single-color'),(0x240002,0x400000,4,'outside input')):
            rom=cartridge();rom[at-0x200000:at-0x200000+size]=value.to_bytes(size,'little')
            with self.subTest(at=hex(at)),self.assertRaisesRegex(ValueError,reason):EXPORT.export(rom,BUILD/'bad.inc')
        rom=cartridge();rom[0x40009:0x40009+66]=bytes(66)
        with self.assertRaisesRegex(ValueError,'Unterminated map'):EXPORT.export(rom,BUILD/'bad.inc')

    def test_lossless_pack_and_atomic_rejection(self):
        rng=random.Random(412)
        samples=[bytes(24320),bytes(range(256))*95,bytes([7])*24320]
        samples += [bytes(rng.randrange(k) for _ in range(24320)) for k in (2,3,4,256)]
        samples += [(bytes([i])*n+bytes([i+1])*(256-n))*95 for i,n in ((0,1),(2,2),(4,3),(6,127),(8,128),(10,129))]
        for source in samples:
            packed=pack(source);buffer=C.create_string_buffer(bytes(packed));entry=Entry();entry.packed=C.addressof(buffer);entry.packed_size=len(packed)
            output=(C.c_uint8*24320)();self.assertEqual(self.lib.tee_planar_unpack(C.byref(entry),output,None),0);self.assertEqual(bytes(output),source)
            for size in (0,1,len(packed)-1):
                entry.packed_size=size;C.memset(output,0xa5,24320)
                self.assertEqual(self.lib.tee_planar_unpack(C.byref(entry),output,None),-1);self.assertEqual(bytes(output),bytes([0xa5])*24320)

    def test_independent_color_stamp_object_and_animation(self):
        c=TeeCartridge(cartridge());tiles,maps={},{}
        c.stamp(tiles,maps,0xb4,0,0x40,0x2846)
        self.assertEqual(bytes(tiles[i] for i in range(0x460,0x470)),bytes(range(16)))
        self.assertEqual(maps,{0x40:0x46,0x41:0x28})
        self.assertEqual(animation_clock(c,0x247000,1),(0,2));self.assertEqual(animation_clock(c,0x247000,3),(0,0))
        sprite,palette=object_frame(c,{},0x247000,0,159,151,400)
        self.assertEqual(sprite[-2:],bytes((159,151)));self.assertEqual(palette,bytes((3,)))
        self.assertEqual(object_frame(c,{},0x247000,0,160,151,400),(b'',b''))

    def test_host_has_no_state_patch_or_rng_override(self):
        text=(ROOT/'native-renderer/amiga/tee_host.c').read_text()
        self.assertNotRegex(text,r'(?:memcpy|memset)\(chip')
        self.assertIn('RETRO_DEVICE_ID_JOYPAD_B',text);self.assertIn('"A1200OG"',text)
        self.assertIn('puae_fastmem_size',text);self.assertIn('word(80),word(78)',text)
        main=(ROOT/'native-renderer/amiga/tee_main.c').read_text()
        self.assertNotRegex(main,r'random_cursor\s*=')
        self.assertNotIn('delay=',main)


if __name__=='__main__':unittest.main()
