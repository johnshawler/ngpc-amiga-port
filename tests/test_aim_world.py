# SPDX-License-Identifier: GPL-2.0-or-later
"""Cartridge-free world-map loading and malformed borrowed-span checks."""
import os,shlex,shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class AimWorldTests(unittest.TestCase):
    def test_world(self):
        cc=shlex.split(os.environ.get('CC','cc'))
        if not cc or not shutil.which(cc[0]):self.skipTest('Compiler unavailable')
        with tempfile.TemporaryDirectory() as d:
            exe=str(Path(d)/'world')
            subprocess.run(cc+['-std=c99','-O2','-Wall','-Wextra','-Werror']+[str(ROOT/'native-game'/n) for n in ('aim_world.c','test_aim_world.c')]+['-o',exe],check=True)
            result=subprocess.run([exe],capture_output=True,text=True,check=True)
            self.assertIn('65536 wrapped origin pairs',result.stdout)
