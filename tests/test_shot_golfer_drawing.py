"""Cartridge-free complete golfer initial drawing contract checks."""
import os
import shlex
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

class GolferDrawingTests(unittest.TestCase):
    def test_native(self):
        cc = shlex.split(os.environ.get('CC', 'cc'))
        if not cc or not shutil.which(cc[0]):
            self.skipTest('Compiler unavailable')
        names = ('shot_golfer_drawing', 'shot_golfer_features',
                 'shot_golfer_animation', 'shot_scene_objects', 'shot_markers',
                 'shot_math', 'shot_aim', 'tee_objects', 'game_services',
                 'course_screen', 'task_lifecycle', 'course_choice',
                 'input_state', 'test_shot_golfer_drawing')
        sources = [str(ROOT / 'native-game' / (name + '.c')) for name in names]
        with tempfile.TemporaryDirectory() as directory:
            for variant in ('default', 'private'):
                extra = []
                if variant == 'private':
                    extra = ['-DNTM_SCREEN_PRIVATE_ASSETS'] + [
                        str(ROOT / 'native-renderer/amiga' / (name + '.c'))
                        for name in ('intro_copy', 'selector_memory')]
                exe = str(Path(directory) / variant)
                subprocess.run(cc + ['-std=c99', '-O2', '-Wall', '-Wextra',
                                     '-Werror'] + extra + sources + ['-o', exe],
                               check=True)
                result = subprocess.run([exe], capture_output=True, text=True,
                                        check=True)
                self.assertIn('1200 authored whole callback cases PASS', result.stdout)
