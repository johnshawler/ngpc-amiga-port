"""Cartridge-free ready and genuine camera/club application contracts."""
import unittest
from tests import test_shot_aim as aim
class ShotControlTests(unittest.TestCase):
 run_native=aim.ShotAimTests.run_native
 def test_ready(self):self.run_native('shot_ready test_shot_ready','262144 countdown')
 def test_application(self):
  self.run_native('input_state course_choice task_lifecycle task_allocation random_byte viewport game_services course_parent course_screen course_driver course_palette course_initialization course_cursor course_application course_intro course_title round_record round_prepare course_intro_application tee_graphics tee_setup tee_objects hole_prepare tee_setup_application shot_math shot_prepare shot_scene shot_banner shot_raster shot_palette shot_terrain shot_application shot_aim shot_aim_setup shot_hud aim_world shot_aim_application shot_scene_objects shot_club shot_markers shot_open_application shot_camera shot_ready shot_control_application test_shot_control_application','shot control application:24 authored flows',True)
