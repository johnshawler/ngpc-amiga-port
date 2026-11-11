#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compare independently verified private selector captures, never whole-game FPS."""
import argparse
import json
import math
from pathlib import Path

MODEL = 'PAL A1200 68EC020 AGA, 2 MiB Chip, zero Fast'
SCENARIOS = ('accept', 'cancel', 'random')
CAPTURES = ('initial', 'right', 'left', 'finished')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def number(capture, key, positive=False):
    value = capture[key]
    require(type(value) in (int, float) and math.isfinite(value), key + ': invalid number')
    require(value > 0 if positive else value >= 0, key + ': invalid range')
    return value


def validate(report, scenario):
    require(report['result'] == 'PASS', 'Unverified report')
    require(report['model'] == MODEL, 'Different hardware profile')
    require(report['scenario'] == scenario, 'Different scenario')
    require(set(CAPTURES).issubset(report['captures']), 'Missing required captures')
    for label in report['captures']:
        capture = report['captures'][label]
        require(capture['logical_updates_dropped'] == 0, 'Dropped logical updates')
        require(capture['source_palette_bytes_verified'] == 512, 'Missing original palette check')
        require(capture['captured_pixels_verified'] > 0, 'Missing pixel verification')
        require(capture['selection'] == capture['displayed_selection'], 'Display is stale at capture')
        number(capture, 'last_composition_seconds', True)
        number(capture, 'max_input_poll_gap_ms')
        number(capture, 'free_chip_after_display_allocation')
    finished = report['captures']['finished']
    require(finished['running'] == 0, 'Flow not finished')
    require(finished['outcome'] == (252 if scenario == 'cancel' else 0), 'Incorrect outcome')
    if scenario == 'random':
        require(finished['random_delay_updates'] > 0, 'Random path not exercised')


def compare(baseline, candidate):
    rows = []
    for scenario in SCENARIOS:
        old, new = baseline[scenario], candidate[scenario]
        validate(old, scenario)
        validate(new, scenario)
        labels = list(CAPTURES) + sorted((old['captures'].keys() & new['captures'].keys()) - set(CAPTURES))
        for label in labels:
            before, after = old['captures'][label], new['captures'][label]
            for key in ('selection', 'displayed_selection', 'running', 'outcome'):
                require(before[key] == after[key], f'{scenario}/{label}: different {key}')
            seconds_old = before['last_composition_seconds']
            seconds_new = after['last_composition_seconds']
            rows.append({'scenario': scenario, 'capture': label,
                         'baseline_composition_seconds': seconds_old,
                         'candidate_composition_seconds': seconds_new,
                         'composition_speedup': seconds_old / seconds_new,
                         'baseline_max_poll_gap_ms': before['max_input_poll_gap_ms'],
                         'candidate_max_poll_gap_ms': after['max_input_poll_gap_ms'],
                         'baseline_free_chip_bytes': before['free_chip_after_display_allocation'],
                         'candidate_free_chip_bytes': after['free_chip_after_display_allocation']})
            latency_key = 'verified_capture_ms_nominal_pal'
            if label in old.get('navigation_latency', {}) and label in new.get('navigation_latency', {}):
                old_ms = number(old['navigation_latency'][label], latency_key, True)
                new_ms = number(new['navigation_latency'][label], latency_key, True)
                rows[-1].update(baseline_input_to_verified_capture_ms_nominal_pal=old_ms,
                                candidate_input_to_verified_capture_ms_nominal_pal=new_ms,
                                response_speedup=old_ms / new_ms)
    return {'result': 'PASS', 'model': MODEL, 'captures': rows,
            'limits': ['Composition intervals are not button-to-visible latency or game FPS.',
                       'Optional response fields count emulated display frames at nominal PAL 20 ms/frame.',
                       'Same logical scenarios and selections; animation phase/work may differ.',
                       'Input reports are cumulative maximum gaps, not average latency.',
                       'Trusts input verifier reports; retain their source manifests and raw captures.']}


def load(directory):
    return {scenario: json.loads((directory / scenario / 'verification.json').read_text())
            for scenario in SCENARIOS}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baseline', type=Path)
    parser.add_argument('candidate', type=Path)
    args = parser.parse_args()
    print(json.dumps(compare(load(args.baseline), load(args.candidate)), indent=2))
