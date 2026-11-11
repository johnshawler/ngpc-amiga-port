"""Cartridge-free parent power/cancel contracts."""
import os
import shlex
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
class ShotPowerTests(unittest.TestCase):
    def test_native(self):
        cc = shlex.split(os.environ.get('CC', 'cc'))
        if not cc or not shutil.which(cc[0]):
            self.skipTest('Compiler unavailable')
        sources = ['shot_power', 'shot_aim', 'task_allocation', 'task_lifecycle',
                   'course_choice', 'input_state', 'test_shot_power']
        with tempfile.TemporaryDirectory() as directory:
            exe = str(Path(directory) / 'test')
            subprocess.run(cc + ['-std=c99', '-O2', '-Wall', '-Wextra', '-Werror'] +
                           [str(ROOT / 'native-game' / (s + '.c')) for s in sources] +
                           ['-o', exe], check=True)
            result = subprocess.run([exe], capture_output=True, text=True, check=True)
            self.assertIn('1049633 allocation, delay, input and cancellation cases PASS', result.stdout)
