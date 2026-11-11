"""Native course-screen command and tilemap tests, no cartridge required."""
import os,shlex,shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class CourseScreenTests(unittest.TestCase):
 def test_native(self):
  cc=shlex.split(os.environ.get('CC','cc'))
  if not cc or not shutil.which(cc[0]):self.skipTest('Compiler unavailable')
  with tempfile.TemporaryDirectory() as d:
   exe=str(Path(d)/'test')
   subprocess.run(cc+['-std=c99','-O2','-Wall','-Wextra','-Werror']+[str(ROOT/'native-game'/f) for f in ['course_screen.c','test_course_screen.c']]+['-o',exe],check=True)
   r=subprocess.run([exe],capture_output=True,text=True,check=True);self.assertIn('221952',r.stdout)
