# SPDX-License-Identifier: GPL-2.0-or-later
"""Synthetic exporter/independent resource decoder tests; no private inputs."""
from pathlib import Path
import sys
import shutil
import subprocess
import unittest
from test_intro_frontend import cartridge, EXPORT, ROOT

sys.path.insert(0, str(ROOT / 'native-renderer/amiga'))
from round_verify import Cartridge

BUILD = ROOT / 'build/round-public-tests'


def cartridge_with_round():
    rom = cartridge()
    rom.extend(bytes(0x80000 - len(rom)))
    def put(address, value, size=4):
        rom[address - 0x200000:address - 0x200000 + size] = value.to_bytes(size, 'little')
    groups = (0xb4, 0x2c, 0x28, 0x30, 0x34, 0x38)
    cursor = 0x251000
    for group_index, group in enumerate(groups):
        base = 0x250000 + group_index * 256
        put(0x230000 + group, base)
        for entry in (0, 4, 8, 12, 16, 20, 0x30, 0x70, 0x84, 0x90):
            put(base + entry, cursor)
            put(cursor, 1, 2)
            put(cursor + 2, 0x256000)
            put(cursor + 6, 0, 2)
            narrow = group == 0xb4 and entry == 0x90
            put(cursor + 8, 1 if narrow else 129, 1)
            put(cursor + 9, 0, 1 if narrow else 2)
            put(cursor + 10 if narrow else cursor + 11, 255 if narrow else 65535, 1 if narrow else 2)
            cursor += 32
    put(0x223b55, 0x84, 2)
    for golfer in range(6):
        put(0x223b6d + 2 * golfer, 4 * golfer, 2)
    put(0x21b282, 0x3b, 1)
    put(0x21b283, 255, 1)
    put(0x2197f5, 32, 2)
    put(0x2197f7, 0, 2)
    put(0x2197f9, 3, 1)
    put(0x2198f3 + 0x9c, (0x225000 << 8) | 4)
    put(0x225000, 1, 1)
    put(0x225001, 0, 1)
    put(0x225002, 0x225010)
    put(0x225010, 0, 2)
    put(0x2198f3 + 0xa0, (0x225020 << 8) | 32)
    put(0x225020, 2, 1)
    put(0x225021, 0x345, 2)
    put(0x225023, 0x118, 2)
    put(0x225025, 0, 1)
    put(0x240000 + 32, 0x244100)
    put(0x244100, 0x244104)
    put(0x244104, 2, 1)
    put(0x244105, 192, 1)
    put(0x244106, 4, 2)
    put(0x244108, 0x8000, 2)
    put(0x24410a, 0xc0, 1)  # signed -64 component x offset
    put(0x240100 + 32, 0x244200)
    put(0x244200, 0x83, 1)
    put(0x244201, 0, 1)
    put(0x244202, 0x244300)
    put(0x244300, 0x256000)
    put(0x244206, 0, 2)
    rom[0x56000:0x56010] = bytes(range(16))
    return rom


class RoundFrontendTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        BUILD.mkdir(parents=True, exist_ok=True)

    @unittest.skipUnless(shutil.which('cc'), 'host compiler unavailable')
    def test_synthetic_round_application_and_private_copy(self):
        names = 'input_state course_choice task_lifecycle task_allocation random_byte viewport game_services course_parent course_screen course_driver course_palette course_initialization course_cursor course_application course_intro course_title round_record round_prepare course_intro_application'.split()
        sources = [str(ROOT/'native-game'/(name + '.c')) for name in names]
        sources += [str(ROOT/'native-renderer'/name) for name in ('scene_renderer.c', 'tile_renderer.c')]
        for private_copy in (False, True):
            binary = BUILD/('application-private' if private_copy else 'application')
            command = ['cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror'] + sources + [str(ROOT/'native-game/test_round_record_application.c')]
            if private_copy:
                command += ['-DNTM_SCREEN_PRIVATE_ASSETS'] + [str(ROOT/'native-renderer/amiga'/name) for name in ('intro_copy.c', 'selector_memory.c')]
            subprocess.run(command + ['-o', str(binary)], check=True, capture_output=True)
            result = subprocess.run([str(binary)], check=True, capture_output=True, text=True)
            self.assertIn('24 real traversals', result.stdout)

    def test_export_is_opt_in_and_bounded(self):
        rom = cartridge_with_round()
        legacy, new = BUILD/'legacy.inc', BUILD/'round.inc'
        EXPORT.export(rom, legacy)
        EXPORT.export(rom, new, True)
        self.assertEqual(legacy.read_text().count('static const uint8_t'), 43)
        text = new.read_text()
        self.assertEqual(text.count('static const uint8_t'), 96)
        self.assertIn('round_font_map[11]', text)
        self.assertIn('r->font.data=round_font_map', text)
        self.assertIn('r->object=o', text)
        for course in range(3):
            for index in range(2):
                self.assertIn(f'r->course[{course}][{index}]', text)
        for golfer in range(6):
            for index in range(2):
                self.assertIn(f'r->golfer[{golfer}][{index}]', text)
        self.assertNotIn('intro_round_resources', legacy.read_text())

    def test_round_resources_fail_closed(self):
        for address, value, size, error in [
            (0x2197f5, 36, 2, 'graphics index'),
            (0x244105, 128, 1, 'animation header'),
            (0x244108, 0, 2, 'single-component'),
            (0x225001, 64, 1, 'palette group'),
            (0x225020, 1, 1, 'single-color destination'),
            (0x244300, 0x300000, 4, 'outside input'),
        ]:
            rom = cartridge_with_round()
            rom[address - 0x200000:address - 0x200000 + size] = value.to_bytes(size, 'little')
            with self.subTest(address=hex(address)), self.assertRaisesRegex(ValueError, error):
                EXPORT.export(rom, BUILD/'bad.inc', True)
        rom = cartridge_with_round()
        rom[0x1b282:0x1b282 + 64] = bytes([59] * 64)
        with self.assertRaisesRegex(ValueError, 'Unterminated round score'):
            EXPORT.export(rom, BUILD/'bad.inc', True)

    def test_round_map_validation(self):
        rom = cartridge_with_round()
        c = Cartridge(rom)
        font = c.value((c.value(c.value(0x260020) + 0xb4) & 0xffffff) + 0x90)
        offset = font - 0x200000
        for data, error in ((bytes([1, 255]), 'Empty map row'), (bytes([1]) + bytes(33), 'Unterminated map row')):
            corrupted = bytearray(rom)
            corrupted[offset + 8:offset + 8 + len(data)] = data
            with self.subTest(error=error), self.assertRaisesRegex(ValueError, error):
                EXPORT.export(corrupted, BUILD/'bad-map.inc', True)

    def test_independent_narrow_and_wide_stamps(self):
        source = Cartridge(cartridge_with_round())
        for group, entry in ((0xb4, 0x90), (0x2c, 0x84)):
            tiles, maps = {}, {}
            self.assertEqual(source.stamp(tiles, maps, group, entry, 0x4c0, 0xe0), (1, 1))
            self.assertEqual(bytes(tiles[i] for i in range(0xe00, 0xe10)), bytes(range(16)))
            self.assertEqual(maps, {0x4c0: 0xe0, 0x4c1: 0})

    def test_independent_object_clip_palette_and_hidden_state(self):
        source = Cartridge(cartridge_with_round())
        tiles = {}
        sprites, palettes, count = source.object(tiles, True)
        self.assertEqual(count, 1)
        word = 0x1d0 | (3 << 11) | (0x80 << 9)
        self.assertEqual(sprites[:4], bytes((word & 255, word >> 8 & 255, 16, 28)))
        self.assertEqual(palettes, bytes([3]) + bytes(63))
        self.assertEqual(source.object(tiles, False)[:2], (bytes(256), bytes(64)))
        self.assertEqual(bytes(tiles[i] for i in range(0x1d00, 0x1d10)), bytes(range(16)))

    def test_host_is_read_only_except_hardware_input(self):
        text = (ROOT/'native-renderer/amiga/round_host.c').read_text()
        self.assertNotIn('memcpy(chip', text)
        self.assertNotIn('memset(chip', text)
        self.assertIn('RETRO_DEVICE_ID_JOYPAD_B', text)
        self.assertIn('word(68),word(67)', text)
        self.assertIn('"A1200OG"', text)
        self.assertIn('puae_fastmem_size', text)


if __name__ == '__main__':
    unittest.main()
