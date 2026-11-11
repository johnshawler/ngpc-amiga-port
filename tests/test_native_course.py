"""Native course selection and trace protocol tests without private game data."""
import copy,os,shlex,shutil,subprocess,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'reference'))
from compare_course_trace import compare,FIELDS
class NativeCourseTests(unittest.TestCase):
 def test_native_transitions(self):
  compiler=shlex.split(os.environ.get('CC','cc'))
  if not compiler or not shutil.which(compiler[0]):self.skipTest('Compiler unavailable')
  with tempfile.TemporaryDirectory() as d:
   exe=str(Path(d)/'course')
   subprocess.run(compiler+['-std=c99','-O2','-Wall','-Wextra','-Werror']+[str(ROOT/'native-game'/f) for f in ['input_state.c','course_choice.c','test_course_choice.c']]+['-o',exe],check=True)
   result=subprocess.run([exe],capture_output=True,text=True,check=True)
   self.assertIn('33619968',result.stdout)
 def fixture(self):
  a=[dict(type='metadata',core_sha256='original'),dict(type='frame',frame=0,video='x'),dict(type='frame',frame=1,video='y'),dict(type='state',after_frames=2)]
  b=copy.deepcopy(a);b[0]['core_sha256']='observer'
  for i,row in enumerate(b[1:-1]):
   row['control_calls']=i
   row.update(zip(FIELDS,[i,0,i,i,0,0]))
  return a,b,copy.deepcopy(b)
 def test_trace_valid(self):self.assertEqual(compare(*self.fixture())['course_confirm'],1)
 def test_trace_corruptions(self):
  for field,value in [('course_calls',None),('course_calls',0),('course_sounds',1),('course_finishes',0),('course_confirm',2),('course_cancel',1),('course_option_confirm',1)]:
   a,b,c=self.fixture();b[2][field]=value;c=copy.deepcopy(b)
   with self.assertRaises(ValueError):compare(a,b,c)
  a,b,c=self.fixture();c[1]['video']='wrong'
  with self.assertRaises(ValueError):compare(a,b,c)
if __name__=='__main__':unittest.main()
