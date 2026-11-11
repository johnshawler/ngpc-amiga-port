"""Cartridge-free native course startup and ordered driver checks."""
import os,shlex,shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class CourseInitializationTests(unittest.TestCase):
 def run_native(self,files,expect):
  cc=shlex.split(os.environ.get('CC','cc'))
  if not cc or not shutil.which(cc[0]):self.skipTest('Compiler unavailable')
  with tempfile.TemporaryDirectory() as d:
   exe=str(Path(d)/'test')
   subprocess.run(cc+['-std=c99','-O2','-Wall','-Wextra','-Werror']+[str(ROOT/'native-game'/(f+'.c')) for f in files]+['-o',exe],check=True)
   r=subprocess.run([exe],capture_output=True,text=True,check=True);self.assertIn(expect,r.stdout)
 def test_initialization(self):self.run_native(['course_initialization','course_palette','course_screen','game_services','test_course_initialization'],'256')
 def test_driver(self):self.run_native(['input_state','course_choice','task_lifecycle','task_allocation','random_byte','viewport','game_services','course_parent','course_screen','course_driver','test_course_driver'],'PASS native course driver')

 def test_application(self):
  self.run_native(['input_state','course_choice','task_lifecycle','task_allocation','random_byte','viewport','game_services','course_parent','course_screen','course_driver','course_palette','course_initialization','course_application','course_cursor','../native-renderer/scene_renderer','../native-renderer/tile_renderer','test_course_application'],'PASS native course application')
