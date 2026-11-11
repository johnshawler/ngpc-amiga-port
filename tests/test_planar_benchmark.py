"""Public synthetic protocol guards; optional local emulator capture."""
import struct,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'native-renderer/amiga'))
import verify_planar_benchmark as v
LOG='\n'.join(['cpu_model=68020','cpu_cycle_exact=true','cpu_memory_cycle_exact=true','cpu_throttle=0.0','ntsc=false','chipmem_size=4','bogomem_size=0','fastmem_size=0'])
class PlanarBenchmarkTests(unittest.TestCase):
 def fixture(self):
  b=bytearray(2097152);b[256:264]=b'NGPDISP1';struct.pack_into('>3I',b,288,4096,512,0x444f4e45)
  screen=v.expected_pixels();frame=bytes(screen[(y+52)*320+x+80] for y in range(152) for x in range(160))
  checks=[v.fnv(v.planar(frame)),v.fnv(v.planar(bytes((x*7)&255 for y in range(152) for x in range(160)))),v.fnv(v.planar(bytes(((x*7)&255)^1 for y in range(152) for x in range(160))))]
  r=[0]*128;r[:4]=[0x4e474254,3,709379,14200]
  for i in range(12):
   mode=0 if i<6 else 1 if i<9 else 2;variant=i if i<3 else 5-i if i<6 else i%3
   counts=(12160,0) if mode==2 else (3040,9120)
   if variant!=2:counts=(0,0)
   r[4+i*7:11+i*7]=[mode,variant,4,1000000,checks[mode],*counts]
  struct.pack_into('>128I',b,4096,*r);return b
 def test_valid(self):self.assertEqual(v.verify(self.fixture(),LOG)['result'],'PASS')
 def test_rejections(self):
  original=self.fixture()
  for offset in [296,4100,4108,4128,4140]:
   b=bytearray(original);b[offset]^=0x80
   with self.assertRaises(ValueError):v.verify(b,LOG)
  with self.assertRaises(ValueError):v.verify(original,LOG+'\nfastmem_size=8')
 @unittest.skipUnless((ROOT/'build/native-planar-benchmark/timing.json').exists(),'Run planar target benchmark first')
 def test_local_target(self):
  p=ROOT/'build/native-planar-benchmark'
  self.assertEqual(v.verify((p/'chip.bin').read_bytes(),(p/'run.log').read_text())['result'],'PASS')
if __name__=='__main__':unittest.main()
