#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Independent cartridge resources and complete stock PAL A1200 tee captures."""
import argparse
import json
import re
import struct
from pathlib import Path
from selector_verify import palette_from_cartridge, verify
from intro_verify import source_palette as title_palette, source_state as title_state
from round_verify import Cartridge, source_state as round_state

SCENARIOS = ['course0','course1','course2','random','cancel','skip0','skip1','skip2','skipboth','recordskip0','recordskip1','recordskip2','recordheld','teeskip0','teeskip1','teeskip2','teeheld','teeseen0','teeseen1','teeseen2']

def prior_round_writes(rom, ram, words, timeline):
    """Apply original queued/direct writes, with independently known head delays."""
    c = TeeCartridge(rom)
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
    return tiles, maps

class TeeCartridge(Cartridge):
    def stamp(self, tiles, maps, group_offset, entry, destination, tile):
        group = self.value(self.value(0x260020) + group_offset) & 0xffffff
        source = self.value(group + entry)
        count, pixels = self.value(source, 2), self.value(source + 2)
        assert 0 < count <= 512 and (tile & 511) + count <= 512
        for index in range(count):
            block = self.span(pixels + self.value(source + 6 + index * 2, 2), 16)
            for byte, value in enumerate(block):
                tiles[((tile & 511) + index) * 16 + byte] = value
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


def tee_palette(c, prior):
    palette = bytearray(prior)
    for command in (0x84, 0x88):
        packed = c.value(0x2198f3 + command)
        kind, source = packed & 255, packed >> 8
        if kind == 4:
            count, dest = c.value(source, 1), c.value(source + 1, 1) * 8
            for i in range(count):
                at = c.value(c.value(source + 2) + 2 * i, 2)
                palette[dest+8*i:dest+8*i+8] = c.span(c.value(0x26001c) + at, 8)
        else:
            assert kind == 32
            dest = 480 + c.value(source, 1)
            palette[dest:dest+2] = c.span(source + 1, 2)
    return palette


def animation_clock(c, animation, tails):
    frame = 0
    delay = c.value(animation, 1) + 1
    frozen = False
    for _ in range(tails):
        if not frozen:
            delay = (delay - 1) & 255
            if delay == 0:
                mode = c.value(animation + frame + 1, 1)
                if mode == 192:
                    frozen = True
                else:
                    frame = 0 if mode & 128 else frame + 4
                    delay = c.value(animation + frame, 1)
    return frame, delay


def object_frame(c, tiles, animation, frame, x, y, tile, priority=3, hidden=False):
    sprites, palettes = bytearray(), bytearray()
    component = animation + c.value(animation + frame + 2, 2)
    shapes = c.value(c.value(0x260028) + 52)
    for _ in range(2):
        raw = c.value(component, 2)
        shape = shapes + (raw & 32767)
        if c.value(shape + 1, 1) != 255:
            count = 1
            while not c.value(shape + 2 * count - 2, 1) & 128:
                count += 1
                assert count <= 64
            pointer = shape + 2 * count
            pixels = c.value(c.value(pointer))
            dx = int.from_bytes(c.span(component + 2, 1), 'little', signed=True)
            dy = int.from_bytes(c.span(component + 3, 1), 'little', signed=True)
            for i in range(count):
                for k, value in enumerate(c.span(pixels + c.value(pointer + 4 + 2 * i, 2) * 16, 16)):
                    tiles[(tile + i) * 16 + k] = value
                attr, xy = c.value(shape + 2 * i, 1), c.value(shape + 2 * i + 1, 1)
                sx, sy = (x + dx + (xy & 15) * 8) & 65535, (y + dy + (xy >> 4) * 8) & 65535
                if hidden or ((sx + 7) & 65535) > 166 or ((sy + 7) & 65535) > 158:
                    continue
                word = ((tile + i) | (priority << 11) | ((attr & 240) << 9)) & 65535
                sprites += bytes((word & 255, word >> 8, sx & 255, sy & 255))
                palettes.append(attr & 15)
            tile += count
        if raw & 32768:
            return sprites, palettes
        component += 4
    raise AssertionError('Unsupported original component count')


