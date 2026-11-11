# SPDX-License-Identifier: GPL-2.0-or-later
"""Cartridge-free terrain and malformed-span checks."""
import os,shlex,shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class ShotTerrainTests(unittest.TestCase):
    def test_terrain(self):
        cc=shlex.split(os.environ.get('CC','cc'))
        if not cc or not shutil.which(cc[0]):self.skipTest('Compiler unavailable')
        with tempfile.TemporaryDirectory() as d:
            exe=str(Path(d)/'terrain')
            subprocess.run(cc+['-std=c99','-O2','-Wall','-Wextra','-Werror']+[str(ROOT/'native-game'/n) for n in ('shot_terrain.c','test_shot_terrain.c')]+['-o',exe],check=True)
            result=subprocess.run([exe],capture_output=True,text=True,check=True)
            self.assertIn('4194304 tile/pixel cases',result.stdout)
