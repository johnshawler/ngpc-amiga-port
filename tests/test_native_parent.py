"""Native course-parent staged service transitions, authored state only."""
import os,shlex,shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class NativeParentTests(unittest.TestCase):
 def test_native_parent(self):
  cc=shlex.split(os.environ.get('CC','cc'))
  if not cc or not shutil.which(cc[0]):self.skipTest('Compiler unavailable')
  with tempfile.TemporaryDirectory() as d:
   exe=str(Path(d)/'parent')
   files=['input_state.c','course_choice.c','task_lifecycle.c','task_allocation.c','random_byte.c','course_parent.c','test_course_parent.c']
   subprocess.run(cc+['-std=c99','-O2','-Wall','-Wextra','-Werror']+[str(ROOT/'native-game'/f) for f in files]+['-o',exe],check=True)
   r=subprocess.run([exe],capture_output=True,text=True,check=True);self.assertIn('131072',r.stdout)
if __name__=='__main__':unittest.main()
