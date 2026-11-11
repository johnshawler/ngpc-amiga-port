"""Cartridge-free exact shot geometry and opt-in scene setup contracts."""
import os,shlex,shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class ShotAimTests(unittest.TestCase):
 def run_native(self,names,expected,renderer=False):
  cc=shlex.split(os.environ.get('CC','cc'))
  if not cc or not shutil.which(cc[0]):self.skipTest('Compiler unavailable')
  with tempfile.TemporaryDirectory() as directory:
   exe=str(Path(directory)/'test')
   subprocess.run(cc+['-std=c99','-O2','-Wall','-Wextra','-Werror']+[str(ROOT/'native-game'/(n+'.c')) for n in names.split()]+([str(ROOT/'native-renderer'/'scene_renderer.c'),str(ROOT/'native-renderer'/'tile_renderer.c')] if renderer else [])+['-o',exe],check=True)
   result=subprocess.run([exe],text=True,capture_output=True,check=True);self.assertIn(expected,result.stdout)
 def test_helpers(self):
  self.run_native('shot_aim test_shot_aim','100000 angles')
 def test_application(self):
  self.run_native('input_state course_choice task_lifecycle task_allocation random_byte viewport game_services course_parent course_screen course_driver course_palette course_initialization course_cursor course_application course_intro course_title round_record round_prepare course_intro_application tee_graphics tee_setup tee_objects hole_prepare tee_setup_application shot_math shot_prepare shot_scene shot_banner shot_raster shot_palette shot_terrain shot_application shot_aim shot_aim_setup shot_hud aim_world shot_aim_application test_shot_aim_application','shot aim application:24 authored flows',True)
if __name__=='__main__':unittest.main()
