# SPDX-License-Identifier: GPL-2.0-or-later
"""Cartridge-free first-tee resource exporter and row dependency regression."""
import importlib.util,subprocess,unittest
from pathlib import Path
from test_tee_frontend import cartridge
ROOT=Path(__file__).resolve().parents[1];BUILD=ROOT/'build/shot-public-tests'
spec=importlib.util.spec_from_file_location('shot_export',ROOT/'tools/export-shot-assets.py');EXPORT=importlib.util.module_from_spec(spec);spec.loader.exec_module(EXPORT)
def fixture():
    rom=cartridge()
    def put(at,v,n=4):rom[at-0x200000:at-0x200000+n]=v.to_bytes(n,'little')
    for i,group in enumerate((12,0x68,0x78,0x7c)):
        root=0x232000+256*i;put(0x230000+group,root)
        for entry in range(0,256,4):put(root+entry,0x240000)
    put(0x21b85c+8,0,2);put(0x2197c8,56,2);put(0x2197ca,60,2);put(0x2197cc,3,1)
    put(0x245038,0x24a000);put(0x245138,0x248000)
    for velocity in (60,148,152,156):put(0x24a000+velocity,0x247000)
    for command,kind,at in ((0x64,4,0x242000),(0x68,32,0x242030),(0x6c,24,0x242100),(0x70,40,0x242200)):put(0x2198f3+command,(at<<8)|kind)
    put(0x242100,1,1);put(0x242101,32,1);put(0x242102,16,1);put(0x242103,0x242020);put(0x242107,0x242020)
    put(0x242200,6,1);put(0x242201,16,1);put(0x242202,0,2);put(0x242204,0x123,2);put(0x242206,0x118,2);put(0x242208,0,1)
    return rom
class ShotFrontend(unittest.TestCase):
    def test_all_resources_and_dedup(self):
        BUILD.mkdir(parents=True,exist_ok=True);r=EXPORT.Resources(fixture());r.collect();pool,offsets,_=r.pack()
        for name,(address,size) in r.spans.items():self.assertEqual(pool[offsets[name]:offsets[name]+size],r.data(address,size))
        report=EXPORT.export(fixture(),BUILD/'assets.inc');self.assertEqual(report['holes'],54);self.assertEqual(report['banner_velocities'],[60,148,152,156]);self.assertTrue(report['terrain_map_unread_at_actual_tee'])
        with self.assertRaisesRegex(ValueError,'ignored build'):EXPORT.export(fixture(),ROOT/'bad.inc')
    def test_invalid_resource_rejection(self):
        for address,v,n,reason in ((0x2197c8,57,2,'Unsupported banner object'),(0x242100,0,1,'palette groups'),(0x242101,64,1,'palette groups'),(0x242200,31,1,'animated single'),(0x242208,2,1,'animated single'),(0x242206,0x1234,2,'animated single'),(0x245138,0x400000,4,'outside input')):
            rom=fixture();rom[address-0x200000:address-0x200000+n]=v.to_bytes(n,'little')
            with self.subTest(address=address),self.assertRaisesRegex(ValueError,reason):EXPORT.Resources(rom).collect()
    def test_row_service_and_no_target_patches(self):
        BUILD.mkdir(parents=True,exist_ok=True)
        sources=['native-renderer/amiga/shot_frontend_test.c','native-renderer/amiga/shot_frontend.c','native-renderer/amiga/selector_frontend.c','native-renderer/amiga/selector_memory.c','native-game/shot_raster.c','native-renderer/tile_renderer.c']
        subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror',*[str(ROOT/x) for x in sources],'-o',str(BUILD/'frontend')],check=True)
        subprocess.run([str(BUILD/'frontend')],check=True)
        main=(ROOT/'native-renderer/amiga/shot_main.c').read_text();host=(ROOT/'native-renderer/amiga/shot_host.c').read_text()
        self.assertNotRegex(main,r'random_cursor\s*=');self.assertNotIn('delay=',main);self.assertNotRegex(host,r'(?:memcpy|memset)\(chip')
        self.assertIn('s->row_scroll',main);self.assertIn('shot_dirty_rows',main)
if __name__=='__main__':unittest.main()
