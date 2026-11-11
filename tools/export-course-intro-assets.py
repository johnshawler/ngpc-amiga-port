#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Resolve bounded course scenery, title objects, tournament stamps and palettes.

Only source metadata and the extraction procedure are public. All generated
cartridge bytes remain under this checkout's ignored build/ directory.
"""
import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def export(rom, output, include_round=False):
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
    title_bindings = []
    def asset(prefix, source):
        count = value(source, 2)
        if not 0 < count <= 512:
            raise ValueError('Unsupported tile count')
        end = source + 6 + 2 * count
        header = value(end, 1)
        rows, stride = header & 127, 2 if header & 128 else 1
        end += 1
        if not 0 < rows <= 32:
            raise ValueError('Unsupported map rows')
        widths = []
        for _ in range(rows):
            for column in range(33):
                word = value(end, stride)
                end += stride
                if word == (65535 if stride == 2 else 255):
                    if not column:
                        raise ValueError('Empty map row')
                    widths.append(column)
                    break
            else:
                raise ValueError('Unterminated map row')
        size = max(value(source + 6 + 2 * i, 2) for i in range(count)) + 16
        arrays[prefix + '_map'] = span(source, end - source)
        arrays[prefix + '_tiles'] = span(value(source + 2), size)
        return widths
    def bind_asset(target, prefix):
        return [f'{target}.data={prefix}_map;{target}.size=sizeof {prefix}_map;',
                f'{target}.tiles={prefix}_tiles;{target}.tiles_size=sizeof {prefix}_tiles;']
    arrays['intro_title_metadata'] = span(0x21980e, 5)
    for course in range(3):
        prefix = f'intro_{course}'
        descriptor = value(0x21b17d + course * 4)
        arrays[prefix + '_descriptor'] = span(descriptor, 12)
        group = value(value(0x260020) + value(descriptor, 2)) & 0xffffff
        asset(prefix, value(group))
        if value(descriptor + 2, 2) + value(value(group), 2) > 512:
            raise ValueError('Background exceeds destination tile memory')
        bindings += [f'r[{course}].descriptor={prefix}_descriptor;r[{course}].descriptor_size=sizeof {prefix}_descriptor;']
        bindings += bind_asset(f'r[{course}].background', prefix)
        title_bindings += [f'r[{course}].descriptor={prefix}_descriptor;r[{course}].descriptor_size=sizeof {prefix}_descriptor;',
                           f'r[{course}].metadata=intro_title_metadata;r[{course}].metadata_size=sizeof intro_title_metadata;']
        for name, index, member in [('title', 3, 'title'), ('tournament', 2, 'tournament[0]'), ('mode2', 4, 'tournament[1]')]:
            label = prefix + '_' + name
            asset(label, value(group + index * 4))
            tile = value(descriptor + (4 if name == 'title' else 10), 2)
            if tile + value(value(group + index * 4), 2) > 512:
                raise ValueError('Stamp exceeds destination tile memory')
            title_bindings += bind_asset(f'r[{course}].{member}', label)
        # The title's verified one-frame, terminal (0xc0) animation has one
        # component. Resolve every referenced shape/tile, never an open ROM span.
        graphics = value(descriptor + 8, 2)
        if graphics not in (20, 24, 28):
            raise ValueError('Unsupported title graphics index')
        root = value(value(0x260024) + graphics) & 0x00bfffff
        animation = value(root) & 0xffffff
        if value(animation, 1) != 1 or value(animation + 1, 1) != 0xc0:
            raise ValueError('Unsupported title animation header')
        component = value(animation + 2, 2)
        shape_word = value(animation + component, 2)
        if not shape_word & 0x8000:
            raise ValueError('Expected single-component title animation')
        shape = shape_word & 0x7fff
        shapes = value(value(0x260028) + graphics)
        for count in range(1, 65):
            if value(shapes + shape + (count - 1) * 2, 1) & 0x80:
                break
        else:
            raise ValueError('Unterminated title shape')
        if value(descriptor + 6, 2) + count > 512:
            raise ValueError('Title object exceeds destination tile memory')
        pointer = shapes + shape + 2 * count
        tiles = value(value(pointer))
        size = max(value(pointer + 4 + i * 2, 2) for i in range(count)) * 16 + 16
        arrays[prefix + '_animation'] = span(animation, max(4, component + 4))
        arrays[prefix + '_shapes'] = span(shapes, shape + 4 * count + 4)
        arrays[prefix + '_object_tiles'] = span(tiles, size)
        title_bindings += [f'o[{course}].animations[0].data={prefix}_animation;o[{course}].animations[0].size=sizeof {prefix}_animation;o[{course}].animations[0].frames=1;',
                           f'o[{course}].shapes={prefix}_shapes;o[{course}].shapes_size=sizeof {prefix}_shapes;',
                           f'o[{course}].tiles={prefix}_object_tiles;o[{course}].tiles_size=sizeof {prefix}_object_tiles;']
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
    round_bindings = []
    if include_round:
        def round_asset(name, group_offset, entry, destination_tile, destination_map):
            group = value(value(0x260020) + group_offset) & 0xffffff
            source = value(group + entry)
            if destination_tile + value(source, 2) > 512:
                raise ValueError('Round stamp exceeds destination tile memory')
            prefix = 'round_' + name.replace('[', '_').replace(']', '')
            widths = asset(prefix, source)
            if any(destination_map + row * 64 + width * 2 > 4096 for row, width in enumerate(widths)):
                raise ValueError('Round stamp exceeds destination map memory')
            round_bindings.extend(bind_asset('r->' + name, prefix))
        round_asset('font', 0xb4, 0x90, 0xe0, 0x4c0)
        round_asset('header', 0x2c, value(0x223b55, 2), 0x180, 0)
        round_asset('footer', 0x2c, 0x70, 0x190, 0x3c0)
        for course in range(3):
            for index, entry, tile in ((0, 8, 0x120), (1, 12, 0x1ae)):
                round_asset(f'course[{course}][{index}]', 4 * (course + 12), entry, tile, 0x218 if index else 0x8c0)
        for golfer in range(6):
            for index, group, tile in ((0, 0x2c, 0x40), (1, 0x28, 0x80)):
                round_asset(f'golfer[{golfer}][{index}]', group, value(0x223b6d + 2 * golfer, 2), tile, 0xb0c if index else 0x18c)
        for index, group in enumerate((0x2c, 0x28)):
            round_asset(f'labels[{index}]', group, 0x30, 0xd0 if index else 0x90, 0xb00 if index else 0x240)
        for size in range(1, 65):
            if value(0x21b282 + size - 1, 1) == 255:
                break
        else:
            raise ValueError('Unterminated round score string')
        arrays['round_missing_score'] = span(0x21b282, size)
        arrays['round_metadata'] = span(0x2197f5, 5)
        round_bindings += ['r->missing_score=round_missing_score;r->missing_score_size=sizeof round_missing_score;',
                           'r->metadata=round_metadata;r->metadata_size=sizeof round_metadata;']
        for index, command in enumerate((0x9c, 0xa0)):
            packed = value(0x2198f3 + command)
            kind, descriptor = packed & 255, packed >> 8
            job = f'r->palettes[{index}]'
            if index == 0:
                if kind != 4:
                    raise ValueError('Expected round group-palette service')
                count, destination = value(descriptor, 1), value(descriptor + 1, 1) * 8
                if not 0 < count <= 64 or destination + count * 8 > 512:
                    raise ValueError('Unsupported round palette group')
                offsets = span(value(descriptor + 2), count * 2)
                size = max(int.from_bytes(offsets[i:i+2], 'little') for i in range(0, len(offsets), 2)) + 8
                arrays['round_offsets'], arrays['round_colors'] = offsets, span(value(0x26001c), size)
                round_bindings += [f'{job}.kind=1;{job}.count={count};{job}.offset={destination};',
                                   f'{job}.offsets=round_offsets;{job}.offsets_size=sizeof round_offsets;',
                                   f'{job}.colors=round_colors;{job}.colors_size=sizeof round_colors;']
            else:
                if kind != 32:
                    raise ValueError('Expected round single-color service')
                offset, control, pending = 480 + value(descriptor, 1), value(descriptor + 3, 2), value(descriptor + 5, 1)
                if offset > 510 or offset & 1 or control not in (0x118, 0x12) or pending > 1:
                    raise ValueError('Unsupported round single-color destination')
                round_bindings += [f'{job}.kind=2;{job}.offset={offset};{job}.color={value(descriptor+1,2)};{job}.control_offset={control};{job}.pending_index={pending};']
        graphics = value(0x2197f5, 2)
        if graphics != 32:
            raise ValueError('Unsupported round graphics index')
        root = value(value(0x260024) + graphics) & 0x00bfffff
        animation = value(root) & 0xffffff
        if value(animation, 1) != 2 or value(animation + 1, 1) != 0xc0:
            raise ValueError('Unsupported round animation header')
        component = value(animation + 2, 2)
        shape_word = value(animation + component, 2)
        if not shape_word & 0x8000:
            raise ValueError('Expected single-component round animation')
        shape = shape_word & 0x7fff
        shapes = value(value(0x260028) + graphics)
        for count in range(1, 65):
            if value(shapes + shape + (count - 1) * 2, 1) & 0x80:
                break
        else:
            raise ValueError('Unterminated round shape')
        if 0x1d0 + count > 512:
            raise ValueError('Round object exceeds destination tile memory')
        pointer = shapes + shape + 2 * count
        tiles = value(value(pointer))
        size = max(value(pointer + 4 + i * 2, 2) for i in range(count)) * 16 + 16
        arrays['round_animation'] = span(animation, max(4, component + 4))
        arrays['round_shapes'] = span(shapes, shape + 4 * count + 4)
        arrays['round_object_tiles'] = span(tiles, size)
        round_bindings += ['o->animations[0].data=round_animation;o->animations[0].size=sizeof round_animation;o->animations[0].frames=1;',
                           'o->shapes=round_shapes;o->shapes_size=sizeof round_shapes;',
                           'o->tiles=round_object_tiles;o->tiles_size=sizeof round_object_tiles;r->object=o;']
    lines = ['/* PRIVATE GENERATED CARTRIDGE ASSETS. Do not commit or distribute. */']
    for name, data in arrays.items():
        lines.append(f'static const uint8_t {name}[{len(data)}]={{')
        for at in range(0, len(data), 24):
            lines.append(','.join(str(x) for x in data[at:at + 24]) + ',')
        lines.append('};')
    lines += ['static void intro_resources(ntm_course_intro_resources r[3]) {'] + bindings + ['}']
    lines += ['static void intro_title_resources(ntm_course_title_resources r[3], ntm_cursor_assets o[3]) {'] + title_bindings + ['}']
    if include_round:
        lines += ['static void intro_round_resources(ntm_round_record_resources *r, ntm_cursor_assets *o) {'] + round_bindings + ['}']
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('\n'.join(lines) + '\n')
    print(f'Exported {len(arrays)} bounded private intro spans to ignored build directory.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROOT/'private-inputs/neo-turf-masters.ngc')
    parser.add_argument('--output', type=Path, default=ROOT/'build/native-intro/intro_assets.inc')
    parser.add_argument('--round', action='store_true', help='Include bounded normal round-record resources')
    args = parser.parse_args()
    export(args.rom.read_bytes(), args.output, args.round)
