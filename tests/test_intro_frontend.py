# SPDX-License-Identifier: GPL-2.0-or-later
"""Public synthetic integration/export regressions; no private inputs required."""
import importlib.util
import subprocess
import shutil
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build' / 'intro-public-tests'
SPEC = importlib.util.spec_from_file_location('intro_export', ROOT / 'tools/export-course-intro-assets.py')
EXPORT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(EXPORT)


def cartridge():
    rom = bytearray(0x60100)
    def put(address, value, size=4):
        rom[address - 0x200000:address - 0x200000 + size] = value.to_bytes(size, 'little')
    put(0x260020, 0x230000)
    put(0x26001c, 0x235000)
    put(0x260024, 0x240000)
    put(0x260028, 0x240100)
    put(0x21980e, 20, 2)
    put(0x219812, 3, 1)
    for course in range(3):
        desc, group = 0x220000 + course * 16, 0x231000 + course * 32
        put(0x21b17d + course * 4, desc)
        put(desc, course * 4, 2)
        put(desc + 2, 7 + course, 2)
        put(desc + 4, 20, 2)
        put(desc + 6, 40, 2)
        put(desc + 8, 20 + course * 4, 2)
        put(desc + 10, 30, 2)
        put(0x230000 + course * 4, group)
        for i in (0, 2, 3, 4):
            asset = 0x232000 + course * 256 + i * 32
            put(group + i * 4, asset)
            put(asset, 1, 2)
            put(asset + 2, 0x233000 + course * 80 + i * 16)
            put(asset + 6, 0, 2)
            put(asset + 8, 129, 1)
            put(asset + 9, 0, 2)
            put(asset + 11, 65535, 2)
        animation, shapes, tiles = 0x241000 + course * 32, 0x242000 + course * 32, 0x243000 + course * 16
        put(0x240000 + 20 + course * 4, animation)
        put(animation, animation + 4)
        put(animation + 4, 1, 1)
        put(animation + 5, 0xc0, 1)
        put(animation + 6, 4, 2)
        put(animation + 8, 0x8000, 2)
        put(0x240100 + 20 + course * 4, shapes)
        put(shapes, 0x80, 1)
        put(shapes + 2, 0x244000 + course * 4)
        put(0x244000 + course * 4, tiles)
        put(shapes + 6, 0, 2)
        group_job, single_job = 0x221000 + course * 16, 0x222000 + course * 16
        put(0x2198f3 + (course + 53) * 4, group_job * 256 + 4)
        put(0x2198f3 + (course + 56) * 4, single_job * 256 + 32)
        put(group_job, course + 1, 1)
        put(group_job + 1, 16, 1)
        put(group_job + 2, 0x234000 + course * 16)
        for i in range(course + 1):
            put(0x234000 + course * 16 + i * 2, i * 8, 2)
        put(single_job, 2 * course, 1)
        put(single_job + 1, 0x123 + course, 2)
        put(single_job + 3, 0x118, 2)
        put(single_job + 5, 0, 1)
    return rom


class IntroFrontendTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        BUILD.mkdir(parents=True, exist_ok=True)

    @unittest.skipUnless(shutil.which('cc'), 'host compiler unavailable')
    def test_synthetic_application_and_private_copy(self):
        names = 'input_state course_choice task_lifecycle task_allocation random_byte viewport game_services course_parent course_screen course_driver course_palette course_initialization course_cursor course_application course_intro course_title round_record round_prepare course_intro_application'.split()
        sources = [str(ROOT / 'native-game' / (name + '.c')) for name in names]
        sources += [str(ROOT / 'native-renderer' / name) for name in ('scene_renderer.c', 'tile_renderer.c')]
        for test, expected in [('test_course_intro_application', '12 real selector/owner traversals'), ('test_course_title_application', '24 real traversals')]:
            for private_copy in (False, True):
                binary = BUILD / (test + ('-private' if private_copy else ''))
                command = ['cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror'] + sources + [str(ROOT / 'native-game' / (test + '.c'))]
                if private_copy:
                    command += ['-DNTM_SCREEN_PRIVATE_ASSETS'] + [str(ROOT / 'native-renderer/amiga' / name) for name in ('intro_copy.c', 'selector_memory.c')]
                subprocess.run(command + ['-o', str(binary)], check=True, capture_output=True)
                result = subprocess.run([str(binary)], check=True, capture_output=True, text=True)
                self.assertIn(expected, result.stdout)

    def test_export_uses_all_three_descriptors_and_palette_counts(self):
        output = BUILD / 'synthetic-assets.inc'
        EXPORT.export(cartridge(), output)
        text = output.read_text()
        for course in range(3):
            self.assertIn(f'r[{course}].descriptor=intro_{course}_descriptor', text)
            self.assertIn(f'r[{course}].palettes[0].count={course + 1}', text)
            self.assertIn(f'intro_{course}_offsets[{2 * (course + 1)}]', text)
            self.assertIn(f'r[{course}].palettes[1].offset={480 + 2 * course}', text)
            self.assertIn(f'intro_{course}_descriptor[12]', text)
            self.assertIn(f'r[{course}].tournament[1].data=intro_{course}_mode2_map', text)
            self.assertIn(f'o[{course}].animations[0].frames=1', text)
        self.assertEqual(text.count('static const uint8_t'), 43)

    def test_export_rejects_unbounded_or_unexpected_inputs(self):
        with self.assertRaisesRegex(ValueError, 'ignored build'):
            EXPORT.export(cartridge(), ROOT / 'forbidden-private-assets.inc')
        with self.assertRaisesRegex(ValueError, 'outside input'):
            EXPORT.export(b'\0', BUILD / 'bad.inc')
        rom = cartridge()
        rom[0x198f3 + 53 * 4] = 3
        with self.assertRaisesRegex(ValueError, 'group-palette service'):
            EXPORT.export(rom, BUILD / 'bad-kind.inc')

    def test_export_rejects_title_resource_errors(self):
        def changed(address, value, size=1):
            rom = cartridge()
            rom[address - 0x200000:address - 0x200000 + size] = value.to_bytes(size, 'little')
            return rom
        cases = [
            (0x220008, 16, 2, 'graphics index'),
            (0x241005, 0x80, 1, 'animation header'),
            (0x241008, 0, 2, 'single-component'),
            (0x220006, 512, 2, 'object exceeds'),
            (0x220004, 512, 2, 'Stamp exceeds'),
            (0x244000, 0x300000, 4, 'outside input'),
        ]
        for address, value, size, error in cases:
            with self.subTest(address=hex(address)), self.assertRaisesRegex(ValueError, error):
                EXPORT.export(changed(address, value, size), BUILD / 'bad-title.inc')


if __name__ == '__main__':
    unittest.main()
