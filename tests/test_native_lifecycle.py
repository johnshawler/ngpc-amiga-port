"""Native lifecycle tests use only authored synthetic state, never a cartridge."""
import copy,os,shlex,shutil,subprocess,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'reference'))
from compare_lifecycle_trace import compare,FIELDS
class NativeLifecycleTests(unittest.TestCase):
 def test_native_lifecycle(self):
  cc=shlex.split(os.environ.get('CC','cc'))
  if not cc or not shutil.which(cc[0]):self.skipTest('Compiler unavailable')
  with tempfile.TemporaryDirectory() as d:
   exe=str(Path(d)/'test')
   subprocess.run(cc+['-std=c99','-O2','-Wall','-Wextra','-Werror']+[str(ROOT/'native-game'/f) for f in ['input_state.c','course_choice.c','task_lifecycle.c','test_task_lifecycle.c']]+['-o',exe],check=True)
   r=subprocess.run([exe],capture_output=True,text=True,check=True);self.assertIn('131072',r.stdout)
 def fixture(self):
  from test_native_course import NativeCourseTests
  a,b,c=NativeCourseTests().fixture()
  for i,row in enumerate(b[1:-1]):row.update(zip(FIELDS,[i,0,i,i]))
  return a,b,copy.deepcopy(b)
 def test_trace(self):self.assertEqual(compare(*self.fixture())['lifecycle_recycled'],1)
 def test_bad_trace(self):
  for field,value in [('lifecycle_calls',0),('lifecycle_deferred',1),('lifecycle_unlinked',0),('lifecycle_recycled',2)]:
   a,b,c=self.fixture();b[2][field]=value
   with self.assertRaises(ValueError):compare(a,b,copy.deepcopy(b))
if __name__=='__main__':unittest.main()
