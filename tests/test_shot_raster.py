# SPDX-License-Identifier: GPL-2.0-or-later
"""Cartridge-free normal banner raster and per-row composition checks."""
import shutil
import subprocess
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'build/shot-raster-public-tests'


@unittest.skipUnless(shutil.which('cc'),'host C compiler unavailable')
class ShotRasterTests(unittest.TestCase):
    def run_case(self,kind,sources,expected):
        BUILD.mkdir(parents=True,exist_ok=True)
        binary=BUILD/kind
        subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror']+[str(ROOT/p) for p in sources]+['-o',str(binary)],check=True,capture_output=True)
        result=subprocess.run([str(binary)],check=True,capture_output=True,text=True)
        self.assertIn(expected,result.stdout)

    def test_original_semantic_service(self):
        self.run_case('service',['native-game/shot_raster.c','native-game/test_shot_raster.c'],'16777216 IRQ combinations')

    def test_only_requested_plane_moves(self):
        self.run_case('compositor',['native-game/shot_raster.c','native-renderer/scene_renderer.c','native-renderer/tile_renderer.c','native-renderer/test_shot_raster.c'],'24320 synthetic pixels/8bitplanes')


if __name__=='__main__':unittest.main()
