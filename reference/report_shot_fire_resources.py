#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Measure first-fire source spans, without exporting any private bytes.

Only this size report is written, under ignored build/. Existing exporters,
assets, binaries and emulator cores are never modified. Source identity and
prefix overlap, not byte equality or pointer relocation, determine sharing.
Run from any directory with the existing private reference dependencies built.
"""
import importlib
import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from aim_pack import pack, unpack


def main():
    spec = importlib.util.spec_from_file_location('fire_budget_export', ROOT / 'tools/export-aim-assets.py')
    exporter = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(exporter)
    rom = (ROOT / 'private-inputs/neo-turf-masters.ngc').read_bytes()
    base = exporter.Resources(rom)
    base.collect()
    fire = importlib.import_module('shot_fire_case_common')
    groups = {}

    def add(group, name, source, size):
        base.data(source, size)  # Validate every source bound.
        groups.setdefault(group, {})[name] = (source, size)

    def palette(group, descriptor):
        count = base.value(descriptor, 1)
        offsets = base.value(descriptor + 2)
        add(group, 'palette_offsets_' + str(descriptor), offsets, count * 2)
        add(group, 'palette_colors_' + str(descriptor), base.value(0x26001c),
            max(base.value(offsets + 2 * i, 2) for i in range(count)) + 8)

    def assets(module_name, group):
        before = len(fire.tee_assets)
        module = importlib.import_module(module_name)
        for i, (source, asset) in enumerate(fire.tee_assets[before:]):
            add(group, 'screen_descriptor_' + str(i), source, asset.size)
            add(group, 'screen_tiles_' + str(i), base.value(source + 2), asset.tiles_size)
        return module

    palette('fire', fire.fire_palette_descriptor)
    assets('shot_spin_case_common', 'spin')
    add('spin', 'bounds', 0x21ee9f, 24)
    trajectory = assets('shot_trajectory_case_common', 'trajectory')
    for descriptor in trajectory.trajectory_descriptors:
        palette('trajectory', descriptor)
    for name, source, size in [('metadata', 0x2197d7, 5), ('sprite_bases', 0x21ee93, 6), ('hud_bases', 0x21ee8d, 6)]:
        add('trajectory', name, source, size)
    golfer = assets('shot_golfer_case_common', 'golfer')
    for i, (source, size) in enumerate(golfer.specs):
        add('golfer', 'initializer_table_' + str(i), source, size)
    for pair in golfer.golfer_palette_descriptors:
        for command, descriptor in pair:
            if base.value(0x2198f3 + command) & 255 == 4:
                palette('golfer', descriptor)
    assets('shot_golfer_animation_case_common', 'golfer_animation')
    drawing = assets('shot_golfer_drawing_case_common', 'golfer_drawing')
    for name, source, size in [('eligible', 0x21ba55, 16), ('depth_records', 0x21b975, 20),
                               ('clip_records', 0x21b99d, 116), ('metadata', 0x219854, 5)]:
        add('golfer_drawing', name, source, size)
    # Resolver read callbacks give the exact animation/shape/tile source spans.
    for tag, module in [('27', 'shot_trajectory_object_assets'), ('51', 'shot_golfer_object_assets'),
                        ('52', 'shot_golfer_drawing_object_assets')]:
        group = 'tag' + tag
        def read(source, size):
            add(group, 'span_' + str(len(groups.get(group, {}))), source, size)
            return base.data(source, size)
        importlib.import_module(module).resolve(base.value, read)

    def union(spans):
        ranges = []
        for address, size in sorted(spans.values()):
            if ranges and address <= ranges[-1][1]:
                ranges[-1][1] = max(ranges[-1][1], address + size)
            else:
                ranges.append([address, address + size])
        return ranges

    def measure(spans):
        resource = exporter.Resources(rom)
        resource.spans = spans
        raw, _, ranges = resource.pack()
        packed = pack(raw)
        assert unpack(packed, len(raw)) == raw
        return {'span_count': len(spans), 'raw_span_bytes': sum(n for _, n in spans.values()),
                'deduplicated_source_bytes': sum(b-a for a, b in union(spans)),
                'aligned_pool_bytes': len(raw), 'packed_pool_bytes': len(packed),
                'alignment_bytes': len(raw)-sum(b-a for a, b in union(spans)),
                'merged_ranges': ranges, 'roundtrip': True}

    def merge(*sets):
        return {str(i) + ':' + key: value for i, spans in enumerate(sets) for key, value in spans.items()}

    static = merge(*groups.values())
    combined = merge(base.spans, static)
    fixed_ranges = union(combined)

    def uncovered(spans):
        result = {}
        for start, end in union(spans):
            for a, b in fixed_ranges:
                if b <= start:
                    continue
                if a >= end:
                    break
                if a > start:
                    result[str(len(result))] = (start, a-start)
                start = max(start, b)
                if start >= end:
                    break
            if start < end:
                result[str(len(result))] = (start, end-start)
        return result

    maps = []
    features = importlib.import_module('shot_golfer_features_case_common')
    all_maps = {}
    terrain_keep = []
    for i, geometry in enumerate(golfer.AM.geometries):
        spans = {}
        for name, index in [('large', geometry.map_index), ('close', geometry.aux_index)]:
            terrain, source = golfer.resolve_terrain(index, terrain_keep)
            spans[name] = (source, terrain.size)
        feature, source = features.feature_resources[i]
        spans['features'] = (source, feature.features_size)
        all_maps.update({str(i) + ':' + key: value for key, value in spans.items()})
        maps.append({'course': i//18, 'hole': i%18, 'source_sizes': {k: n for k, (_, n) in spans.items()},
                     'standalone': measure(spans), 'incremental_slice': measure(uncovered(spans)), '_spans': spans})
    report = {'scope': 'First-fire static children, plus separately loadable current-hole terrain and feature slices. Sizes only; no cartridge bytes or hashes.',
              'exclusions': ['Native structs, bindings, code, stack and working buffers',
                             'Assembled palette arrays and scalar palette jobs outside source pool',
                             'Disk filesystem overhead and any new loader',
                             'No native target build, fit proof, gameplay completeness or speed claim'],
              'base': measure(base.spans), 'static_fire': measure(static),
              'base_plus_static_fire': measure(combined),
              'groups': {name: measure(spans) for name, spans in groups.items()},
              'source_manifest': {name: {key: size for key, (_, size) in spans.items()} for name, spans in groups.items()},
              'all54_maps': measure(all_maps),
              'all54_incremental_map_slices_packed_sum': sum(m['incremental_slice']['packed_pool_bytes'] for m in maps),
              'all54_maps_packed_as_one_pool': 'Reported for comparison only; not required to keep all holes resident.',
              'stable_control_free_bytes': {'chip': 69712, 'dd': 7168},
              'startup_loader_reclaim': 'Existing packed pool remains resident. Potential future reclaim is not implemented.'}
    report['static_increment'] = {k: report['base_plus_static_fire'][k]-report['base'][k]
                                  for k in ('aligned_pool_bytes', 'deduplicated_source_bytes', 'packed_pool_bytes')}
    sample_indices = sorted({0, 18, 36, max(range(54), key=lambda i: maps[i]['standalone']['aligned_pool_bytes'])})
    report['combined_samples'] = [{'course': i//18, 'hole': i%18,
                                  'base_static_current_hole': measure(merge(combined, maps[i]['_spans']))} for i in sample_indices]
    for item in maps:
        del item['_spans']
    report['holes'] = maps
    report['first_holes'] = [maps[i] for i in (0, 18, 36)]
    assert report['base']['aligned_pool_bytes'] == 596238
    assert report['base']['packed_pool_bytes'] == 306527
    assert sum(m['source_sizes']['features'] for m in maps) == 5464
    assert max(m['source_sizes']['large']+m['source_sizes']['close'] for m in maps) == 44244
    output = ROOT / 'build/native-fire-resource-report.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('base', 'static_fire', 'base_plus_static_fire', 'static_increment', 'all54_maps', 'combined_samples')}, indent=2))


if __name__ == '__main__':
    main()
