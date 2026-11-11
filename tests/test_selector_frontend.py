"""Cartridge-free tests for the actual Amiga selector's pure boundary code."""
import importlib.util
from pathlib import Path
import shutil
import re
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SelectorFrontendTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which('cc'), 'host compiler unavailable')
    def test_input_cadence_palette_and_conservative_row_reuse(self):
        out = ROOT / 'build/native-selector/test-frontend-unittest'
        out.parent.mkdir(parents=True, exist_ok=True)
        sources = ['native-renderer/amiga/selector_frontend.c',
                   'native-renderer/amiga/selector_test.c',
                   'native-renderer/scene_renderer.c',
                   'native-renderer/tile_renderer.c']
        subprocess.run(['cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                        *[str(ROOT / x) for x in sources], '-o', str(out)], check=True)
        result = subprocess.run([str(out)], check=True, capture_output=True, text=True)
        self.assertIn('384 exact invalidation cases', result.stdout)


    @unittest.skipUnless(shutil.which('cc'), 'host compiler unavailable')
    def test_exact_planar_key_mutations_and_fade_fallbacks(self):
        out = ROOT / 'build/native-selector/test-cache-key-unittest'
        out.parent.mkdir(parents=True, exist_ok=True)
        sources = ['native-renderer/amiga/selector_planar_cache.c',
                   'native-renderer/amiga/selector_memory.c',
                   'native-renderer/amiga/selector_cache_test.c',
                   'native-renderer/scene_renderer.c', 'native-renderer/tile_renderer.c']
        subprocess.run(['cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                        *[str(ROOT / x) for x in sources], '-o', str(out)], check=True)
        result = subprocess.run([str(out)], check=True, capture_output=True, text=True)
        self.assertIn('4653 invalid-key misses', result.stdout)

    def test_palette_count_survives_different_cursor_geometry_count(self):
        spec = importlib.util.spec_from_file_location('selector_export', ROOT/'tools/export-course-selector-assets.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        payload = bytearray(0x70000)
        def put(address, value, size=4):
            offset = address - 0x200000
            payload[offset:offset+size] = value.to_bytes(size, 'little')
        put(0x260020, 0x200100); put(0x200118, 0x200200)
        for i in range(5):
            address = 0x201000 + i * 0x100
            put(0x200200 + i * 4, address)
            put(address, 1, 2); put(address + 2, 0x205000 + i * 16)
            put(address + 6, 0, 2); put(address + 8, 1, 1)
            put(address + 9, 0, 2); put(address + 11, 65535, 2)
        palette_count = 48
        put(0x21a045, palette_count, 1); put(0x21a047, 0x206000)
        put(0x26001c, 0x207000)
        for i in range(palette_count):
            put(0x206000 + i * 2, i * 8, 2)
        put(0x260010, 0x20b080)
        put(0x219836, 16, 2)
        put(0x260024, 0x208000); put(0x208010, 0x208100)
        put(0x260028, 0x209000); put(0x209010, 0x209100)
        for animation in range(2):
            address = 0x208200 + animation * 0x100
            put(0x208100 + animation * 4, address)
            for frame in range(5):
                put(address + frame * 4, 5, 1)
                put(address + frame * 4 + 2, 20 + frame * 4, 2)
                put(address + 20 + frame * 4, 0x8000, 2)
        # One geometry entry, intentionally different from 48 palette groups.
        put(0x209100, 128, 1); put(0x209102, 0x209200)
        put(0x209200, 0x20a000); put(0x209106, 0, 2)
        output = ROOT/'build/native-selector/synthetic-export-regression.inc'
        module.export(bytes(payload), output)
        generated = output.read_text()
        count = int(re.search(r'j\[0\]\.count=(\d+);', generated).group(1))
        self.assertEqual(count, payload[0x1a045])
        self.assertEqual(count, palette_count)
        self.assertIn('selector_palette_offsets[96]', generated)
        self.assertIn('selector_cursor_shapes[8]', generated)

    def test_exporter_rejects_public_output_before_reading_assets(self):
        spec = importlib.util.spec_from_file_location('selector_export', ROOT/'tools/export-course-selector-assets.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        with self.assertRaisesRegex(ValueError, 'ignored build'):
            module.export(b'', ROOT/'native-renderer/amiga/accidental-assets.inc')


if __name__ == '__main__':
    unittest.main()
