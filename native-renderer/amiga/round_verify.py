#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Independent cartridge-resource and complete PAL A1200 round-record captures.

The emulator host supplies hardware joystick input only. Snapshot phase and live
phase are deliberately separate: exact pixels do not prove timely presentation.
"""
import argparse
import json
import re
import struct
from pathlib import Path
from selector_verify import palette_from_cartridge, verify
from intro_verify import source_palette as title_palette, source_state as title_state

SCENARIOS = ['course0', 'course1', 'course2', 'random', 'cancel', 'skip0', 'skip1', 'skip2', 'skipboth',
             'recordskip0', 'recordskip1', 'recordskip2', 'recordheld']


class Cartridge:
    def __init__(self, rom):
        self.rom = rom

    def span(self, address, size):
        offset = address - 0x200000
        assert size >= 0 and 0 <= offset <= len(self.rom) - size
        return self.rom[offset:offset + size]

    def value(self, address, size=4):
        return int.from_bytes(self.span(address, size), 'little')

    def stamp(self, tiles, maps, group_offset, entry, destination, tile):
        group = self.value(self.value(0x260020) + group_offset) & 0xffffff
        source = self.value(group + entry)
        count, pixels = self.value(source, 2), self.value(source + 2)
        assert 0 < count <= 512 and tile + count <= 512
        for index in range(count):
            block = self.span(pixels + self.value(source + 6 + index * 2, 2), 16)
            for byte, value in enumerate(block):
                tiles[(tile + index) * 16 + byte] = value
        pointer = source + 6 + count * 2
        header = self.value(pointer, 1)
        pointer += 1
        stride, rows = (2 if header & 128 else 1), header & 127
        assert 0 < rows <= 32
        for y in range(rows):
            for x in range(33):
                entry = self.value(pointer, stride)
                pointer += stride
                if entry == (65535 if stride == 2 else 255):
                    assert x > 0
                    break
                assert x < 32
                value = (entry + tile) & 65535
                dest = destination + 64 * y + 2 * x
                assert 0 <= dest <= 4094
                maps[dest], maps[dest + 1] = value & 255, value >> 8
            else:
                raise AssertionError('Unterminated original map row')
        return count, rows

    def palette(self, prior):
        palette = bytearray(prior)
        for command in (0x9c, 0xa0):
            packed = self.value(0x2198f3 + command)
            kind, source = packed & 255, packed >> 8
            if kind == 4:
                count, destination = self.value(source, 1), self.value(source + 1, 1) * 8
                assert 0 < count <= 64 and destination + 8 * count <= 512
                offsets, colors = self.value(source + 2), self.value(0x26001c)
                for group in range(count):
                    palette[destination + group * 8:destination + (group + 1) * 8] = self.span(colors + self.value(offsets + group * 2, 2), 8)
            else:
                assert kind == 32
                destination = 480 + self.value(source, 1)
                assert destination <= 510 and not destination & 1
                palette[destination:destination + 2] = self.span(source + 1, 2)
        return palette

    def object(self, tiles, visible):
        graphics = self.value(0x2197f5, 2)
        assert graphics == 32
        root = self.value(self.value(0x260024) + graphics) & 0x00bfffff
        animation = self.value(root) & 0xffffff
        assert self.span(animation, 2) == bytes((2, 192))
        component = animation + self.value(animation + 2, 2)
        shape = self.value(component, 2)
        assert shape & 32768
        shapes = self.value(self.value(0x260028) + graphics) + (shape & 32767)
        count = 1
        while not self.value(shapes + 2 * (count - 1), 1) & 128:
            count += 1
            assert count <= 48
        pointer = shapes + count * 2
        pixels = self.value(self.value(pointer))
        dx, dy = (int.from_bytes(self.span(component + n, 1), 'little', signed=True) for n in (2, 3))
        sprites, palettes = bytearray(256), bytearray(64)
        used = 0
        for index in range(count):
            for byte, value in enumerate(self.span(pixels + self.value(pointer + 4 + index * 2, 2) * 16, 16)):
                tiles[(0x1d0 + index) * 16 + byte] = value
            attr, xy = self.value(shapes + index * 2, 1), self.value(shapes + index * 2 + 1, 1)
            x, y = 80 + dx + (xy & 15) * 8, 28 + dy + (xy >> 4) * 8
            if not visible or not (0 <= x + 7 < 166 and 0 <= y + 7 < 158):
                continue
            word = (0x1d0 + index) | (self.value(0x2197f9, 1) & 3) << 11 | (attr & 240) << 9
            sprites[4 * used:4 * used + 4] = bytes((word & 255, word >> 8 & 255, x & 255, y & 255))
            palettes[used] = attr & 15
            used += 1
        return sprites, palettes, count


def source_state(rom, ram, words, timeline):
    """Apply original queued/direct writes, with independently known head delays."""
    c = Cartridge(rom)
    tiles, maps, sprites, palettes = title_state(rom, ram, words, timeline, writes_only=True)
    update, start, drawing, complete = words[19], timeline[64], timeline[70], timeline[66]
    counts = {}
    if update >= drawing:
        for y in range(19):
            for x in range(40):
                maps[y * 64 + x] = 0
    if update >= start:
        counts['font'] = c.stamp(tiles, maps, 0xb4, 0x90, 0x4c0, 0xe0)
    if update >= drawing:
        counts['footer'] = c.stamp(tiles, maps, 0x2c, 0x70, 0x3c0, 0x190)
    if update >= drawing + 1:
        counts['header'] = c.stamp(tiles, maps, 0x2c, c.value(0x223b55, 2), 0, 0x180)
        course = words[51]
        counts['course_picture'] = c.stamp(tiles, maps, 4 * (course + 12), 8, 0x8c0, 0x120)
        counts['course_name'] = c.stamp(tiles, maps, 4 * (course + 12), 12, 0x218, 0x1ae)
        # Fresh score text service copies live font map glyphs, after that head.
        output = 0x450
        for index in range(64):
            char = c.value(0x21b282 + index, 1)
            if char == 255:
                break
            glyph = (char - 48) & 65535
            source = 0x4c0 + 4 * (glyph & 15) + 8 * (glyph & 240)
            for y in range(2):
                for x in range(2):
                    at = source + y * 64 + x * 2
                    value = ((maps[at] + 256 * maps[at + 1]) & 511) | 0x3600
                    dest = output + y * 64 + x * 2
                    maps[dest], maps[dest + 1] = value & 255, value >> 8
            output += 4
        else:
            raise AssertionError('Unterminated original score text')
    if update >= drawing + 2:
        golfer = 0  # This actual target starts normal golfer0, with no save restore.
        offset = c.value(0x223b6d + 2 * golfer, 2)
        counts['golfer_name'] = c.stamp(tiles, maps, 0x2c, offset, 0x18c, 0x40)
        counts['golfer_picture'] = c.stamp(tiles, maps, 0x28, offset, 0xb0c, 0x80)
        counts['label0'] = c.stamp(tiles, maps, 0x2c, 0x30, 0x240, 0x90)
        counts['label1'] = c.stamp(tiles, maps, 0x28, 0x30, 0xb00, 0xd0)
    shape_count = 0
    # Blink child starts at start+1, decrements30 immediately, creates its object
    # at start+31. Sprite/tile writes become visible at the next drawing head.
    if update >= start + 32 and complete > start + 31:
        visible = ((update - start - 32) // 5) % 2 == 0 and update <= complete
        sprites, palettes, shape_count = c.object(tiles, visible)
    elif update >= start:
        sprites, palettes = bytearray(256), bytearray(64)
    for offset, value in tiles.items():
        assert ram[words[22] + offset] == value, ('Round source tile byte', offset, update)
    for offset, value in maps.items():
        assert ram[words[23] + offset] == value, ('Round source map byte', offset, update)
    assert ram[words[24]:words[24] + 256] == sprites, ('Round source sprite VRAM', update)
    assert ram[words[25]:words[25] + 64] == palettes, ('Round source sprite palettes', update)
    metadata = struct.unpack_from('>16I', ram, words[69])
    phase = 4 if update >= complete else 3 if update >= start + 2 else 2 if update == start + 1 else 1
    assert metadata[0] == words[68] == phase
    width = 160 if start + 2 <= update < complete else 0
    assert ram[words[27]:words[27] + 13] == bytes((0, 0, 0, 0, 0, 0, 0, 0, width, 152, 0, 129, 7)), 'Original round scene controls'
    if start + 31 <= update < complete:
        assert metadata[1] == 2 + ((update - start - 31) // 5) % 2
        assert metadata[14] == 1
        assert metadata[4:7] == (80, 28, 0x1d0)
        assert metadata[8:13] == (0, 3, 33, 32, 0)
    return {'verified_tile_bytes': len(tiles), 'verified_map_bytes': len(maps),
            'stamp_tiles_and_rows': counts, 'blink_shape_tiles': shape_count,
            'snapshot_round_phase': metadata[0], 'snapshot_blink_phase': metadata[1]}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--directory', type=Path, required=True)
    p.add_argument('--scenario', choices=SCENARIOS, required=True)
    p.add_argument('--rom', type=Path, default=Path(__file__).resolve().parents[2]/'private-inputs/neo-turf-masters.ngc')
    a = p.parse_args()
    log = (a.directory/'run.log').read_text()
    for setting in ['cpu_model=68020', 'chipmem_size=4', 'bogomem_size=0', 'chipset=aga', 'ntsc=false', 'Model: "A1200OG".']:
        assert setting in log, setting
    assert all(line == 'fastmem_size=0' for line in log.splitlines() if line.startswith('fastmem_size='))
    assert 'ROUND stage=8' in log
    rom = a.rom.read_bytes()
    final = (a.directory/'chip.finished.bin').read_bytes()
    timeline = struct.unpack_from('>128I', final, final.index(b'NRND0001'))
    labels = list(dict.fromkeys(re.findall(r'^STAGE ([^ ]+) ', log, re.M)))
    results = {}
    for label in labels:
        path = a.directory/f'chip.{label}.bin'
        ram = path.read_bytes()
        words = struct.unpack_from('>128I', ram, ram.index(b'NRND0001'))
        update = words[19]
        intro = a.scenario != 'cancel' and update >= timeline[53] + 1
        round_started = a.scenario != 'cancel' and update >= timeline[64]
        palette = title_palette(rom, words[51]) if intro else palette_from_cartridge(rom, a.scenario != 'cancel' and update >= timeline[52])
        if round_started and update >= timeline[64] + 1:
            palette = Cartridge(rom).palette(palette)
        results[label] = verify(a.directory/f'screen.{label}.ppm', path, palette, b'NRND0001', 512)
        results[label].update(snapshot_title_phase=words[62], live_title_phase=words[61],
                              snapshot_round_phase=words[68], live_round_phase=words[67], snapshot_update=update)
        if label in ('initial', 'right', 'left', 'japan', 'return', 'repeat', 'return2', 'chosen'):
            assert words[46] < 15
        if intro:
            assert words[46] == 0xffffffff
            results[label]['source_state'] = source_state(rom, ram, words, timeline) if round_started else title_state(rom, ram, words, timeline)
        if label == 'finished':
            assert words[9] == 0 and words[10] == (252 if a.scenario == 'cancel' else 0)
            assert bool(words[38]) == (a.scenario == 'random')
            if a.scenario == 'cancel':
                assert words[48] == 2 and words[64:68] == (0, 0, 0, 0)
            else:
                if a.scenario[-1].isdigit():
                    assert words[51] == int(a.scenario[-1])
                assert words[48] == 5 and words[67] == 4
                assert words[53] == words[52] + 1 and words[54] == words[53] + 2
                assert words[58] == words[54] + 20 and words[59] == words[63] + 2
                if a.scenario.startswith('skip'):
                    assert 'TITLE_SKIP ' in log and 0 < words[63] - words[58] < 60
                else:
                    assert words[63] - words[58] == 60
                assert words[60] - words[59] < 60 if a.scenario == 'skipboth' else words[60] - words[59] == 60
                assert words[64] == words[60] + 1 and words[65] == words[64] + 2
                assert words[70] == words[64]
                assert words[7] == words[66] + 1
                assert words[55:57] == (1, 255)
                assert ram[words[27] + 6:words[27] + 10] == bytes((0, 0, 0, 152))
                skip = a.scenario.startswith('recordskip') or a.scenario == 'recordheld'
                assert (0 < words[66] - words[65] < 240) if skip else words[66] - words[65] == 240
                assert ('ROUND_SKIP ' in log) == skip
    assert [results[x]['displayed_selection'] for x in ('initial', 'right', 'left', 'japan', 'return', 'repeat', 'return2')] == [1, 2, 1, 0, 1, 2, 1]
    presentations = []
    pattern = r'PRESENTED_INTRO label=(\S+) frame=(\d+) snapshot_update=(\d+) current_update=(\d+) title_phase=(\d+) live_title_phase=(\d+) round_phase=(\d+) live_round_phase=(\d+) composition_ticks=(\d+)'
    for match in re.finditer(pattern, log):
        item = dict(zip(('frame', 'snapshot_update', 'current_update', 'title_phase', 'live_title_phase', 'round_phase', 'live_round_phase', 'composition_ticks'), map(int, match.groups()[1:])))
        item['label'] = match.group(1)
        item['composition_seconds'] = item['composition_ticks'] / 709379
        presentations.append(item)
    accept_label = 'chosen' if 'chosen' in labels else 'return2'
    accept_frame = int(re.search(r'^STAGE ' + accept_label + r' frame=(\d+)', log, re.M).group(1))
    stages = {}
    for name in ('scenery', 'title', 'tournament', 'round_record', 'closed_boundary'):
        def included(item):
            u = item['snapshot_update']
            if name == 'scenery':
                return timeline[54] <= u < timeline[58]
            if name == 'title':
                return timeline[58] + 1 <= u < timeline[63]
            if name == 'tournament':
                return timeline[63] + 1 <= u < timeline[60]
            if name == 'round_record':
                return timeline[65] <= u < timeline[66] and item['round_phase'] == 3
            return item['round_phase'] == 4
        shown = [item for item in presentations if a.scenario != 'cancel' and included(item)]
        stages[name] = None if not shown else dict(shown[0], accept_to_verified_seconds_nominal_pal=(shown[0]['frame'] - accept_frame) * .02,
                                                 late_by_logical_updates=shown[0]['current_update'] - shown[0]['snapshot_update'])
    if a.scenario.startswith('course') or a.scenario == 'random':
        assert stages['round_record'] is not None, 'Ordinary flow must actually present round-record'
    timing = {'accept_input_frame': accept_frame, 'first_verified_stages': stages,
              'logical_updates': dict(zip(('selector_exit', 'intro_start', 'scenery_open', 'title_start', 'tournament_open', 'title_complete', 'tournament_prepare', 'round_start', 'round_open', 'round_complete', 'round_drawing'),
                                         [timeline[i] for i in (52, 53, 54, 58, 59, 60, 63, 64, 65, 66, 70)])),
              'skip_input_events': re.findall(r'^(?:TITLE|TOURNAMENT|ROUND)_SKIP .*$', log, re.M)}
    result = {'result': 'PASS', 'scenario': a.scenario, 'model': 'PAL A1200 68EC020 AGA, 2 MiB Chip, zero Fast',
              'captures': results, 'presentations': presentations, 'timing': timing,
              'checks': ['actual hardware joystick normal/random/cancel/skip paths', 'scheduled selector/title/round-record children through tee-shot request',
                         'all bitplane bytes and all RGB pixels independently composed', '512 source palette bytes independently resolved',
                         'cartridge-derived round tiles/maps/text/blink sprites checked with deferred drawing-head timing', 'zero dropped logical updates'],
              'limits': ['tee-shot and later gameplay remain unimplemented', 'normal golfer0 fresh record only on this target; no save restoration',
                         'immutable presentation can lag or miss logical phases', 'no audio, physical hardware, full-speed or playable-golf claim',
                         'hardware takeover until emulator reset']}
    (a.directory/'verification.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
