"""Cartridge-free native title object contracts; original assets stay private."""
import os
import shlex
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]

class CourseTitleCursorTests(unittest.TestCase):
    def test_title_object(self):
        cc = shlex.split(os.environ.get('CC', 'cc'))
        if not cc or not shutil.which(cc[0]):
            self.skipTest('Compiler unavailable')
        with tempfile.TemporaryDirectory() as directory:
            executable = str(Path(directory) / 'title-cursor')
            subprocess.run(cc + ['-std=c99', '-O2', '-Wall', '-Wextra', '-Werror'] +
                [str(ROOT / 'native-game' / name) for name in
                 ('course_cursor.c', 'game_services.c', 'test_course_title_cursor.c')] +
                ['-o', executable], check=True)
            result = subprocess.run([executable], capture_output=True, text=True, check=True)
            self.assertIn('196608 flag/delay states', result.stdout)

if __name__ == '__main__':
    unittest.main()
