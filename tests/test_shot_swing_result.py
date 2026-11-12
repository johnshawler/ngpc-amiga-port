"""Cartridge-free swing/result contracts. No ROM or target timing claims."""
import os
import re
import shlex
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

class ShotSwingResultTests(unittest.TestCase):
    def run_contract(self, name):
        cc = shlex.split(os.environ.get('CC', 'cc'))
        if not cc or not shutil.which(cc[0]):
            self.skipTest('Compiler unavailable')
        script = (ROOT / ('tools/check-shot-' + name + '.sh')).read_text()
        # Only literal source-list declarations, never shell evaluation.
        groups = dict(re.findall(r"^([A-Z_]+)='([^']*)'$", script, re.M))
        if name.endswith('application'):
            sources = ' '.join(groups[k] for k in
                ('FIXTURE', 'COMMON', 'SHOT', 'AIM', 'OPEN', 'FIRE', 'POWER')).split()
        else:
            sources = ['native-game/test_shot_' + name.replace('-', '_') + '.c']
            sources += groups.get('SOURCES', groups.get('COMMON',
                'native-game/shot_' + name.replace('-', '_') + '.c')).split()
        self.assertTrue(all(s.startswith('native-game/') and s.endswith('.c') for s in sources))
        with tempfile.TemporaryDirectory() as directory:
            for variant in ('default', 'private'):
                extra = []
                if variant == 'private':
                    extra = ['-DNTM_SCREEN_PRIVATE_ASSETS'] + [str(ROOT /
                        ('native-renderer/amiga/' + n + '.c')) for n in
                        ('intro_copy', 'selector_memory')]
                exe = str(Path(directory) / variant)
                subprocess.run(cc + ['-std=c99', '-O2', '-Wall', '-Wextra',
                    '-Werror', '-I', str(ROOT / 'native-game')] + extra +
                    [str(ROOT / s) for s in sources] + ['-o', exe], check=True)
                result = subprocess.run([exe], capture_output=True, text=True, check=True)
                self.assertIn('PASS', result.stdout.upper())

    def test_swing(self): self.run_contract('swing')
    def test_trajectory(self): self.run_contract('trajectory-swing')
    def test_exit(self): self.run_contract('swing-exit')
    def test_miss(self): self.run_contract('miss')
    def test_perfect(self): self.run_contract('perfect')
    def test_swing_application(self): self.run_contract('swing-application')
    def test_result_application(self): self.run_contract('result-application')
