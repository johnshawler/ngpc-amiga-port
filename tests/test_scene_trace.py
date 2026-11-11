"""Synthetic trace validation: no cartridge hashes or screenshots included."""
import copy,importlib.util,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def load(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
trace=load('scene_trace',ROOT/'reference/compare_scene_trace.py')
runner=load('scene_runner',ROOT/'reference/run.py')
class SceneTraceTests(unittest.TestCase):
 def fixture(self):
  a=[{'type':'metadata','core_sha256':'synthetic-original','input':'synthetic'},
     {'type':'frame','frame':0,'video':'synthetic','audio':'synthetic'},
     {'type':'state','after_frames':1,'state':'synthetic'}]
  b=copy.deepcopy(a);b[0]['core_sha256']='synthetic-observed';b[1].update(scene_rows=152,scene_pixels=24320)
  return a,b,copy.deepcopy(b)
 def test_matching(self):
  self.assertEqual(trace.compare(*self.fixture())['pixels'],24320)
 def test_corruptions_rejected(self):
  for row,key,value in [(0,'input','other'),(1,'video','other'),(1,'audio','other'),(1,'scene_rows',151),(1,'scene_pixels',1),(2,'state','other'),(2,'after_frames',2)]:
   a,b,c=self.fixture();b[row][key]=value;c=copy.deepcopy(b)
   with self.assertRaises(ValueError):trace.compare(a,b,c)
  a,b,c=self.fixture();c[1]['audio']='other'
  with self.assertRaises(ValueError):trace.compare(a,b,c)
 def test_ppm_conversion(self):
  words=[0xf800,0x07e0,0x001f,0xffff,0]
  for order in ['little','big']:
   raw=b''.join(v.to_bytes(2,order) for v in words)
   self.assertEqual(runner.ppm565(raw,5,1,order),b'P6\n5 1\n255\n'+bytes([255,0,0,0,255,0,0,0,255,255,255,255,0,0,0]))
  with self.assertRaises(ValueError):runner.ppm565(b'\0',1,1,'little')
if __name__=='__main__':unittest.main()
