"""Synthetic event dependency benchmark protocol checks, no game assets."""
import struct,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'native-renderer/amiga'))
import verify_event_benchmark as v
LOG='\n'.join(['cpu_model=68020','cpu_cycle_exact=true','cpu_memory_cycle_exact=true','cpu_throttle=0.0','ntsc=false','chipmem_size=4','bogomem_size=0','fastmem_size=0','chipset=aga','Model: "A1200OG".'])
class EventBenchmarkTests(unittest.TestCase):
 def fixture(self):
  b=bytearray(2097152);b[256:264]=b'NGPDISP1';struct.pack_into('>3I',b,288,4096,512,0x444f4e45)
  checks=[]
  for scroll in [251,250]:
   s=v.expected_pixels(scroll);checks.append(v.fnv(v.planar(bytes(s[(y+52)*320+x+80] for y in range(152) for x in range(160)))))
  r=[0]*128;r[:4]=[0x4e474254,8,709379,14200]
  for i in range(16):
   mode=0 if i<8 else 1 if i<12 else 2;variant=i if i<4 else 7-i if i<8 else i%4
   counts=((608,0) if mode==1 or variant==3 else (152,456)) if variant else (1,587)
   r[4+i*7:11+i*7]=[mode,variant,4,1000000,checks[int(mode==1)],*counts]
  struct.pack_into('>128I',b,4096,*r);return b
 def test_valid(self):self.assertEqual(v.verify(self.fixture(),LOG)['result'],'PASS')
 def test_rejections(self):
  original=self.fixture()
  for offset in [296,4100,4108,4128,4140]:
   b=bytearray(original);b[offset]^=0x80
   with self.assertRaises(ValueError):v.verify(b,LOG)
  with self.assertRaises(ValueError):v.verify(original,LOG+'\nfastmem_size=8')
 @unittest.skipUnless((ROOT/'build/native-event-benchmark/timing.json').exists(),'Run direct target benchmark first')
 def test_local_target(self):
  p=ROOT/'build/native-event-benchmark'
  self.assertEqual(v.verify((p/'chip.bin').read_bytes(),(p/'run.log').read_text())['result'],'PASS')
if __name__=='__main__':unittest.main()
