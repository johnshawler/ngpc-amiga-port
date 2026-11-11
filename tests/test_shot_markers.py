"""Cartridge-free bounded shot-marker and delayed-child fixtures."""
import os,shlex,shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class ShotMarkerTests(unittest.TestCase):
 def test_native(self):
  cc=shlex.split(os.environ.get('CC','cc'))
  if not cc or not shutil.which(cc[0]):self.skipTest('Compiler unavailable')
  with tempfile.TemporaryDirectory() as directory:
   exe=str(Path(directory)/'markers')
   sources=['shot_markers','shot_math','tee_objects','game_services','task_lifecycle','course_choice','input_state','test_shot_markers']
   subprocess.run(cc+['-std=c99','-O2','-Wall','-Wextra','-Werror']+[str(ROOT/'native-game'/(n+'.c')) for n in sources]+['-o',exe],check=True)
   result=subprocess.run([exe],capture_output=True,text=True,check=True)
   self.assertIn('32768 positions',result.stdout)
   self.assertIn('explicit active boundaries',result.stdout)
