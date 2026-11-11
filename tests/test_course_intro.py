"""Cartridge-free native owner, record, scenery and ordering contracts."""
import os
import shlex
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]

class CourseIntroTests(unittest.TestCase):
    def test_native_intro(self):
        cc = shlex.split(os.environ.get('CC', 'cc'))
        if not cc or not shutil.which(cc[0]):
            self.skipTest('Compiler unavailable')
        names = ['input_state', 'course_intro', 'course_palette', 'course_screen',
                 'game_services', 'task_allocation', 'task_lifecycle',
                 'course_choice', 'test_course_intro']
        with tempfile.TemporaryDirectory() as directory:
            executable = str(Path(directory) / 'intro')
            subprocess.run(cc + ['-std=c99', '-O2', '-Wall', '-Wextra', '-Werror'] +
                           [str(ROOT / 'native-game' / (name + '.c')) for name in names] +
                           ['-o', executable], check=True)
            result = subprocess.run([executable], capture_output=True, text=True, check=True)
            self.assertIn('131072 signed-delay cases passed', result.stdout)

if __name__ == '__main__':
    unittest.main()
