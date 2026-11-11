# SPDX-License-Identifier: GPL-2.0-or-later
import subprocess,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class ShotHud(unittest.TestCase):
 def test_authored_hud(self):
  out=ROOT/'build/shot-hud-public';out.mkdir(parents=True,exist_ok=True)
  sources=['shot_hud.c','tee_graphics.c','course_screen.c','test_shot_hud.c']
  subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror',*[str(ROOT/'native-game'/p) for p in sources],'-o',str(out/'test')],check=True)
  subprocess.run([str(out/'test')],check=True)
if __name__=='__main__':unittest.main()
