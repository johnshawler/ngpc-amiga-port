"""Synthetic composition-cache benchmark protocol and optional target capture."""
import struct,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'native-renderer/amiga'))
import verify_composition_benchmark as v
LOG='\n'.join(['cpu_model=68020','cpu_cycle_exact=true','cpu_memory_cycle_exact=true','cpu_throttle=0.0','ntsc=false','chipmem_size=4','bogomem_size=0','fastmem_size=0'])
class CompositionBenchmarkTests(unittest.TestCase):
 def fixture(self):
  b=bytearray(2097152);b[256:264]=b'NGPDISP1';struct.pack_into('>3I',b,288,4096,384,0x444f4e45)
  checks=[]
  for scroll in [251,250]:
   s=v.expected_pixels(scroll);checks.append(v.fnv(v.planar(bytes(s[(y+52)*320+x+80] for y in range(152) for x in range(160)))))
  checks.append(v.fnv(bytes([253])*160))
  r=[0]*96;r[:4]=[0x4e474254,4,709379,14200]
  for i in range(8):
   mode=0 if i<4 else 1 if i<6 else 2;variant=int(i in [1,2,5,7])
   counts=(608,0) if mode==1 else (152,456)
   if not variant:counts=(0,0)
   r[4+i*7:11+i*7]=[mode,variant,4,1000000,checks[mode],*counts]
  struct.pack_into('>96I',b,4096,*r);return b
 def test_valid(self):self.assertEqual(v.verify(self.fixture(),LOG)['result'],'PASS')
 def test_rejections(self):
  original=self.fixture()
  for offset in [296,4100,4108,4128,4140]:
   b=bytearray(original);b[offset]^=0x80
   with self.assertRaises(ValueError):v.verify(b,LOG)
  with self.assertRaises(ValueError):v.verify(original,LOG+'\nfastmem_size=8')
 @unittest.skipUnless((ROOT/'build/native-composition-benchmark/timing.json').exists(),'Run composition target benchmark first')
 def test_local_target(self):
  p=ROOT/'build/native-composition-benchmark'
  self.assertEqual(v.verify((p/'chip.bin').read_bytes(),(p/'run.log').read_text())['result'],'PASS')
if __name__=='__main__':unittest.main()
