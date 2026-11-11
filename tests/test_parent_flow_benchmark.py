"""Parent-flow target protocol tests, synthetic data only."""
import struct,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'native-renderer/amiga'))
import verify_parent_flow_benchmark as v
from test_course_benchmark import LOG
class ParentFlowBenchmarkTests(unittest.TestCase):
 def fixture(self):
  b=bytearray(2097152);b[256:264]=b'NGPDISP1';struct.pack_into('>3I',b,288,4096,448,0x444f4e45)
  r=[0]*112;r[:4]=[0x4e504642,1,709379,14200]
  for n in range(6):
   variant=n if n<3 else 5-n
   selection=0 if variant==2 else 1;remaining=0 if variant==2 else 85;flags=53 if variant==1 else 181
   mapped=204 if variant==1 else 2 if variant==2 else 0;substate=0 if variant==1 else 165;word=0xaabb if variant==1 else 4
   digest=2166136261
   for x in [selection,17,remaining,flags,mapped,substate,255,255]:digest=((digest^x)*16777619)&0xffffffff
   counts=([31,0,1,1],[7,0,1,1],[91,30,31,31])[variant]
   r[4+n*16:20+n*16]=[variant,128,1000000,*[x*128 for x in counts],digest,selection,17,remaining,word*256+flags,65535,mapped,substate,0]
  struct.pack_into('>112I',b,4096,*r);return b
 def test_valid(self):self.assertEqual(v.verify(self.fixture(),LOG)['result'],'PASS')
 def test_corruptions(self):
  for offset in [296,4100,4108,*range(4112,4176,4)]:
   b=self.fixture();b[offset]^=128
   with self.assertRaises(ValueError):v.verify(b,LOG)
  with self.assertRaises(ValueError):v.verify(self.fixture(),LOG+'\nfastmem_size=8')
 @unittest.skipUnless((ROOT/'build/native-parent-flow-benchmark/timing.json').exists(),'Run target benchmark first')
 def test_local_capture(self):
  p=ROOT/'build/native-parent-flow-benchmark';self.assertEqual(v.verify((p/'chip.bin').read_bytes(),(p/'run.log').read_text())['result'],'PASS')
if __name__=='__main__':unittest.main()
