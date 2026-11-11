#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Stock PAL selector-to-course-scenery target checks, with private local captures.

Render oracle is shared with selector verification; resource resolution below is
independent of the exporter and native queues. No later title/golf claim.
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


def scenery_from_cartridge(rom, ram, words):
    """Independently decode course background and verify its tile/map writes."""
    def span(address, size):
        offset = address - 0x200000
        assert 0 <= offset <= len(rom) - size
        return rom[offset:offset + size]
    def value(address, size=4):
        return int.from_bytes(span(address, size), 'little')
    course = words[51]
    descriptor = value(0x21b17d + course * 4)
    tile = value(descriptor + 2, 2)
    table = value(value(0x260020) + value(descriptor, 2)) & 0xffffff
    source = value(table)
    count, pixels = value(source, 2), value(source + 2)
    assert 0 < count <= 512 and tile + count <= 512
    for i in range(count):
        expected = span(pixels + value(source + 6 + i * 2, 2), 16)
        assert ram[words[22] + 16 * (tile + i):words[22] + 16 * (tile + i + 1)] == expected
    pointer = source + 6 + count * 2
    rows = value(pointer, 1)
    pointer += 1
    assert rows & 128  # The three course backgrounds use nonnegative local IDs.
    for y in range(rows & 127):
        x = 0
        while True:
            entry = value(pointer, 2)
            pointer += 2
            if entry == 65535:
                break
            expected = (entry + tile) & 65535
            dest = words[23] + 2048 + 64 * y + 2 * x
            assert int.from_bytes(ram[dest:dest + 2], 'little') == expected
            x += 1
            assert x <= 32
    for y in range(19):
        assert not any(ram[words[23] + 64 * y:words[23] + 64 * y + 40]), 'Selector map0 must be directly cleared'
    assert not any(ram[words[24]:words[24] + 256]), 'Stale selector cursor sprite survived display reset'
    assert not any(ram[words[25]:words[25] + 64]), 'Stale selector sprite palette survived display reset'
    return {'background_tiles': count, 'background_rows': rows & 127, 'stale_selector_sprites': 0}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--directory', type=Path, required=True)
    p.add_argument('--scenario', choices=['course0', 'course1', 'course2', 'random', 'cancel'], required=True)
    p.add_argument('--rom', type=Path, default=Path(__file__).resolve().parents[2]/'private-inputs/neo-turf-masters.ngc')
    a = p.parse_args()
    log = (a.directory/'run.log').read_text()
    for setting in ['cpu_model=68020', 'chipmem_size=4', 'bogomem_size=0', 'chipset=aga', 'ntsc=false', 'Model: "A1200OG".']:
        assert setting in log, setting
    assert all(x == 'fastmem_size=0' for x in log.splitlines() if x.startswith('fastmem_size='))
    assert 'INTRO stage=8' in log
    rom = a.rom.read_bytes()
    results = {}
    for chip in sorted(a.directory.glob('chip.*.bin')):
        label = chip.name[5:-4]
        ram = chip.read_bytes()
        marker = ram.index(b'NINT0001')
        words = struct.unpack_from('>64I', ram, marker)
        scenery = label in ('scenery', 'finished') and a.scenario != 'cancel'
        palette = source_palette(rom, words[51]) if scenery else palette_from_cartridge(rom, False)
        results[label] = verify(a.directory/f'screen.{label}.ppm', chip, palette, b'NINT0001')
        if label not in ('scenery', 'finished'):
            assert words[46] < 15, 'Full selector must use an exact cache hit'
        if label == 'scenery':
            assert words[46] == 0xffffffff
            results[label]['source_scenery'] = scenery_from_cartridge(rom, ram, words)
        if label == 'finished':
            assert words[9] == 0 and words[10] == (252 if a.scenario == 'cancel' else 0)
            assert (words[38] > 0) == (a.scenario == 'random')
            if scenery:
                assert words[46] == 0xffffffff, 'Scenery must use the general renderer, not a selector cache entry'
                if a.scenario.startswith('course'):
                    assert words[51] == int(a.scenario[-1])
                assert words[48:51] == (1, 3, 3)  # title boundary, owner wait, intro title pending
                assert words[53] == words[52] + 1 and words[54] == words[53] + 2
                assert words[55:57] == (1, 255)
                assert ram[words[27] + 6:words[27] + 10] == bytes((0, 0, 160, 152))
                results[label]['source_scenery'] = scenery_from_cartridge(rom, ram, words)
            else:
                assert words[48] == 2 and words[53:57] == (0, 0, 0, 0)
            results[label].update(boundary=words[48], mapped_course=words[51], selector_exit_update=words[52], intro_start_update=words[53], first_visible_update=words[54], golfer_count=words[55])
    assert [results[x]['displayed_selection'] for x in ('initial', 'right', 'left', 'japan', 'return', 'repeat', 'return2')] == [1, 2, 1, 0, 1, 2, 1]
    latency = None
    if a.scenario != 'cancel':
        m = re.search(r'INTRO_LATENCY input_frame=(\d+) first_scenery_report_frame=(\d+) verified_scenery_frame=(\d+) reported_frames=(\d+) verified_frames=(\d+) snapshot_update=(\d+) current_update=(\d+) first_visible_update=(\d+) running=(\d+) composition_ticks=(\d+)', log)
        assert m and 'scenery' in results
        fields = ('input_frame', 'first_scenery_report_frame', 'verified_scenery_frame', 'reported_frames', 'verified_frames', 'snapshot_update', 'current_update', 'first_visible_update', 'running', 'composition_ticks')
        latency = dict(zip(fields, map(int, m.groups())))
        latency['verified_capture_seconds_nominal_pal'] = latency['verified_frames'] * 0.02
        latency['first_scenery_composition_seconds'] = latency['composition_ticks'] / 709379
    result = {'result': 'PASS', 'first_scenery_latency': latency, 'scenario': a.scenario, 'model': 'PAL A1200 68EC020 AGA, 2 MiB Chip, zero Fast', 'captures': results,
              'checks': ['hardware joystick normal/random/cancel path', 'real scheduled selector parent, outer owner and newly allocated intro child', 'every bitplane and RGB capture pixel independently composed', '512 source palette bytes independently resolved', 'course source tiles/maps independently decoded', 'empty display list clears selector cursor via deferred object service'],
              'limits': ['bounded course scenery only; title object, round-record and tee-shot remain unimplemented', 'default normal golfer0 record, no save restoration', 'composition can lag logical intro timing; final scenery is verified after halting at the title boundary, not a timed title animation', 'no audio, physical hardware or full-speed claim', 'hardware takeover until emulator reset']}
    (a.directory/'verification.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
