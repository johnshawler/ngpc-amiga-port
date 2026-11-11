"""Cartridge-free native round preparation and unchanged-field contracts."""
import os
import shlex
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]

class RoundPrepareTests(unittest.TestCase):
    def test_native_preparation(self):
        cc = shlex.split(os.environ.get('CC', 'cc'))
        if not cc or not shutil.which(cc[0]):
            self.skipTest('Compiler unavailable')
        names = ('round_prepare', 'random_byte', 'test_round_prepare')
        with tempfile.TemporaryDirectory() as directory:
            executable = str(Path(directory) / 'round_prepare')
            subprocess.run(cc + ['-std=c99', '-O2', '-Wall', '-Wextra', '-Werror'] +
                           [str(ROOT / 'native-game' / (name + '.c')) for name in names] +
                           ['-o', executable], check=True)
            result = subprocess.run([executable], capture_output=True, text=True, check=True)
            self.assertIn('401404 exact-state cases', result.stdout)

if __name__ == '__main__':
    unittest.main()
