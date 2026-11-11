"""Cartridge-free genuine scene-child and first-visible application contracts."""
import unittest
from tests import test_shot_aim as aim
class ShotOpenTests(unittest.TestCase):
 run_native=aim.ShotAimTests.run_native
 def test_club(self):
  self.run_native('shot_club shot_aim shot_scene_objects tee_objects game_services course_screen test_shot_club','630 authored bounds')
 def test_scene_objects(self):
  self.run_native('shot_scene_objects tee_objects game_services test_shot_scene_objects','4096 power cases')
 def test_open_application(self):
  self.run_native('input_state course_choice task_lifecycle task_allocation random_byte viewport game_services course_parent course_screen course_driver course_palette course_initialization course_cursor course_application course_intro course_title round_record round_prepare course_intro_application tee_graphics tee_setup tee_objects hole_prepare tee_setup_application shot_math shot_prepare shot_scene shot_banner shot_raster shot_palette shot_terrain shot_application shot_aim shot_aim_setup shot_hud aim_world shot_aim_application shot_scene_objects shot_club shot_markers shot_open_application test_shot_open_application','shot open application:24 authored flows',True)
