"""Cartridge-free native fire parent and genuine child routine contracts."""
import unittest
from tests import test_shot_aim as aim

class ShotFireChildrenTests(unittest.TestCase):
    run_native = aim.ShotAimTests.run_native

    def test_fire(self):
        self.run_native("shot_fire shot_markers shot_math tee_objects game_services course_palette task_allocation task_lifecycle course_choice input_state test_shot_fire", "shot fire:")

    def test_projection(self):
        self.run_native("shot_projection shot_markers shot_math tee_objects game_services task_lifecycle course_choice input_state test_shot_projection", "shot projection:")

    def test_spin(self):
        self.run_native("shot_spin task_lifecycle course_choice input_state course_screen test_shot_spin", "shot spin:")

    def test_trajectory(self):
        self.run_native("shot_trajectory tee_objects course_palette course_screen game_services test_shot_trajectory", "shot trajectory:")

    def test_golfer(self):
        self.run_native("shot_golfer shot_projection shot_terrain shot_aim shot_math shot_markers tee_objects game_services course_palette course_screen task_lifecycle course_choice input_state test_shot_golfer", "shot golfer:")

    def test_golfer_animation(self):
        self.run_native("shot_golfer_animation course_screen test_shot_golfer_animation", "shot golfer animation:")
