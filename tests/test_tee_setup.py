"""Cartridge-free tee overview and integrated native scheduling contracts."""
import os,shlex,shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class TeeSetupTests(unittest.TestCase):
 def test_overview(self):
  self.run_native('tee_setup tee_graphics tee_objects course_title course_screen course_palette game_services task_allocation task_lifecycle course_choice input_state test_tee_setup','458752 timer states')
 def test_application(self):
  self.run_native('input_state course_choice task_lifecycle task_allocation random_byte viewport game_services course_parent course_screen course_driver course_palette course_initialization course_cursor course_application course_intro course_title round_record round_prepare course_intro_application tee_graphics tee_setup tee_objects hole_prepare tee_setup_application test_tee_setup_application','30 real traversals',renderer=True)
 def run_native(self,names,expected,renderer=False):
  cc=shlex.split(os.environ.get('CC','cc'))
  if not cc or not shutil.which(cc[0]):self.skipTest('Compiler unavailable')
  with tempfile.TemporaryDirectory() as directory:
   exe=str(Path(directory)/'test')
   subprocess.run(cc+['-std=c99','-O2','-Wall','-Wextra','-Werror']+[str(ROOT/'native-game'/(n+'.c')) for n in names.split()]+([str(ROOT/'native-renderer'/'scene_renderer.c'),str(ROOT/'native-renderer'/'tile_renderer.c')] if renderer else [])+['-o',exe],check=True)
   result=subprocess.run([exe],text=True,capture_output=True,check=True);self.assertIn(expected,result.stdout)
if __name__=='__main__':unittest.main()
