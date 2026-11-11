#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Stock PAL selector/title/tournament target checks, with private local captures.

Render oracle is shared with selector verification; resource resolution below is
independent of the exporter and native queues. No round-record/golf claim.
"""
import argparse
import json
import re
import struct
from pathlib import Path
from selector_verify import palette_from_cartridge, verify


def source_palette(rom, course):
    palette = bytearray(palette_from_cartridge(rom, True))
    def span(address, size):
        offset = address - 0x200000
        assert 0 <= offset <= len(rom) - size
        return rom[offset:offset + size]
    def value(address, size=4):
        return int.from_bytes(span(address, size), 'little')
    for command in ((course + 53) * 4, (course + 56) * 4):
        entry = value(0x2198f3 + command)
        kind, source = entry & 255, entry >> 8
        if kind == 4:
            count, destination = value(source, 1), value(source + 1, 1) * 8
            assert count > 0 and destination + count * 8 <= 512
            offsets, colors = value(source + 2), value(0x26001c)
            for group in range(count):
                palette[destination + group * 8:destination + (group + 1) * 8] = span(colors + value(offsets + group * 2, 2), 8)
        else:
            assert kind == 32
            destination = 480 + value(source, 1)
            palette[destination:destination + 2] = span(source + 1, 2)
    return palette


def source_state(rom, ram, words, timeline):
    """Decode the original resources independently and replay only their writes.

    Native phase is observed, never changed. Update markers distinguish queued
    writes from writes consumed by the next ordinary drawing head.
    """
    def span(address, size):
        offset = address - 0x200000
        assert size >= 0 and 0 <= offset <= len(rom) - size
        return rom[offset:offset + size]
    def value(address, size=4):
        return int.from_bytes(span(address, size), 'little')
    course, update = words[51], words[19]
    descriptor = value(0x21b17d + course * 4)
    group = value(value(0x260020) + value(descriptor, 2)) & 0xffffff
    tiles, maps = {}, {}
    def clear(destination, width, rows):
        for y in range(rows):
            for x in range(width * 2):
                maps[destination + y * 64 + x] = 0
    def stamp(index, destination, tile):
        source = value(group + index * 4)
        count, pixels = value(source, 2), value(source + 2)
        assert 0 < count <= 512 and tile + count <= 512
        for i in range(count):
            for j, byte in enumerate(span(pixels + value(source + 6 + i * 2, 2), 16)):
                tiles[(tile + i) * 16 + j] = byte
        pointer = source + 6 + count * 2
        rows = value(pointer, 1)
        pointer += 1
        assert rows & 128
        for y in range(rows & 127):
            x = 0
            while True:
                entry = value(pointer, 2)
                pointer += 2
                if entry == 65535:
                    break
                expected = (entry + tile) & 65535
                dest = destination + 64 * y + 2 * x
                maps[dest], maps[dest + 1] = expected & 255, expected >> 8
                x += 1
                assert x <= 32
        return count, rows & 127
    clear(0, 20, 19)
    background = stamp(0, 2048, value(descriptor + 2, 2))
    title_at, prepare_at = timeline[58], timeline[63]
    sprite_bytes, sprite_palettes = bytearray(256), bytearray(64)
    title_count = 0
    if update >= title_at + 1:
        stamp(3, 0x188, value(descriptor + 4, 2))
        graphics = value(descriptor + 8, 2)
        root = value(value(0x260024) + graphics) & 0x00bfffff
        animation = value(root) & 0xffffff
        assert span(animation, 2) == bytes((1, 192))
        component = animation + value(animation + 2, 2)
        shape = value(component, 2)
        assert shape & 0x8000
        shapes = value(value(0x260028) + graphics) + (shape & 32767)
        while not value(shapes + title_count * 2, 1) & 128:
            title_count += 1
            assert title_count < 64
        title_count += 1
        pointer = shapes + title_count * 2
        pixels = value(value(pointer))
        tile = value(descriptor + 6, 2)
        dx, dy = (int.from_bytes(span(component + n, 1), 'little', signed=True) for n in (2, 3))
        used = 0
        for i in range(title_count):
            for j, byte in enumerate(span(pixels + value(pointer + 4 + i * 2, 2) * 16, 16)):
                tiles[(tile + i) * 16 + j] = byte
            attr, xy = value(shapes + i * 2, 1), value(shapes + i * 2 + 1, 1)
            x, y = 40 + dx + (xy & 15) * 8, 48 + dy + (xy >> 4) * 8
            if not (0 <= x + 7 < 166 and 0 <= y + 7 < 158):
                continue
            word = (tile + i) | (value(0x219812, 1) & 3) << 11 | (attr & 240) << 9
            sprite_bytes[4 * used:4 * used + 4] = bytes((word & 255, (word >> 8) & 255, x & 255, y & 255))
            sprite_palettes[used] = attr & 15
            used += 1
    if update >= prepare_at:
        clear(0x188, 12, 8)
    if update >= prepare_at + 1:
        stamp(2, 0x180, value(descriptor + 10, 2))
        sprite_bytes, sprite_palettes = bytearray(256), bytearray(64)
    for offset, byte in tiles.items():
        assert ram[words[22] + offset] == byte, ('Source tile byte', offset)
    for offset, byte in maps.items():
        assert ram[words[23] + offset] == byte, ('Source map byte', offset)
    assert ram[words[24]:words[24] + 256] == sprite_bytes, 'Source title/cleared sprite VRAM mismatch'
    assert ram[words[25]:words[25] + 64] == sprite_palettes, 'Source title/cleared sprite palettes mismatch'
    metadata = struct.unpack_from('>16I', ram, words[57])
    if metadata[0] == 1 and update > title_at + 1:
        assert metadata[2:4] == (0, 0) and metadata[4:14] == (40, 48, value(descriptor + 6, 2), 66, 0, 3, 38, value(descriptor + 8, 2), 0, 64)
    return {'background_tiles': background[0], 'background_rows': background[1], 'title_shape_tiles': title_count,
            'verified_tile_bytes': len(tiles), 'verified_map_bytes': len(maps), 'snapshot_title_phase': metadata[0]}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--directory', type=Path, required=True)
    p.add_argument('--scenario', choices=['course0', 'course1', 'course2', 'random', 'cancel', 'skip0', 'skip1', 'skip2', 'skipboth'], required=True)
    p.add_argument('--rom', type=Path, default=Path(__file__).resolve().parents[2]/'private-inputs/neo-turf-masters.ngc')
    a = p.parse_args()
    log = (a.directory/'run.log').read_text()
    for setting in ['cpu_model=68020', 'chipmem_size=4', 'bogomem_size=0', 'chipset=aga', 'ntsc=false', 'Model: "A1200OG".']:
        assert setting in log, setting
    assert all(x == 'fastmem_size=0' for x in log.splitlines() if x.startswith('fastmem_size='))
    assert 'INTRO stage=8' in log
    rom = a.rom.read_bytes()
    final = (a.directory/'chip.finished.bin').read_bytes()
    timeline = struct.unpack_from('>64I', final, final.index(b'NINT0001'))
    results = {}
    # Read only this run's explicit captures, never leftover files from an older run.
    labels = list(dict.fromkeys(re.findall(r'^STAGE ([^ ]+) ', log, re.M)))
    for label in labels:
        chip = a.directory/f'chip.{label}.bin'
        ram = chip.read_bytes()
        words = struct.unpack_from('>64I', ram, ram.index(b'NINT0001'))
        update = words[19]
        intro = a.scenario != 'cancel' and update >= timeline[53] + 1
        completed = a.scenario != 'cancel' and update >= timeline[52]
        palette = source_palette(rom, words[51]) if intro else palette_from_cartridge(rom, completed)
        results[label] = verify(a.directory/f'screen.{label}.ppm', chip, palette, b'NINT0001')
        results[label].update(snapshot_title_phase=words[62], live_title_phase=words[61], snapshot_update=update)
        if label in ('initial', 'right', 'left', 'japan', 'return', 'repeat', 'return2', 'chosen'):
            assert words[46] < 15, 'Full selector must use an exact cache hit'
        if intro:
            assert words[46] == 0xffffffff, 'New scenes must use general renderer'
            results[label]['source_state'] = source_state(rom, ram, words, timeline)
        if label == 'finished':
            assert words[9] == 0 and words[10] == (252 if a.scenario == 'cancel' else 0)
            assert (words[38] > 0) == (a.scenario == 'random')
            if a.scenario != 'cancel':
                if a.scenario.startswith(('course', 'skip')) and a.scenario[-1].isdigit():
                    assert words[51] == int(a.scenario[-1])
                assert words[48:51] == (4, 4, 3) and words[61] == 4
                assert words[53] == words[52] + 1 and words[54] == words[53] + 2
                assert words[58] == words[54] + 20 and words[59] == words[63] + 2
                assert words[7] == words[60] + 1
                assert words[55:57] == (1, 255)
                assert ram[words[27] + 6:words[27] + 10] == bytes((0, 0, 0, 152))
                if a.scenario.startswith('skip'):
                    assert 'TITLE_SKIP ' in log and 0 < words[63] - words[58] < 60
                else:
                    assert words[63] - words[58] == 60
                assert words[60] - words[59] < 60 if a.scenario == 'skipboth' else words[60] - words[59] == 60
            else:
                assert words[48] == 2 and words[53:57] == (0, 0, 0, 0)
            results[label].update(boundary=words[48], mapped_course=words[51], selector_exit_update=words[52], intro_start_update=words[53], first_visible_update=words[54], title_start_update=words[58], tournament_start_update=words[59], title_complete_update=words[60], golfer_count=words[55])
    assert [results[x]['displayed_selection'] for x in ('initial', 'right', 'left', 'japan', 'return', 'repeat', 'return2')] == [1, 2, 1, 0, 1, 2, 1]
    presentations = []
    for m in re.finditer(r'PRESENTED_INTRO label=(\S+) frame=(\d+) snapshot_update=(\d+) current_update=(\d+) title_phase=(\d+) live_title_phase=(\d+) composition_ticks=(\d+)', log):
        values = list(map(int, m.groups()[1:]))
        item = dict(zip(('frame', 'snapshot_update', 'current_update', 'title_phase', 'live_title_phase', 'composition_ticks'), values))
        item['label'] = m.group(1)
        item['composition_seconds'] = item['composition_ticks'] / 709379
        presentations.append(item)
    accept_label = 'chosen' if 'chosen' in labels else 'return2'
    accept_frame = int(re.search(r'^STAGE ' + accept_label + r' frame=(\d+)', log, re.M).group(1))
    stages = {}
    for name, phase in [('scenery', 0), ('title', 1), ('tournament', 3), ('closed_boundary', 4)]:
        shown = [item for item in presentations if
                 (phase == 0 and item['title_phase'] == 0 and timeline[54] <= item['snapshot_update'] < timeline[58]) or
                 (phase == 1 and item['title_phase'] == 1 and item['snapshot_update'] >= timeline[58] + 1) or
                 (phase == 3 and item['title_phase'] in (2, 3) and item['snapshot_update'] >= timeline[63] + 1) or
                 (phase == 4 and item['title_phase'] == 4)]
        stages[name] = None if not shown else dict(shown[0], accept_to_verified_seconds_nominal_pal=(shown[0]['frame'] - accept_frame) * .02,
                                                late_by_logical_updates=shown[0]['current_update'] - shown[0]['snapshot_update'])
    if a.scenario.startswith('course'):
        assert stages['title'] is not None and stages['tournament'] is not None, 'Ordinary flow must actually present title and tournament'
    if a.scenario == 'random':
        assert all(stages[name] is not None for name in ('scenery', 'title', 'tournament')), 'Random flow must actually present all three stages'
    timing = {'accept_input_frame': accept_frame, 'first_verified_stages': stages,
              'logical_updates': dict(zip(('selector_exit', 'intro_start', 'scenery_open', 'title_start', 'tournament_open', 'child_complete', 'tournament_prepare'),
                                         [timeline[i] for i in (52, 53, 54, 58, 59, 60, 63)])),
              'skip_input_events': re.findall(r'^(?:TITLE|TOURNAMENT)_SKIP .*$', log, re.M)}
    result = {'result': 'PASS', 'scenario': a.scenario, 'model': 'PAL A1200 68EC020 AGA, 2 MiB Chip, zero Fast', 'captures': results, 'presentations': presentations, 'timing': timing,
              'checks': ['hardware joystick normal/random/cancel/skip path', 'real scheduled selector, owner and title child through round-record boundary', 'every bitplane and RGB capture pixel independently composed', '512 source palette bytes independently resolved', 'course/title/tournament source tiles/maps and object sprites independently decoded', 'zero dropped logical updates'],
              'limits': ['round-record and tee-shot remain unimplemented', 'default normal golfer0 record, no save restoration', 'immutable presentation can lag or miss logical phases; see actual presented snapshot phases', 'no audio, physical hardware or full-speed claim', 'hardware takeover until emulator reset']}
    (a.directory/'verification.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
