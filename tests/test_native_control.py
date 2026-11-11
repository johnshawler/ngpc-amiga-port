"""Native control state and private-trace comparison protocol tests, no ROM."""
import copy,os,shlex,shutil,subprocess,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'reference'))
from compare_control_trace import compare
class NativeControlTests(unittest.TestCase):
 def test_native_exhaustive(self):
  compiler=shlex.split(os.environ.get('CC','cc'))
  if not compiler or not shutil.which(compiler[0]):self.skipTest('Compiler unavailable')
  with tempfile.TemporaryDirectory() as d:
   exe=str(Path(d)/'control')
   subprocess.run(compiler+['-std=c99','-O2','-Wall','-Wextra','-Werror',str(ROOT/'native-game/input_state.c'),str(ROOT/'native-game/test_input_state.c'),'-o',exe],check=True)
   r=subprocess.run([exe],capture_output=True,text=True,check=True)
   self.assertIn('16842752',r.stdout)
 def fixture(self):
  original=[dict(type='metadata',core_sha256='original',input='synthetic'),dict(type='frame',frame=0,video='a',audio='b',input_mask=0),dict(type='frame',frame=1,video='c',audio='d',input_mask=1),dict(type='state',after_frames=2,state='synthetic')]
  observed=copy.deepcopy(original);observed[0]['core_sha256']='observer'
  observed[1]['control_calls']=0;observed[2]['control_calls']=1
  return original,observed,copy.deepcopy(observed)
 def test_trace_valid(self):self.assertEqual(compare(*self.fixture())['control_calls'],1)
 def test_trace_corruptions(self):
  for field,value in [('control_calls',None),('video','wrong'),('audio','wrong'),('input_mask',9),('frame',9)]:
   a,b,c=self.fixture();b[2][field]=value;c=copy.deepcopy(b)
   with self.assertRaises(ValueError):compare(a,b,c)
  a,b,c=self.fixture();c[1]['video']='wrong'
  with self.assertRaises(ValueError):compare(a,b,c)
  a,b,c=self.fixture();b[-1]['state']='wrong';c=copy.deepcopy(b)
  with self.assertRaises(ValueError):compare(a,b,c)
if __name__=='__main__':unittest.main()
