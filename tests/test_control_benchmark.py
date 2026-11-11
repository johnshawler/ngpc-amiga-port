"""Synthetic native-control timing report checks, no game or firmware assets."""
import struct,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'native-renderer/amiga'))
import verify_control_benchmark as v
LOG='\n'.join(['cpu_model=68020','cpu_cycle_exact=true','cpu_memory_cycle_exact=true','cpu_throttle=0.0','ntsc=false','chipmem_size=4','bogomem_size=0','chipset=aga','Model: "A1200OG".','fastmem_size=0'])
class ControlBenchmarkTests(unittest.TestCase):
 def fixture(self):
  b=bytearray(2097152);b[256:264]=b'NGPDISP1';struct.pack_into('>3I',b,288,4096,320,0x444f4e45)
  r=[0]*80;r[:4]=[0x4e435442,1,709379,14200]
  for n in range(8):
   variant=n if n<4 else 7-n;data=v.expected(variant);digest=2166136261
   for x in data:digest=((digest^x)*16777619)&0xffffffff
   r[4+n*8:12+n*8]=[variant,16384,1000000,digest,*struct.unpack('>4I',data)]
  struct.pack_into('>80I',b,4096,*r);return b
 def test_valid(self):self.assertEqual(v.verify(self.fixture(),LOG)['result'],'PASS')
 def test_corruptions(self):
  for offset in [296,4100,4108,4116,4124,4128]:
   b=self.fixture();b[offset]^=128
   with self.assertRaises(ValueError):v.verify(b,LOG)
  with self.assertRaises(ValueError):v.verify(self.fixture(),LOG+'\nfastmem_size=8')
  with self.assertRaises(ValueError):v.verify(self.fixture(),LOG.replace('ntsc=false','ntsc=true'))
 @unittest.skipUnless((ROOT/'build/native-control-benchmark/timing.json').exists(),'Run control target benchmark first')
 def test_local_capture(self):
  p=ROOT/'build/native-control-benchmark';self.assertEqual(v.verify((p/'chip.bin').read_bytes(),(p/'run.log').read_text())['result'],'PASS')
if __name__=='__main__':unittest.main()
