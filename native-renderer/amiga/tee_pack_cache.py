#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Losslessly compress the generated selector pictures for the separate tee target."""
import argparse
import json
import re
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
KEY_SIZE = 12688
PICTURE_SIZE = 24320
ENTRY_SIZE = KEY_SIZE + PICTURE_SIZE


def pack(data):
    out, at = bytearray(), 0
    while at < len(data):
        end = at + 1
        while end < len(data) and data[end] == data[at] and end - at < 128:
            end += 1
        if end - at >= 3:
            out += bytes((257 - (end - at), data[at]))
            at = end
            continue
        begin, at = at, end
        while at < len(data) and at - begin < 128:
            end = at + 1
            while end < len(data) and data[end] == data[at] and end - at < 128:
                end += 1
            if end - at >= 3:
                break
            at = min(end, begin + 128)
        out += bytes((at - begin - 1,)) + data[begin:at]
    return out


def emit(data):
    return '{\n' + '\n'.join(','.join(map(str, data[i:i+24])) + ',' for i in range(0, len(data), 24)) + '\n}'


def convert(source, output):
    for path in (source, output):
        if not path.resolve().is_relative_to((ROOT/'build').resolve()):
            raise ValueError('Private cache must remain under ignored build/')
    data = bytes(map(int, re.findall(r'\d+', source.read_text().split('=', 1)[1])))
    if len(data) != ENTRY_SIZE * 15:
        raise ValueError('Expected fifteen complete exact-key entries')
    lines = ['/* PRIVATE GENERATED CACHE. Do not commit or distribute. */']
    pictures = []
    for i in range(15):
        compressed = pack(data[i*ENTRY_SIZE+KEY_SIZE:(i+1)*ENTRY_SIZE])
        pictures.append(compressed)
        lines.append(f'static const uint8_t tee_picture_{i}[{len(compressed)}]=' + emit(compressed) + ';')
    lines.append(f'#define TEE_PACKED_PICTURE_BYTES {sum(map(len,pictures))}u')
    lines.append('static const tee_planar_entry selector_planar_entries[15]={')
    for i in range(15):
        at = i * ENTRY_SIZE
        fields = []
        for size in (4096, 256, 64, 16, 64, 8192):
            fields.append(emit(data[at:at+size]))
            at += size
        lines.append('{' + ','.join(fields) + f',tee_picture_{i},sizeof tee_picture_{i}' + '},')
    lines.append('};')
    output.write_text('\n'.join(lines) + '\n')
    report = {'entries': 15, 'original_bytes': len(data), 'keys_bytes': KEY_SIZE * 15,
              'compressed_picture_bytes': sum(map(len, pictures)), 'm68020_entry_metadata_bytes': 8 * 15,
              'all_existing_keys_preserved': True}
    output.with_suffix('.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source', type=Path)
    p.add_argument('output', type=Path)
    a = p.parse_args()
    print(json.dumps(convert(a.source, a.output), indent=2))
