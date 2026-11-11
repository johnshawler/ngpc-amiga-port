"""Synthetic viewport/planar target protocol validation."""
import struct,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'native-renderer/amiga'))
import verify_viewport_benchmark as v
from test_course_benchmark import LOG
class ViewportBenchmarkTests(unittest.TestCase):
 def fixture(self):
  b=bytearray(2097152);b[256:264]=b'NGPDISP1';struct.pack_into('>3I',b,288,4096,512,0x444f4e45)
  r=[0]*128;r[:4]=[0x4e564642,1,709379,14200]
  for n in range(14):
   stage=n%7;r[4+n*8:12+n*8]=[stage,1000000,v.checksum(stage),stage*10,160-stage*20,stage*2+2,int(stage<6),0]
  struct.pack_into('>128I',b,4096,*r);return b
 def test_valid(self):self.assertEqual(v.verify(self.fixture(),LOG)['result'],'PASS')
 def test_corruptions(self):
  for offset in [296,4100,4108,*range(4112,4144,4)]:
   b=self.fixture();b[offset]^=128
   with self.assertRaises(ValueError):v.verify(b,LOG)
  with self.assertRaises(ValueError):v.verify(self.fixture(),LOG+'\nfastmem_size=8')
 @unittest.skipUnless((ROOT/'build/native-viewport-benchmark/timing.json').exists(),'Run target benchmark first')
 def test_local_capture(self):
  p=ROOT/'build/native-viewport-benchmark';self.assertEqual(v.verify((p/'chip.bin').read_bytes(),(p/'run.log').read_text())['result'],'PASS')
if __name__=='__main__':unittest.main()
