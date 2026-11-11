#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Export bounded normal tee overview resources to private ignored build output.

All 54 holes and every reachable tee object animation are retained. Overlapping
cartridge spans are shared byte-for-byte, without repacking embedded indices.
No original instructions are executed and no cartridge is embedded in source.
"""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class Resources:
    def __init__(self, rom):
        self.rom = rom
        self.spans = {}
        self.bindings = []

    def data(self, address, size):
        offset = address - 0x200000
        if size < 0 or offset < 0 or offset + size > len(self.rom):
            raise ValueError('Resolved cartridge span outside input')
        return self.rom[offset:offset + size]

    def value(self, address, size=4):
        return int.from_bytes(self.data(address, size), 'little')

    def span(self, name, address, size):
        self.data(address, size)
        if not size or name in self.spans:
            raise ValueError('Empty or repeated span')
        self.spans[name] = (address, size)
        return name

    def asset(self, target, group_offset, entry, font=False):
        v = self.value
        group = v(v(0x260020) + group_offset) & 0xffffff
        source = v(group + entry)
        count = v(source, 2)
        if not 0 < count <= 512:
            raise ValueError('Unsupported tile count')
        end = source + 6 + 2 * count
        if not font:
            header = v(end, 1)
            end += 1
            stride, rows = (2 if header & 128 else 1), header & 127
            if not 0 < rows <= 32:
                raise ValueError('Unsupported map rows')
            for _ in range(rows):
                for column in range(33):
                    word = v(end, stride)
                    end += stride
                    if word == (65535 if stride == 2 else 255):
                        if not column:
                            raise ValueError('Empty map row')
                        break
                else:
                    raise ValueError('Unterminated map row')
        size = max(v(source + 6 + 2 * i, 2) for i in range(count)) + 16
        prefix = f'tee_span_{len(self.spans)}'
        self.span(prefix + '_map', source, end - source)
        self.span(prefix + '_tiles', v(source + 2), size)
        self.bindings += [f'{target}.data={prefix}_map;{target}.size={end-source};',
                          f'{target}.tiles={prefix}_tiles;{target}.tiles_size={size};']

    def collect(self):
        v = self.value
        self.asset('r->font', 0xb4, 0x80, True)
        self.asset('r->header', 0x64, 0)
        for i in range(18):
            self.asset(f'r->hole_numbers[{i}]', 0x8c, 4 * (i + 1))
        for i in range(3):
            self.asset(f'r->pars[{i}]', 0xb4, 4 * (i + 33))
        for course in range(3):
            for hole in range(18):
                for view in range(3):
                    self.asset(f'r->holes[{course}][{hole}].views[{view}]', 4 * (course + 36), 4 * hole + 0x48 * view)
                position = v(0x21ba69 + 4 * course) + 4 * hole
                self.bindings += [f'r->holes[{course}][{hole}].marker_x={v(position,2)};r->holes[{course}][{hole}].marker_y={v(position+2,2)};']
        for i, address in enumerate((0x2197e6, 0x2197e1, 0x2197eb, 0x2197f0)):
            name = self.span(f'tee_metadata_{i}', address, 5)
            self.bindings += [f'r->metadata[{i}]={name};r->metadata_size[{i}]=5;']
        for index, command in enumerate((0x84, 0x88)):
            packed = v(0x2198f3 + command)
            kind, source = packed & 255, packed >> 8
            job = f'r->palettes[{index}]'
            if kind == 4:
                count, destination = v(source, 1), v(source + 1, 1) * 8
                if not 0 < count <= 64 or destination + 8 * count > 512:
                    raise ValueError('Unsupported palette group')
                pointer = v(source + 2)
                size = max(v(pointer + i * 2, 2) for i in range(count)) + 8
                offsets = self.span(f'tee_offsets_{index}', pointer, count * 2)
                colors = self.span(f'tee_colors_{index}', v(0x26001c), size)
                self.bindings += [f'{job}.kind=1;{job}.count={count};{job}.offset={destination};',
                                  f'{job}.offsets={offsets};{job}.offsets_size={count*2};{job}.colors={colors};{job}.colors_size={size};']
            elif kind == 32:
                offset, control, pending = 480 + v(source, 1), v(source + 3, 2), v(source + 5, 1)
                if offset > 510 or offset & 1 or control not in (0x118, 0x12) or pending > 1:
                    raise ValueError('Unsupported single-color destination')
                self.bindings += [f'{job}.kind=2;{job}.offset={offset};{job}.color={v(source+1,2)};{job}.control_offset={control};{job}.pending_index={pending};']
            else:
                raise ValueError('Unsupported palette service')
        graphics = v(0x2197e1, 2)
        if graphics != 52:
            raise ValueError('Unsupported tee graphics index')
        animation_root = v(v(0x260024) + graphics) & 0xbfffff
        shape_root = v(v(0x260028) + graphics)
        shape_end = tile_end = 0
        tile_root = None
        for index in range(8):
            address = v(animation_root + index * 4) & 0xffffff
            frame = end = 0
            while True:
                component = v(address + frame + 2, 2)
                for _ in range(2):
                    raw = v(address + component, 2)
                    shape = raw & 32767
                    end = max(end, component + 4)
                    if v(shape_root + shape + 1, 1) == 255:
                        shape_end = max(shape_end, shape + 2)
                    else:
                        for count in range(1, 65):
                            if v(shape_root + shape + 2 * count - 2, 1) & 128:
                                break
                        else:
                            raise ValueError('Unterminated tee object shape')
                        shape_end = max(shape_end, shape + 4 * count + 4)
                        pointer = shape_root + shape + 2 * count
                        root = v(v(pointer))
                        if tile_root not in (None, root):
                            raise ValueError('Inconsistent tee object tile source')
                        tile_root = root
                        tile_end = max(tile_end, 16 * (1 + max(v(pointer + 4 + 2 * j, 2) for j in range(count))))
                    if raw & 32768:
                        break
                    component += 4
                else:
                    raise ValueError('More than two tee object components')
                frame += 4
                if v(address + frame - 3, 1) & 128:
                    break
                if frame >= 256:
                    raise ValueError('Unterminated tee animation')
            name = self.span(f'tee_animation_{index}', address, max(end, frame))
            self.bindings += [f'o->animations[{index}].data={name};o->animations[{index}].size={max(end,frame)};o->animations[{index}].frames={frame//4};']
        self.span('tee_shapes', shape_root, shape_end)
        self.span('tee_object_tiles', tile_root, tile_end)
        self.bindings += [f'o->shapes=tee_shapes;o->shapes_size={shape_end};o->tiles=tee_object_tiles;o->tiles_size={tile_end};r->objects=o;']
        self.geometries = []
        for course in range(3):
            group = 0x21bb4d + v(0x21bb4d + 2 * course, 2)
            for hole in range(18):
                address = group + v(group + 2 * hole, 2)
                pins = self.span(f'tee_pins_{18*course+hole}', address + v(address + 14, 2), 64)
                mapgroup, mapbase = v(0x36000c), v(0x360000)
                mapindex = (v(mapgroup + 4 * course, 2) + 18 * hole) & 65535
                auxindex = (v(mapgroup + 4 * course + 12, 2) + 18 * hole) & 65535
                record = mapbase + mapindex
                delta = int.from_bytes(self.data(record + 4, 2), 'little', signed=True)
                descriptor = ','.join(map(str, self.data(address, 18)))
                dimensions = ','.join(map(str, self.data(v(record) + delta, 2)))
                self.geometries += [f'{{{{{descriptor}}},{pins},64,{{{dimensions}}},{mapindex},{auxindex},{address},{address+v(address+16,2)},{v(mapbase+auxindex+14)}}},']
        hazard_size = 0
        while v(0x204d81 + hazard_size, 2) != 65535:
            hazard_size += 10
            if hazard_size >= 1024:
                raise ValueError('Unterminated special-hole records')
        hazard_size += 2
        self.hole_bindings = ['h->holes=tee_geometries;h->hole_count=54;']
        for name, address, size in [('hazards', 0x204d81, hazard_size), ('quarter', 0x21d67b, 4100), ('variation', 0x21afc9, 22), ('threshold', 0x21afdf, 8), ('wind_low', 0x2050b9, 16), ('wind_high', 0x2050c9, 16), ('wind_limit', 0x2050a7, 18)]:
            label = self.span('tee_' + name, address, size)
            self.hole_bindings += [f'h->{name}={label};h->{name}_size={size};']

    def pack(self):
        ranges = []
        for address, size in sorted(self.spans.values()):
            if ranges and address <= ranges[-1][1]:
                ranges[-1][1] = max(ranges[-1][1], address + size)
            else:
                ranges.append([address, address + size])
        data, offsets = bytearray(), {}
        for first, end in ranges:
            # Preserve at least word alignment for private word-copy paths.
            if len(data) & 1:
                data.append(0)
            for name, (address, size) in self.spans.items():
                if first <= address and address + size <= end:
                    offsets[name] = len(data) + address - first
            data.extend(self.data(first, end - first))
        return data, offsets, len(ranges)


def export(rom, output):
    output = output.resolve()
    if not output.is_relative_to((ROOT / 'build').resolve()):
        raise ValueError('Private generated assets must stay under ignored build/')
    r = Resources(rom)
    r.collect()
    data, offsets, ranges = r.pack()
    lines = ['/* PRIVATE GENERATED CARTRIDGE ASSETS. Do not commit or distribute. */', f'static const uint8_t tee_pool[{len(data)}] __attribute__((aligned(2)))={{']
    for at in range(0, len(data), 24):
        lines.append(','.join(map(str, data[at:at+24])) + ',')
    lines += ['};']
    lines += [f'#define {name} (tee_pool+{offset})' for name, offset in offsets.items()]
    lines += ['static const ntm_hole_geometry tee_geometries[54]={'] + r.geometries + ['};']
    lines += ['static void tee_resources(ntm_tee_setup_resources *r,ntm_tee_object_assets *o,ntm_hole_prepare_resources *h) {'] + r.bindings + r.hole_bindings + ['}']
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('\n'.join(lines) + '\n')
    report = {'holes': 54, 'views': 162, 'animations': 8, 'bounded_spans': len(r.spans), 'raw_span_bytes': sum(n for _, n in r.spans.values()), 'deduplicated_pool_bytes': len(data), 'merged_ranges': ranges, 'geometry_records': 54, 'normal_handicap_art_omitted': True}
    output.with_suffix('.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom', type=Path, default=ROOT/'private-inputs/neo-turf-masters.ngc')
    p.add_argument('--output', type=Path, default=ROOT/'build/native-tee/tee_assets.inc')
    a = p.parse_args()
    print(json.dumps(export(a.rom.read_bytes(), a.output), indent=2))
