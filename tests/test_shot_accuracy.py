"""Cartridge-free accuracy child contracts."""
import os
import shlex
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
class ShotAccuracyTests(unittest.TestCase):
    def test_native(self):
        cc = shlex.split(os.environ.get('CC', 'cc'))
        if not cc or not shutil.which(cc[0]):
            self.skipTest('Compiler unavailable')
        sources = ['shot_accuracy', 'task_lifecycle', 'course_choice',
                   'input_state', 'test_shot_accuracy']
        with tempfile.TemporaryDirectory() as directory:
            exe = str(Path(directory) / 'test')
            subprocess.run(cc + ['-std=c99', '-O2', '-Wall', '-Wextra', '-Werror'] +
                           [str(ROOT / 'native-game' / (s + '.c')) for s in sources] +
                           ['-o', exe], check=True)
            result = subprocess.run([exe], capture_output=True, text=True, check=True)
            self.assertIn('accuracy authored cases: 115454', result.stdout)
