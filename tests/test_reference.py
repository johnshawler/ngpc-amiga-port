"""Host reference smoke tests; no cartridge or game-equivalence assertions."""
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('reference_run', ROOT / 'reference/run.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class ReferenceTests(unittest.TestCase):
    def test_inputs(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / 'inputs.json'
            path.write_text('{"0": ["a", "a", "up"], "12": ["option"]}')
            self.assertEqual(module.read_inputs(path), {0: 17, 12: 8})
            path.write_text('{"-1": ["a"]}')
            with self.assertRaises(ValueError):
                module.read_inputs(path)
            path.write_text('{"0": ["unknown"]}')
            with self.assertRaises(KeyError):
                module.read_inputs(path)

    @unittest.skipUnless((ROOT / 'reference/build/clock.so').exists(), 'Build reference first')
    def test_metadata_and_clock(self):
        command = [sys.executable, str(ROOT / 'reference/run.py'), '--epoch', '978307200']
        first = subprocess.check_output(command, text=True)
        second = subprocess.check_output(command, text=True)
        self.assertEqual(first, second)
        data = json.loads(first)
        self.assertEqual(data['api'], 1)
        self.assertEqual(data['version'], 'v1.29.0.0 a50d5ac')
        self.assertEqual(data['rtc_epoch'], 978307200)
        self.assertEqual(data['game_checks'], 'not_run_no_rom')

    @unittest.skipUnless((ROOT / 'reference/build/instrumented/mednafen_ngp_libretro.so').exists(), 'Build instrumented reference first')
    def test_instrumented_metadata(self):
        command = [sys.executable, str(ROOT / 'reference/run.py')]
        original = json.loads(subprocess.check_output(command, text=True))
        observed = json.loads(subprocess.check_output(command + ['--core', str(ROOT / 'reference/build/instrumented/mednafen_ngp_libretro.so')], text=True))
        self.assertNotEqual(original.pop('core_sha256'), observed.pop('core_sha256'))
        self.assertEqual(original, observed)

if __name__ == '__main__':
    unittest.main()
