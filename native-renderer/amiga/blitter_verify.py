#!/usr/bin/env python3
"""Independent byte-exact oracle for the prepared-plane Blitter experiment."""
import argparse
import json
import struct
import subprocess
import sys
from pathlib import Path

BYTES = 24320
NAMES = ['cpu_long_loop', 'cpu_long_unrolled', 'blitter_tile_pairs',
         'blitter_tile_rows', 'blitter_planes', 'blitter_all_planes',
         'blitter_all_planes_hog']
BLITS = [0, 0, 1520, 152, 8, 1, 1]


def fnv(data):
    value = 2166136261
    for byte in data:
        value = ((value ^ byte) * 16777619) & 0xffffffff
    return value


def planar(pixels):
    output = bytearray(BYTES)
    for y in range(152):
        for x in range(160):
            for plane in range(8):
                if pixels[y * 160 + x] & (1 << plane):
                    output[plane * 3040 + y * 20 + x // 8] |= 128 >> (x % 8)
    return bytes(output)


def oracle():
    backdrop = bytes(((x * 3) ^ (y * 7) ^ 0xa5) & 255
                     for y in range(152) for x in range(160))
    sources, masks = [], []
    for frame in range(2):
        source, mask = bytearray(), bytearray()
        for y in range(152):
            for x in range(160):
                tile = x // 8 + (y // 8) * 20
                u = 7 - x % 8 if tile & 1 else x % 8
                v = 7 - y % 8 if tile & 2 else y % 8
                code = (u * 3 + v * 5 + tile + frame * 7) % 16
                source.append((tile * 17 + code * 13 + frame * 29) % 256)
                mask.append(255 if code and (x + y + frame) % 8 else 0)
        sources.append(planar(source))
        masks.append(planar(mask))
    result = bytearray(planar(backdrop))
    for frame in range(16):
        result[:] = bytes((s & m) | (d & (255 ^ m))
                          for s, m, d in zip(sources[frame % 2], masks[frame % 2], result))
    return sources, masks, planar(backdrop), bytes(result)


def verify(ram, log):
    for setting in ['cpu_model=68020', 'cpu_cycle_exact=true',
                    'cpu_memory_cycle_exact=true', 'blitter_cycle_exact=true',
                    'immediate_blits=false', 'cpu_throttle=0.0',
                    'ntsc=false', 'chipmem_size=4', 'bogomem_size=0',
                    'chipset=aga', 'Model: "A1200OG".']:
        if setting not in log:
            raise ValueError('Missing setting ' + setting)
    fast = [line for line in log.splitlines() if line.startswith('fastmem_size=')]
    if not fast or any(line != 'fastmem_size=0' for line in fast):
        raise ValueError('Fast RAM differs')
    if len(ram) != 2097152:
        raise ValueError('Wrong Chip size')
    marker = ram.find(b'NGPDISP1')
    if marker < 0 or ram.find(b'NGPDISP1', marker + 1) >= 0:
        raise ValueError('Missing or duplicate allocation header')
    pointer, size, done = struct.unpack_from('>3I', ram, marker + 32)
    if done != 0x444f4e45 or size != 512 or pointer != marker + 245920:
        raise ValueError('Incomplete benchmark or wrong report allocation')
    if marker + 300000 > len(ram):
        raise ValueError('Allocation is not entirely in Chip RAM')
    r = struct.unpack_from('>128I', ram, pointer)
    if r[:3] != (0x424c5452, 2, 709379):
        raise ValueError('Wrong clock/protocol')
    calibration = r[3] * 1000 / r[2]
    if not 19.8 < calibration < 20.2:
        raise ValueError('Bad CIA/PAL calibration')
    if r[4:7] != (marker + 197280, BYTES, marker + 221600):
        raise ValueError('Wrong bitmap pointers')
    if r[8:12] != tuple(marker + 100000 + n * BYTES for n in range(4)):
        raise ValueError('Wrong source pointers')
    # Master, bitplanes, Copper and Blitter DMA must be enabled; no Blitter hog.
    if r[7] & 0x07c0 != 0x03c0 or r[14:16] != (16, 14) or not r[12]:
        raise ValueError('Wrong DMA flags/workload')
    sources, masks, backdrop, expected = oracle()
    for ptr, data in zip(r[8:12], sources + masks):
        if ram[ptr:ptr + BYTES] != data:
            raise ValueError('Source or mask differs from independent oracle')
    if r[13] != fnv(backdrop) or ram[r[6]:r[6] + BYTES] != backdrop:
        raise ValueError('Backdrop differs')
    if ram[r[4]:r[4] + BYTES] != expected:
        raise ValueError('Final destination differs')
    records = []
    for sample in range(14):
        variant, frames, ticks, checksum, mismatches, blits, dma, size = r[16 + sample * 8:24 + sample * 8]
        wanted = sample if sample < 7 else 13 - sample
        if (variant, frames, checksum, mismatches, blits, size) != (wanted, 16, fnv(expected), 0, BLITS[wanted], BYTES):
            raise ValueError('Wrong workload or exactness record ' + str(sample))
        if not ticks or dma & 0x47c0 != (0x07c0 if variant == 6 else 0x03c0):
            raise ValueError('Zero duration, active Blit, or wrong DMA state')
        records.append(dict(variant=NAMES[variant], frames=frames, ticks=ticks,
                            ms_per_frame=ticks * 1000 / r[2] / frames,
                            blits_per_frame=blits, mismatched_bytes=mismatches))
    summaries = []
    for variant in range(7):
        a, b = records[variant]['ms_per_frame'], records[13 - variant]['ms_per_frame']
        difference = abs(a - b) / min(a, b)
        if difference > .01:
            raise ValueError('Forward/reverse timing differs by over 1%: ' + NAMES[variant])
        summaries.append(dict(variant=NAMES[variant], min_ms_per_frame=min(a, b),
                              max_ms_per_frame=max(a, b), order_difference_percent=100 * difference))
    return dict(result='PASS', pal_frame_calibration_ms=calibration,
                prepared_source_generation_ms=r[12] * 1000 / r[2],
                source_and_mask_bytes=4 * BYTES, allocated_chip_bytes=300000,
                records=records, summary=summaries,
                constraints=[
                    'Stock PAL A1200 emulator, 68020, AGA, 2 MiB Chip, zero Fast/Slow RAM; not physical hardware.',
                    'Identical active 320x256 eight-plane display DMA for all variants; last variant explicitly enables Blitter priority.',
                    'Each frame cookie-cuts 160x152 pixels in eight planes, alternating two prepared source/mask sets.',
                    'Actual custom-chip A/B/C/D DMA, minterm 0xCA; all timed frames wait for Blitter completion.',
                    '2-tile groups are 16x8; row groups are 160x8; plane groups are 160x152; all-plane batch is 160x1216.',
                    'Source conversion and batch construction are excluded; prepared source generation is reported separately.',
                    'No arbitrary tile gathering, clipping, shifting, layer/sprite ordering, palette changes, game logic, audio or flipping.',
                    'No Neo Turf Masters acceleration, native game frame rate or full-speed gameplay claim.'
                ])


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--chip', required=True, type=Path)
    p.add_argument('--log', required=True, type=Path)
    p.add_argument('--report', type=Path)
    p.add_argument('--capture', type=Path)
    args = p.parse_args()
    result = verify(args.chip.read_bytes(), args.log.read_text())
    if args.capture:
        display = json.loads(subprocess.check_output([
            sys.executable, str(Path(__file__).with_name('verify.py')), '--scene',
            '--capture', str(args.capture), '--chip', str(args.chip), '--log', str(args.log)
        ], text=True))
        # The shared display verifier checks the first 100000 bytes only.
        # This wrapper separately validates the larger experiment allocation.
        display['display_fixture_prefix_bytes'] = display['allocated_chip_bytes']
        display['allocated_chip_bytes'] = result['allocated_chip_bytes']
        result['display_verification'] = display
    output = json.dumps(result, indent=2) + '\n'
    print(output, end='')
    if args.report:
        args.report.write_text(output)


if __name__ == '__main__':
    main()
