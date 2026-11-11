"""Reflected map stamps and public preflight; no private assets required."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class CourseScreenReflectionTests(unittest.TestCase):
    def test_both_copy_backends(self):
        cc = shlex.split(os.environ.get('CC', 'cc'))
        if not cc or not shutil.which(cc[0]):
            self.skipTest('Compiler unavailable')
        for backend in ([], ['-DNTM_SCREEN_PRIVATE_ASSETS']):
            with self.subTest(backend=backend), tempfile.TemporaryDirectory() as d:
                exe = str(Path(d) / 'test')
                subprocess.run(cc + ['-std=c99', '-O2', '-Wall', '-Wextra', '-Werror'] + backend +
                               [str(ROOT / 'native-game' / f) for f in
                                ('course_screen.c', 'test_course_screen_reflection.c')] +
                               ['-o', exe], check=True)
                result = subprocess.run([exe], capture_output=True, text=True, check=True)
                self.assertIn('4096 authored cases', result.stdout)
