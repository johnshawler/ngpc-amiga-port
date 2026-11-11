#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Extract resolved selector spans from an authorized local cartridge into build/.

No cartridge bytes are included in public source. This helper follows the bounded
spans already checked by reference/course_init_case_common.py and palette checks.
It intentionally writes only under this checkout's ignored build/ directory.
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
    table = value(value(0x260020) + 24) & 0xffffff
    for index in range(5):
        source = value(table + 4 * index)
        tile_count = value(source, 2)
        if not 0 < tile_count <= 512:
            raise ValueError('Unsupported tile count')
        end = source + 6 + 2 * tile_count
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
        tile_size = max(value(source + 6 + 2 * i, 2) for i in range(tile_count)) + 16
        arrays[f'asset_{index}_map'] = span(source, end - source)
        arrays[f'asset_{index}_tiles'] = span(value(source + 2), tile_size)
    palette_count = value(0x21a045, 1)
    if not 0 < palette_count <= 64:
        raise ValueError('Unsupported palette count')
    arrays['palette_offsets'] = span(value(0x21a047), palette_count * 2)
    size = max(int.from_bytes(arrays['palette_offsets'][i:i + 2], 'little') for i in range(0, palette_count * 2, 2)) + 8
    arrays['palette_colors'] = span(value(0x26001c), size)
    arrays['cursor_descriptor'] = span(0x219836, 5)
    arrays['fade_in'] = span(0x219859 - 128, 256)
    arrays['fade_out'] = span(0x219867 - 128, 256)
    arrays['random_window'] = span(value(0x260010) - 128, 256)
    # The two bounded single-component cursor animations share shape/tile spans.
    index = value(0x219836, 2)
    animation_root = value(value(0x260024) + index) & 0x00bfffff
    shapes_base = value(value(0x260028) + index)
    shapes_size = tiles_size = 0
    tiles_base = None
    for animation in range(2):
        address = value(animation_root + animation * 4) & 0xffffff
        animation_size = 20
        for frame in range(5):
            component = value(address + frame * 4 + 2, 2)
            animation_size = max(animation_size, component + 4)
            descriptor = value(address + component, 2)
            if not descriptor & 0x8000:
                raise ValueError('Only verified single-component cursor frames supported')
            shape = descriptor & 0x7fff
            for cursor_count in range(1, 65):
                if value(shapes_base + shape + (cursor_count - 1) * 2, 1) & 0x80:
                    break
            else:
                raise ValueError('Unterminated cursor shape')
            pointer_at = shapes_base + shape + cursor_count * 2
            root = value(value(pointer_at))
            if tiles_base is not None and root != tiles_base:
                raise ValueError('Cursor shapes must share resolved tile source')
            tiles_base = root
            shapes_size = max(shapes_size, shape + cursor_count * 4 + 4)
            for tile in range(cursor_count):
                tiles_size = max(tiles_size, value(pointer_at + 4 + tile * 2, 2) * 16 + 16)
        arrays[f'cursor_animation_{animation}'] = span(address, animation_size)
    arrays['cursor_shapes'] = span(shapes_base, shapes_size)
    arrays['cursor_tiles'] = span(tiles_base, tiles_size)
    lines = ['/* PRIVATE GENERATED CARTRIDGE ASSETS. Do not commit or distribute. */']
    for name, data in arrays.items():
        lines.append(f'static const uint8_t selector_{name}[{len(data)}]={{')
        for at in range(0, len(data), 24):
            lines.append(','.join(str(x) for x in data[at:at + 24]) + ',')
        lines.append('};')
    lines += ['static ntm_cursor_assets selector_cursor_assets;', 'static void selector_resources(ntm_course_resources *r, ntm_screen_asset *a, ntm_palette_job *j, uint8_t *cursor) {']
    for i in range(5):
        lines.append(f'a[{i}].data=selector_asset_{i}_map;a[{i}].size=sizeof selector_asset_{i}_map;a[{i}].tiles=selector_asset_{i}_tiles;a[{i}].tiles_size=sizeof selector_asset_{i}_tiles;')
    lines += [f'j[0].kind=1;j[0].offset={value(0x21a046, 1)*8};j[0].count={palette_count};',
              'j[0].offsets=selector_palette_offsets;j[0].offsets_size=sizeof selector_palette_offsets;',
              'j[0].colors=selector_palette_colors;j[0].colors_size=sizeof selector_palette_colors;']
    for i, address in enumerate((0x21a053, 0x21a24e), 1):
        lines.append(f'j[{i}].kind=2;j[{i}].pending_index={value(address+5,1)};j[{i}].offset={480+value(address,1)};j[{i}].color={value(address+1,2)};j[{i}].control_offset={value(address+3,2)};')
    lines += ['r->assets=a;r->palette_jobs=j;r->cursor_descriptor=selector_cursor_descriptor;',
              'r->fade_in=selector_fade_in;r->fade_out=selector_fade_out;r->random=selector_random_window;r->random_cursor=cursor;',
              'selector_cursor_assets.animations[0].data=selector_cursor_animation_0;selector_cursor_assets.animations[0].size=sizeof selector_cursor_animation_0;selector_cursor_assets.animations[0].frames=5;',
              'selector_cursor_assets.animations[1].data=selector_cursor_animation_1;selector_cursor_assets.animations[1].size=sizeof selector_cursor_animation_1;selector_cursor_assets.animations[1].frames=5;',
              'selector_cursor_assets.shapes=selector_cursor_shapes;selector_cursor_assets.shapes_size=sizeof selector_cursor_shapes;',
              'selector_cursor_assets.tiles=selector_cursor_tiles;selector_cursor_assets.tiles_size=sizeof selector_cursor_tiles;r->cursor=&selector_cursor_assets;', '}']
    assert len(arrays['palette_offsets']) == palette_count * 2
    assert palette_count == value(0x21a045, 1), 'Palette count must come from its own source field'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('\n'.join(lines)+'\n')
    print(f'Exported {len(arrays)} bounded private spans to ignored build directory.')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROOT/'private-inputs/neo-turf-masters.ngc')
    parser.add_argument('--output', type=Path, default=ROOT/'build/native-selector/selector_assets.inc')
    args = parser.parse_args()
    export(args.rom.read_bytes(), args.output)
