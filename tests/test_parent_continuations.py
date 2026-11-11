"""Native timer/provider flow tests with authored inputs only."""
import copy,os,shlex,shutil,subprocess,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'reference'))
from compare_random_trace import compare
class ParentContinuationTests(unittest.TestCase):
 def test_native(self):
  cc=shlex.split(os.environ.get('CC','cc'))
  if not cc or not shutil.which(cc[0]):self.skipTest('Compiler unavailable')
  with tempfile.TemporaryDirectory() as d:
   exe=str(Path(d)/'test')
   files=['input_state.c','course_choice.c','task_lifecycle.c','task_allocation.c','random_byte.c','viewport.c','game_services.c','course_parent.c','course_screen.c','test_parent_continuations.c']
   subprocess.run(cc+['-std=c99','-O2','-Wall','-Wextra','-Werror']+[str(ROOT/'native-game'/f) for f in files]+['-o',exe],check=True)
   r=subprocess.run([exe],capture_output=True,text=True,check=True);self.assertIn('262144',r.stdout)
 def fixture(self):
  from test_native_allocation import NativeAllocationTests
  a,b,c=NativeAllocationTests().fixture()
  for i,row in enumerate(b[1:-1]):row['random_calls']=i
  return a,b,copy.deepcopy(b)
 def test_trace(self):self.assertEqual(compare(*self.fixture())['random_calls'],1)
 def test_zero_count(self):
  a,b,c=self.fixture()
  for row in b[1:-1]:row['random_calls']=0
  self.assertEqual(compare(a,b,copy.deepcopy(b))['random_calls'],0)
 def test_bad_trace(self):
  a,b,c=self.fixture();b[2]['random_calls']=-1
  with self.assertRaises(ValueError):compare(a,b,copy.deepcopy(b))
if __name__=='__main__':unittest.main()
