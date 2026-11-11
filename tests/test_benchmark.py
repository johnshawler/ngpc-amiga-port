"""Synthetic benchmark-parser guards plus optional private local capture check."""
import importlib.util,struct,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'native-renderer/amiga'))
import verify_benchmark as bench
LOG='\n'.join(['cpu_model=68020','cpu_cycle_exact=true','cpu_memory_cycle_exact=true','cpu_throttle=0.0','ntsc=false','chipmem_size=4','bogomem_size=0','fastmem_size=0'])
class BenchmarkTests(unittest.TestCase):
 def fixture(self):
  ram=bytearray(2097152);ram[256:264]=b'NGPDISP1'
  struct.pack_into('>3I',ram,288,4096,384,0x444f4e45)
  screen=bench.expected_pixels();frame=bytes(screen[(y+52)*320+x+80] for y in range(152) for x in range(160))
  checks=[bench.fnv(bench.planar(frame))]*6+[bench.fnv(bytes([253])*160)]*2+[bench.fnv(bench.planar(bytes((x*7)&255 for y in range(152) for x in range(160))))]
  values=[0]*96;values[:4]=[0x4e474254,2,709379,14200]
  for i in range(9):
   mode=0 if i<4 else 1 if i<6 else 2 if i<8 else 3;cached=i in [1,2,5,7]
   values[4+i*7:11+i*7]=[mode,int(cached),1 if mode==1 else 4,1000000,checks[i],147 if i==5 else int(cached),0 if i==5 or not cached else 587]
  struct.pack_into('>96I',ram,4096,*values);return ram
 def test_valid_synthetic(self):
  self.assertEqual(bench.verify(self.fixture(),LOG)['result'],'PASS')
 def test_corruptions(self):
  base=self.fixture()
  for offset in [288+8,4096+4,4096+8,4096+12,4096+8*4,4096+9*4]:
   bad=bytearray(base);bad[offset]^=0x80
   with self.assertRaises(ValueError):bench.verify(bad,LOG)
  with self.assertRaises(ValueError):bench.verify(base,LOG+'\nfastmem_size=8')
 @unittest.skipUnless((ROOT/'build/native-benchmark/timing.json').exists(),'Run target benchmark first')
 def test_actual_local_capture(self):
  p=ROOT/'build/native-benchmark'
  self.assertEqual(bench.verify((p/'chip.bin').read_bytes(),(p/'run.log').read_text())['result'],'PASS')
if __name__=='__main__':unittest.main()
