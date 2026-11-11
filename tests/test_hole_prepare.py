# SPDX-License-Identifier: GPL-2.0-or-later
"""Cartridge-free exact-state and invalid-resource preparation tests."""
import os
import shlex
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
class HolePrepareTests(unittest.TestCase):
    def test_native_preparation(self):
        cc = shlex.split(os.environ.get('CC', 'cc'))
        if not cc or not shutil.which(cc[0]):
            self.skipTest('Compiler unavailable')
        with tempfile.TemporaryDirectory() as directory:
            exe = str(Path(directory) / 'hole')
            subprocess.run(cc + ['-std=c99', '-O2', '-Wall', '-Wextra', '-Werror'] +
                           [str(ROOT / 'native-game' / (name + '.c')) for name in
                            ('hole_prepare', 'random_byte', 'test_hole_prepare')] + ['-o', exe], check=True)
            result = subprocess.run([exe], capture_output=True, text=True, check=True)
            self.assertIn('110592 exact-state cases', result.stdout)
