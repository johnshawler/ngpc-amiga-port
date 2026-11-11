"""Synthetic course/input target report validation, no game/firmware assets."""
import struct,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'native-renderer/amiga'))
import verify_course_benchmark as v
LOG='\n'.join(['cpu_model=68020','cpu_cycle_exact=true','cpu_memory_cycle_exact=true','cpu_throttle=0.0','ntsc=false','chipmem_size=4','bogomem_size=0','chipset=aga','Model: "A1200OG".','fastmem_size=0'])
class CourseBenchmarkTests(unittest.TestCase):
 def fixture(self):
  b=bytearray(2097152);b[256:264]=b'NGPDISP1';struct.pack_into('>3I',b,288,4096,448,0x444f4e45)
  r=[0]*112;r[:4]=[0x4e435342,1,709379,14200]
  for n in range(8):
   variant=n if n<4 else 7-n;data,selection,result,sounds,finishes=v.expected(variant);digest=2166136261
   for x in data+bytes([selection,result]):digest=((digest^x)*16777619)&0xffffffff
   r[4+n*12:16+n*12]=[variant,16384,1000000,digest,*struct.unpack('>4I',data),selection*256+result,sounds,finishes,0]
  struct.pack_into('>112I',b,4096,*r);return b
 def test_valid(self):self.assertEqual(v.verify(self.fixture(),LOG)['result'],'PASS')
 def test_corruptions(self):
  for offset in [296,4100,4108,4116,4124,4128,4144,4148,4152]:
   b=self.fixture();b[offset]^=128
   with self.assertRaises(ValueError):v.verify(b,LOG)
  with self.assertRaises(ValueError):v.verify(self.fixture(),LOG+'\nfastmem_size=8')
  with self.assertRaises(ValueError):v.verify(self.fixture(),LOG.replace('chipset=aga','chipset=ecs'))
 @unittest.skipUnless((ROOT/'build/native-course-benchmark/timing.json').exists(),'Run course target benchmark first')
 def test_local_capture(self):
  p=ROOT/'build/native-course-benchmark';self.assertEqual(v.verify((p/'chip.bin').read_bytes(),(p/'run.log').read_text())['result'],'PASS')
if __name__=='__main__':unittest.main()
