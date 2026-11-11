"""Cartridge-free opt-in power application contracts, both asset-copy backends."""
import os
import shlex
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

class ShotPowerApplicationTests(unittest.TestCase):
    def test_native(self):
        cc = shlex.split(os.environ.get('CC', 'cc'))
        if not cc or not shutil.which(cc[0]):
            self.skipTest('Compiler unavailable')
        # The check script owns the dependency list shared with the full oracle.
        script = (ROOT / 'tools/check-shot-power-application.sh').read_text()
        sources = []
        for line in script.splitlines():
            if any(line.startswith(name + '=') for name in
                   ('COMMON', 'SHOT', 'AIM', 'OPEN', 'FIRE', 'POWER', 'FIXTURE')):
                sources.extend(shlex.split(line.split('=', 1)[1])[0].split())
        sources = [str(ROOT / source) for source in sources]
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
                self.assertIn('8 enable, scheduler, cancellation and boundary cases PASS', result.stdout)