def tee_state(rom, ram, words, timeline):
    c = TeeCartridge(rom)
    tiles, maps = prior_round_writes(rom, ram, words, timeline)
    update, start, complete = words[19], timeline[73], timeline[76]
    meta = struct.unpack_from('>128I', ram, words[81])
    course, hole, number, par, distance = meta[4:9]
    assert (course, hole, number, par, distance) == tuple(timeline[i] for i in (82,83,84,85,86))
    for y in range(19):
        for x in range(40):
            maps[y * 64 + x] = 0
    source = c.value((c.value(c.value(0x260020) + 0xb4) & 0xffffff) + 0x80)
    count, pixels = c.value(source, 2), c.value(source + 2)
    for i in range(count):
        for k, value in enumerate(c.span(pixels + c.value(source + 6 + i * 2, 2), 16)):
            tiles[0x100 + 16 * i + k] = value
    c.stamp(tiles, maps, 0x64, 0, 0x800, 0x50)
    # Distance is taken from the independently addressed hole descriptor.
    group = 0x21bb4d + c.value(0x21bb4d + 2 * course, 2)
    descriptor = group + c.value(group + 2 * hole, 2)
    assert c.value(descriptor + 1, 1) == par
    assert c.value(descriptor + 2, 2) == distance
    for i, digit in enumerate(f'{distance % 1000:3d}'):
        value = (32 if digit == ' ' else ord(digit) - 48 + 16) + 0x2800
        dest = 0x902 + 2 * i
        maps[dest], maps[dest + 1] = value & 255, value >> 8
    if update >= start + 1:
        c.stamp(tiles, maps, 0x8c, 4 * (number + 1), 0x52, 0x640)
        c.stamp(tiles, maps, 0xb4, 4 * (par + 30), 0x94a, 0x2846)
    if update >= start + 2:
        for view, dest, tile in ((0,0xa46,0x70),(1,0x246,0x2e0),(2,0x858,0x550)):
            c.stamp(tiles, maps, 4 * (course + 36), 4 * hole + 0x48 * view, dest, tile)
    sprites, palettes = bytearray(256), bytearray(64)
    setup = start + 1
    cursor = (timeline[39] - 6) & 255
    random_offset = cursor if cursor < 128 else cursor - 256
    pin_index = c.value(c.value(0x260010) + random_offset, 1) & 15
    pins = descriptor + c.value(descriptor + 14, 2)
    marker = c.value(0x21ba69 + 4 * course) + 4 * hole
    positions = ((96,8),(96+c.value(pins+4*pin_index,2),8+c.value(pins+4*pin_index+2,2)),(c.value(marker,2),c.value(marker+2,2)))
    assert meta[11] == timeline[39], 'Shared RNG retained after six normal hole draws'
    if update >= setup + 1:
        root = c.value(c.value(0x260024) + 52) & 0xbfffff
        used = 0
        for i, offset in enumerate((12, 0, 4)):
            at = 64 + 12 + 13 * i
            tag, x, y, tile, flags, palette_offset, priority, velocity, frame, delay, second, width, objid = meta[at:at+13]
            assert (x,y) == positions[i], ('Independent tee object coordinates',i,update,(x,y),positions[i])
            assert tile == (400,432,448)[i]
            animation = c.value(root + offset) & 0xffffff
            age = min(update, complete) - setup
            source_frame, source_delay = animation_clock(c, animation, age)
            if update <= complete:
                assert (tag, tile, palette_offset, priority, velocity, frame, delay, second, width, objid) == ((30,29,31)[i], (400,432,448)[i], 0, 3, offset, source_frame, source_delay, 0, 52, i), ('Uploaded object state', i, update, meta[at:at+13], source_frame, source_delay)
            # Terminal empty frames retain previously uploaded tiles. Replay all
            # preceding source animation shapes, never target tile bytes.
            for earlier in range(0, source_frame, 4):
                object_frame(c, tiles, animation, earlier, x, y, tile)
            ss, ps = object_frame(c, tiles, animation, source_frame, x, y, tile)
            if update <= complete:
                sprites[4*used:4*(used+len(ps))] = ss
                palettes[used:used+len(ps)] = ps
                used += len(ps)
    for offset, value in tiles.items():
        assert ram[words[22] + offset] == value, ('Tee source tile', offset, update, ram[words[22]+offset], value)
    for offset, value in maps.items():
        assert ram[words[23] + offset] == value, ('Tee source map', offset, update, ram[words[23]+offset], value)
    assert ram[words[24]:words[24]+256] == sprites, ('Tee source sprites', update)
    assert ram[words[25]:words[25]+64] == palettes, ('Tee source sprite palettes', update)
    width = 160 if timeline[74] <= update < complete else 0
    assert ram[words[27]:words[27]+13] == bytes((0,0,0,0,0,0,0,0,width,152,0,129,7))
    phase = 6 if update >= complete else 5 if update >= complete-1 else 4 if update >= timeline[75] else 3 if update >= timeline[74] else 2 if update >= start+1 else 1
    assert meta[0] == words[80] == phase
    return {'verified_tile_bytes': len(tiles), 'verified_map_bytes': len(maps), 'course':course, 'source_hole':hole, 'display_hole_number':number+1, 'par':par, 'distance':distance, 'snapshot_phase':meta[0], 'uploaded_object_count':meta[67], 'pin_index_from_shared_rng':pin_index}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--directory', type=Path, required=True)
    p.add_argument('--scenario', choices=SCENARIOS, required=True)
    p.add_argument('--rom', type=Path, default=Path(__file__).resolve().parents[2]/'private-inputs/neo-turf-masters.ngc')
    a = p.parse_args()
    log = (a.directory/'run.log').read_text()
    for setting in ('cpu_model=68020','chipmem_size=4','bogomem_size=0','chipset=aga','ntsc=false','Model: "A1200OG".'):
        assert setting in log, setting
    assert all(x == 'fastmem_size=0' for x in log.splitlines() if x.startswith('fastmem_size='))
    assert 'TEE stage=8' in log
    rom = a.rom.read_bytes()
    final = (a.directory/'chip.finished.bin').read_bytes()
    timeline = struct.unpack_from('>128I', final, final.index(b'NTEE0001'))
    c = TeeCartridge(rom)
    labels = list(dict.fromkeys(re.findall(r'^STAGE ([^ ]+) ', log, re.M)))
    results = {}
    for label in labels:
        path = a.directory/f'chip.{label}.bin'
        ram = path.read_bytes()
        words = struct.unpack_from('>128I', ram, ram.index(b'NTEE0001'))
        update = words[19]
        intro = a.scenario != 'cancel' and update >= timeline[53] + 1
        round_started = intro and update >= timeline[64]
        tee_started = round_started and update >= timeline[73]
        palette = title_palette(rom, words[51]) if intro else palette_from_cartridge(rom, a.scenario != 'cancel' and update >= timeline[52])
        if round_started and update >= timeline[64] + 1:
            palette = c.palette(palette)
        if tee_started and update >= timeline[73] + 1:
            palette = tee_palette(c, palette)
        result = verify(a.directory/f'screen.{label}.ppm', path, palette, b'NTEE0001', 512)
        result.update(maximum_cache_decode_seconds=words[88]/709379,last_cache_decode_seconds=words[89]/709379,cache_decodes=words[90])
        result.update(snapshot_title_phase=words[62],live_title_phase=words[61],snapshot_round_phase=words[68],live_round_phase=words[67],snapshot_tee_phase=words[80],live_tee_phase=words[78],snapshot_update=update)
        if label in ('initial','right','left','japan','return','repeat','return2','chosen'):
            assert words[46] < 15, 'All ordinary selector keys retained'
        if intro:
            assert words[46] == 0xffffffff
            result['source_state'] = tee_state(rom,ram,words,timeline) if tee_started else round_state(rom,ram,words,timeline) if round_started else title_state(rom,ram,words,timeline)
        results[label] = result
    assert timeline[9] == 0 and timeline[12] == 0
    assert timeline[10] == (252 if a.scenario == 'cancel' else 0)
    assert bool(timeline[38]) == (a.scenario == 'random')
    assert [results[x]['displayed_selection'] for x in ('initial','right','left','japan','return','repeat','return2')] == [1,2,1,0,1,2,1]
    if a.scenario != 'cancel':
        if a.scenario[-1].isdigit():
            assert timeline[51] == int(a.scenario[-1]), 'Requested hardware course selection'
        assert timeline[53] == timeline[52]+1 and timeline[54] == timeline[53]+2
        assert timeline[58] == timeline[54]+20 and timeline[59] == timeline[63]+2
        assert (0 < timeline[63]-timeline[58] < 60) if a.scenario.startswith('skip') else timeline[63]-timeline[58] == 60
        assert timeline[60]-timeline[59] < 60 if a.scenario == 'skipboth' else timeline[60]-timeline[59] == 60
        assert timeline[64] == timeline[60]+1 and timeline[65] == timeline[64]+2
        assert timeline[70] == timeline[64] and timeline[72] == timeline[66]+1
        record_skip = a.scenario.startswith('recordskip') or a.scenario == 'recordheld'
        assert (0 < timeline[66]-timeline[65] < 240) if record_skip else timeline[66]-timeline[65] == 240
        assert ('ROUND_SKIP ' in log) == record_skip
    if a.scenario != 'cancel':
        assert timeline[77:80] == (1,6,2), 'Exact unconverted2096a4 boundary'
        assert timeline[7] == timeline[76] + 1
        assert timeline[73] == timeline[72] and timeline[74] == timeline[73] + 3 and timeline[75] == timeline[74] + 4
        skip = a.scenario.startswith(('teeskip','teeseen')) or a.scenario == 'teeheld'
        assert (0 < timeline[76] - timeline[75] < 181) if skip else timeline[76] - timeline[75] == 181
        assert ('TEE_SKIP ' in log) == skip
    else:
        assert timeline[48] == 2 and not any(timeline[72:80])
    pattern = r'PRESENTED_INTRO label=(\S+) frame=(\d+) snapshot_update=(\d+) current_update=(\d+) title_phase=(\d+) live_title_phase=(\d+) round_phase=(\d+) live_round_phase=(\d+) tee_phase=(\d+) live_tee_phase=(\d+) composition_ticks=(\d+)'
    presentations = []
    for match in re.finditer(pattern, log):
        item = dict(zip(('frame','snapshot_update','current_update','title_phase','live_title_phase','round_phase','live_round_phase','tee_phase','live_tee_phase','composition_ticks'), map(int,match.groups()[1:])))
        item['label'] = match.group(1)
        item['composition_seconds'] = item['composition_ticks']/709379
        presentations.append(item)
    accept_label = 'chosen' if 'chosen' in labels else 'return2'
    accept_frame = int(re.search(r'^STAGE '+accept_label+r' frame=(\d+)',log,re.M).group(1))
    stages = {}
    intervals = {'scenery':(timeline[54],timeline[58]),'title':(timeline[58]+1,timeline[63]),'tournament':(timeline[63]+1,timeline[60]),'round_record':(timeline[65],timeline[66]),'tee_overview':(timeline[74],timeline[76])}
    for name, (begin,end) in intervals.items():
        shown = [x for x in presentations if a.scenario != 'cancel' and begin <= x['snapshot_update'] < end]
        stages[name] = None if not shown else dict(shown[0],accept_to_verified_seconds_nominal_pal=(shown[0]['frame']-accept_frame)*.02,late_by_logical_updates=shown[0]['current_update']-shown[0]['snapshot_update'])
    if a.scenario.startswith('course') or a.scenario == 'random':
        assert stages['tee_overview'] is not None, 'Ordinary flow must actually present tee overview'
    if a.scenario.startswith('teeseen'):
        event = re.search(r'TEE_SKIP input_frame=(\d+) update=(\d+) phase=(\d+)',log)
        shown = stages['tee_overview']
        assert shown and event and int(event.group(1)) >= shown['frame'] and int(event.group(3)) == 4
    result = {'result':'PASS','scenario':a.scenario,'model':'PAL A1200 68EC020 AGA,2MiBChip,zeroFast','captures':results,'presentations':presentations,'timing':{'accept_input_frame':accept_frame,'first_verified_stages':stages,'logical_updates':{str(i):timeline[i] for i in (52,53,54,58,59,60,63,64,65,66,70,72,73,74,75,76)},'skip_input_events':re.findall(r'^(?:TITLE|TOURNAMENT|ROUND|TEE)_SKIP .*$',log,re.M)},'limits':['Stops before2096a4 shot scene, not playable golf','No audio synthesis','Snapshots can be late or omit stages; no full-speed or physical-hardware claim','Normal golfer0 fresh record target; no save restoration']}
    (a.directory/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__ == '__main__':
    main()
