"""Cartridge-free streaming loader and pre-takeover failure contracts."""
import os, shlex, shutil, subprocess, tempfile, unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
class FireLoaderTests(unittest.TestCase):
    def run_case(self, name):
        cc = shlex.split(os.environ.get('CC', 'cc'))
        if not cc or not shutil.which(cc[0]): self.skipTest('Compiler unavailable')
        sources = ['fire_stream.c', 'fire_' + name + '_test.c',
                   'fire_boot.c' if name == 'boot' else 'aim_unpack.c']
        with tempfile.TemporaryDirectory() as directory:
            exe = str(Path(directory) / name)
            subprocess.run(cc + ['-std=c99', '-O2', '-Wall', '-Wextra', '-Werror'] +
                           [str(ROOT / 'native-renderer/amiga' / p) for p in sources] + ['-o', exe], check=True)
            subprocess.run([exe], check=True, capture_output=True, text=True)
    def test_stream(self): self.run_case('stream')
    def test_boot(self): self.run_case('boot')
