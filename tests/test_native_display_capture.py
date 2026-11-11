"""Optional local capture regression. CI without an Amiga capture skips it."""
import json,subprocess,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];CAP=ROOT/'build/native-display'
@unittest.skipUnless((CAP/'chip.bin').exists() and (CAP/'screen.ppm').exists(),'Run synthetic Amiga display first')
class CaptureTests(unittest.TestCase):
 def verify(self,chip,screen,log):
  return subprocess.run([sys.executable,str(ROOT/'native-renderer/amiga/verify.py'),'--chip',str(chip),'--capture',str(screen),'--log',str(log)],capture_output=True,text=True)
 def test_complete_capture(self):
  r=self.verify(CAP/'chip.bin',CAP/'screen.ppm',CAP/'run.log');self.assertEqual(r.returncode,0,r.stderr);self.assertEqual(json.loads(r.stdout)['result'],'PASS')
 def test_mutations_rejected(self):
  with tempfile.TemporaryDirectory() as d:
   d=Path(d);raw=(CAP/'chip.bin').read_bytes();marker=raw.find(b'NGPDISP1');self.assertGreaterEqual(marker,0)
   bitmap=int.from_bytes(raw[marker+8:marker+12],'big');copper=int.from_bytes(raw[marker+12:marker+16],'big')
   for offset in [marker,bitmap,copper+2]:
    bad=bytearray(raw);bad[offset]^=1;(d/'chip.bin').write_bytes(bad)
    self.assertNotEqual(self.verify(d/'chip.bin',CAP/'screen.ppm',CAP/'run.log').returncode,0)
   (d/'log').write_text((CAP/'run.log').read_text()+'\nfastmem_size=8\n')
   self.assertNotEqual(self.verify(CAP/'chip.bin',CAP/'screen.ppm',d/'log').returncode,0)
   bad=bytearray((CAP/'screen.ppm').read_bytes());bad[-1]^=1;(d/'screen.ppm').write_bytes(bad)
   self.assertNotEqual(self.verify(CAP/'chip.bin',d/'screen.ppm',CAP/'run.log').returncode,0)
SCENE=ROOT/'build/native-scene-display'
@unittest.skipUnless((SCENE/'chip.bin').exists() and (SCENE/'screen.ppm').exists(),'Run synthetic Amiga scene first')
class SceneCaptureTests(unittest.TestCase):
 def test_scene_capture_and_corruption(self):
  command=[sys.executable,str(ROOT/'native-renderer/amiga/verify.py'),'--scene','--chip',str(SCENE/'chip.bin'),'--capture',str(SCENE/'screen.ppm'),'--log',str(SCENE/'run.log')]
  good=subprocess.run(command,capture_output=True,text=True)
  self.assertEqual(good.returncode,0,good.stderr)
  self.assertEqual(json.loads(good.stdout)['result'],'PASS')
  with tempfile.TemporaryDirectory() as d:
   raw=bytearray((SCENE/'chip.bin').read_bytes());marker=raw.find(b'NGPDISP1')
   bitmap=int.from_bytes(raw[marker+8:marker+12],'big');raw[bitmap+52*40+10]^=1
   path=Path(d)/'bad.bin';path.write_bytes(raw)
   command[command.index('--chip')+1]=str(path)
   self.assertNotEqual(subprocess.run(command,capture_output=True).returncode,0)
if __name__=='__main__':unittest.main()
