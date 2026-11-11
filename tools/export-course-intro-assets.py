#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Resolve three bounded course-scenery backgrounds and six private palette jobs.

Only source metadata and the extraction procedure are public. All generated
cartridge bytes remain under this checkout's ignored build/ directory.
"""
import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def export(rom, output):
    output = output.resolve()
    if not output.is_relative_to((ROOT / 'build').resolve()):
        raise ValueError('Private generated assets must stay under ignored build/')
    def span(address, size):
        offset = address - 0x200000
        if size < 0 or offset < 0 or offset + size > len(rom):
            raise ValueError('Resolved cartridge span outside input')
        return rom[offset:offset + size]
    def value(address, size=4):
        return int.from_bytes(span(address, size), 'little')
    arrays = {}
    bindings = []
    for course in range(3):
        prefix = f'intro_{course}'
        descriptor = value(0x21b17d + course * 4)
        arrays[prefix + '_descriptor'] = span(descriptor, 4)
        group = value(value(0x260020) + value(descriptor, 2)) & 0xffffff
        source = value(group)
        count = value(source, 2)
        if not 0 < count <= 512:
            raise ValueError('Unsupported tile count')
        end = source + 6 + 2 * count
        rows = value(end, 1) & 127
        end += 1
        if not 0 < rows <= 32:
            raise ValueError('Unsupported map rows')
        for _ in range(rows):
            for _ in range(33):
                word = value(end, 2)
                end += 2
                if word == 65535:
                    break
            else:
                raise ValueError('Unterminated map row')
        size = max(value(source + 6 + 2 * i, 2) for i in range(count)) + 16
        arrays[prefix + '_map'] = span(source, end - source)
        arrays[prefix + '_tiles'] = span(value(source + 2), size)
        bindings += [f'r[{course}].descriptor={prefix}_descriptor;r[{course}].descriptor_size=sizeof {prefix}_descriptor;',
                     f'r[{course}].background.data={prefix}_map;r[{course}].background.size=sizeof {prefix}_map;',
                     f'r[{course}].background.tiles={prefix}_tiles;r[{course}].background.tiles_size=sizeof {prefix}_tiles;']
        for index in range(2):
            packed = value(0x2198f3 + (course + 0x35) * 4 + index * 12)
            kind, descriptor = packed & 255, packed >> 8
            job = f'r[{course}].palettes[{index}]'
            if index == 0:
                if kind != 4:
                    raise ValueError('Expected original group-palette service')
                count = value(descriptor, 1)
                destination = value(descriptor + 1, 1) * 8
                if not 0 < count <= 64 or destination + count * 8 > 512:
                    raise ValueError('Unsupported palette group')
                offsets = span(value(descriptor + 2), count * 2)
                size = max(int.from_bytes(offsets[i:i+2], 'little') for i in range(0, len(offsets), 2)) + 8
                arrays[prefix + '_offsets'] = offsets
                arrays[prefix + '_colors'] = span(value(0x26001c), size)
                bindings += [f'{job}.kind=1;{job}.count={count};{job}.offset={destination};',
                             f'{job}.offsets={prefix}_offsets;{job}.offsets_size=sizeof {prefix}_offsets;',
                             f'{job}.colors={prefix}_colors;{job}.colors_size=sizeof {prefix}_colors;']
            else:
                if kind != 32:
                    raise ValueError('Expected original single-color service')
                offset = 480 + value(descriptor, 1)
                control = value(descriptor + 3, 2)
                pending = value(descriptor + 5, 1)
                if offset > 510 or offset & 1 or control not in (0x118, 0x12) or pending > 1:
                    raise ValueError('Unsupported single-color destination')
                bindings.append(f'{job}.kind=2;{job}.offset={offset};{job}.color={value(descriptor+1,2)};{job}.control_offset={control};{job}.pending_index={pending};')
    lines = ['/* PRIVATE GENERATED CARTRIDGE ASSETS. Do not commit or distribute. */']
    for name, data in arrays.items():
        lines.append(f'static const uint8_t {name}[{len(data)}]={{')
        for at in range(0, len(data), 24):
            lines.append(','.join(str(x) for x in data[at:at + 24]) + ',')
        lines.append('};')
    lines += ['static void intro_resources(ntm_course_intro_resources r[3]) {'] + bindings + ['}']
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('\n'.join(lines) + '\n')
    print(f'Exported {len(arrays)} bounded private intro spans to ignored build directory.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROOT/'private-inputs/neo-turf-masters.ngc')
    parser.add_argument('--output', type=Path, default=ROOT/'build/native-intro/intro_assets.inc')
    args = parser.parse_args()
    export(args.rom.read_bytes(), args.output)
