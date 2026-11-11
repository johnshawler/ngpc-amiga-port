"""Authored game-service, viewport bridge and complete-parent tests."""
import copy,os,shlex,shutil,subprocess,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'reference'))
from compare_service_trace import compare,FIELDS
class GameServiceTests(unittest.TestCase):
 def test_native(self):
  cc=shlex.split(os.environ.get('CC','cc'))
  if not cc or not shutil.which(cc[0]):self.skipTest('Compiler unavailable')
  with tempfile.TemporaryDirectory() as d:
   exe=str(Path(d)/'test')
   files=['input_state.c','course_choice.c','task_lifecycle.c','task_allocation.c','random_byte.c','viewport.c','game_services.c','course_parent.c','course_screen.c','render_bridge.c','test_game_services.c']
   subprocess.run(cc+['-std=c99','-O2','-Wall','-Wextra','-Werror']+[str(ROOT/'native-game'/f) for f in files]+[str(ROOT/'native-renderer'/f) for f in ['scene_renderer.c','tile_renderer.c']]+['-o',exe],check=True)
   r=subprocess.run([exe],capture_output=True,text=True,check=True);self.assertIn('102400',r.stdout);self.assertIn('1536',r.stdout)
 def fixture(self):
  from test_parent_continuations import ParentContinuationTests
  a,b,c=ParentContinuationTests().fixture()
  for i,row in enumerate(b[1:-1]):row.update(zip(FIELDS,[i,i,0,0,0]))
  return a,b,copy.deepcopy(b)
 def test_trace(self):self.assertEqual(compare(*self.fixture())['service_fade'],1)
 def test_bad_trace(self):
  for field,value in [('service_calls',0),('service_fade',0),('service_reset',1)]:
   a,b,c=self.fixture();b[2][field]=value
   with self.assertRaises(ValueError):compare(a,b,copy.deepcopy(b))
if __name__=='__main__':unittest.main()
