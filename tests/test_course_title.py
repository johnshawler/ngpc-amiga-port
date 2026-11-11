"""Cartridge-free title, tournament, skip and completion contracts."""
import os
import shlex
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]

class CourseTitleTests(unittest.TestCase):
    def test_native_title(self):
        cc = shlex.split(os.environ.get('CC', 'cc'))
        if not cc or not shutil.which(cc[0]):
            self.skipTest('Compiler unavailable')
        names = ['input_state', 'course_title', 'course_screen',
                 'game_services', 'task_allocation', 'task_lifecycle',
                 'course_choice', 'test_course_title']
        with tempfile.TemporaryDirectory() as directory:
            executable = str(Path(directory) / 'intro')
            subprocess.run(cc + ['-std=c99', '-O2', '-Wall', '-Wextra', '-Werror'] +
                           [str(ROOT / 'native-game' / (name + '.c')) for name in names] +
                           ['-o', executable], check=True)
            result = subprocess.run([executable], capture_output=True, text=True, check=True)
            self.assertIn('196608 timer states', result.stdout)

if __name__ == '__main__':
    unittest.main()
