"""Public synthetic renderer tests. No cartridge, snapshots or private assets."""
from pathlib import Path
import os
import shlex
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class NativeRendererTests(unittest.TestCase):
    def test_c_renderer(self):
        compiler = shlex.split(os.environ.get('CC', 'cc'))
        if not compiler or not shutil.which(compiler[0]):
            self.skipTest('A host C compiler is required')
        with tempfile.TemporaryDirectory() as d:
            exe = str(Path(d) / 'renderer-test')
            subprocess.run(compiler + ['-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                str(ROOT / 'native-renderer/tile_renderer.c'),
                str(ROOT / 'native-renderer/test_renderer.c'), '-o', exe], check=True)
            result = subprocess.run([exe], check=True, capture_output=True, text=True)
            self.assertIn('PASS: 131072 row decodes', result.stdout)

    def test_scene_renderer(self):
        compiler = shlex.split(os.environ.get('CC', 'cc'))
        if not compiler or not shutil.which(compiler[0]):
            self.skipTest('A host C compiler is required')
        with tempfile.TemporaryDirectory() as d:
            exe = str(Path(d) / 'scene-test')
            subprocess.run(compiler + ['-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                str(ROOT / 'native-renderer/scene_renderer.c'),
                str(ROOT / 'native-renderer/test_scene.c'), '-o', exe], check=True)
            result = subprocess.run([exe], check=True, capture_output=True, text=True)
            self.assertIn('PASS: 4000 scene rows', result.stdout)

if __name__ == '__main__':
    unittest.main()
