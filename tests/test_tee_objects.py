"""Cartridge-free bounded native tee object service checks."""
import os
import shlex
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class TeeObjectTests(unittest.TestCase):
 def test_native(self):
  cc=shlex.split(os.environ.get('CC','cc'))
  if not cc or not shutil.which(cc[0]):self.skipTest('Compiler unavailable')
  with tempfile.TemporaryDirectory() as directory:
   exe=str(Path(directory)/'test')
   sources=[str(ROOT/'native-game'/(name+'.c')) for name in ('tee_objects','game_services','test_tee_objects')]
   subprocess.run(cc+['-std=c99','-O2','-Wall','-Wextra','-Werror']+sources+['-o',exe],check=True)
   result=subprocess.run([exe],capture_output=True,text=True,check=True)
   self.assertIn('65536',result.stdout)
   self.assertIn('131072',result.stdout)
