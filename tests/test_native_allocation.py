"""Authored native free-list tests; no original game assets."""
import copy,os,shlex,shutil,subprocess,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'reference'))
from compare_allocation_trace import compare,FIELDS
class NativeAllocationTests(unittest.TestCase):
 def test_native_allocation(self):
  cc=shlex.split(os.environ.get('CC','cc'))
  if not cc or not shutil.which(cc[0]):self.skipTest('Compiler unavailable')
  with tempfile.TemporaryDirectory() as d:
   exe=str(Path(d)/'allocation')
   subprocess.run(cc+['-std=c99','-O2','-Wall','-Wextra','-Werror']+[str(ROOT/'native-game'/f) for f in ['task_allocation.c','test_task_allocation.c']]+['-o',exe],check=True)
   r=subprocess.run([exe],capture_output=True,text=True,check=True);self.assertIn('65536',r.stdout)
 def fixture(self):
  from test_native_lifecycle import NativeLifecycleTests
  a,b,c=NativeLifecycleTests().fixture()
  for i,row in enumerate(b[1:-1]):row.update(zip(FIELDS,[i,i,0]))
  return a,b,copy.deepcopy(b)
 def test_trace(self):self.assertEqual(compare(*self.fixture())['allocation_success'],1)
 def test_bad_trace(self):
  for field,value in [('allocation_calls',0),('allocation_success',0),('allocation_exhausted',1)]:
   a,b,c=self.fixture();b[2][field]=value
   with self.assertRaises(ValueError):compare(a,b,copy.deepcopy(b))
if __name__=='__main__':unittest.main()
