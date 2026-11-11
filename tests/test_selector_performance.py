"""Synthetic report gates; no cartridge or captured trace required."""
import copy
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('compare_selector', ROOT/'tools/compare-selector-performance.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def reports():
    result = {}
    for scenario in module.SCENARIOS:
        captures = {}
        for label in module.CAPTURES:
            captures[label] = dict(selection=1, displayed_selection=1,
                running=int(label != 'finished'), outcome=252 if scenario == 'cancel' and label == 'finished' else 0,
                logical_updates_dropped=0, source_palette_bytes_verified=512,
                captured_pixels_verified=100, last_composition_seconds=2.0,
                max_input_poll_gap_ms=30.0, free_chip_after_display_allocation=1500000,
                random_delay_updates=60)
        result[scenario] = dict(result='PASS', model=module.MODEL, scenario=scenario, captures=captures)
    return result


class SelectorPerformanceTests(unittest.TestCase):
    def test_composition_ratio(self):
        old = reports(); new = copy.deepcopy(old)
        for flow in new.values():
            for capture in flow['captures'].values():
                capture['last_composition_seconds'] = 0.1
        result = module.compare(old, new)
        self.assertEqual(len(result['captures']), 12)
        self.assertTrue(all(row['composition_speedup'] == 20 for row in result['captures']))

    def test_extra_capture_and_response(self):
        old = reports(); new = reports()
        for data in (old, new):
            data['accept']['captures']['repeat'] = copy.deepcopy(data['accept']['captures']['right'])
        old['accept']['navigation_latency'] = {'repeat': {'verified_capture_ms_nominal_pal': 2000}}
        new['accept']['navigation_latency'] = {'repeat': {'verified_capture_ms_nominal_pal': 100}}
        result = module.compare(old, new)
        self.assertEqual(len(result['captures']), 13)
        repeat = next(row for row in result['captures'] if row['capture'] == 'repeat')
        self.assertEqual(repeat['response_speedup'], 20)
        new['accept']['captures']['repeat']['source_palette_bytes_verified'] = 0
        with self.assertRaises(ValueError):
            module.compare(old, new)

    def test_invalid_reports_rejected(self):
        for key, value in [('source_palette_bytes_verified', 0), ('logical_updates_dropped', 1),
                           ('last_composition_seconds', 0), ('last_composition_seconds', float('nan')),
                           ('last_composition_seconds', float('inf')), ('selection', 2),
                           ('max_input_poll_gap_ms', -1)]:
            old = reports(); new = reports()
            new['accept']['captures']['right'][key] = value
            with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                module.compare(old, new)
        for key, value in [('model', 'expanded A1200'), ('result', 'FAIL'), ('scenario', 'other')]:
            old = reports(); new = reports(); new['accept'][key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                module.compare(old, new)
        new = reports(); new['random']['captures']['finished']['random_delay_updates'] = 0
        with self.assertRaises(ValueError):
            module.compare(reports(), new)
