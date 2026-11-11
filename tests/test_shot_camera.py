"""Cartridge-free native camera/control and incremental map scrolling."""
import unittest
from tests import test_shot_aim as aim
class ShotCameraTests(unittest.TestCase):
 run_native=aim.ShotAimTests.run_native
 def test_camera(self):
  self.run_native('shot_camera shot_markers shot_math tee_objects game_services task_allocation task_lifecycle course_choice input_state test_shot_camera','PASS camera:65536 input pairs')
