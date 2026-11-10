import importlib.util,struct,unittest
from pathlib import Path
s=importlib.util.spec_from_file_location('state_format',Path(__file__).resolve().parents[1]/'reference/state_format.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
class StateTests(unittest.TestCase):
 def blob(self):return bytearray(struct.pack('>4sIII',b'NGR1',1,m.SIZE,23)+bytes(m.SIZE-16))
 def test_valid(self):
  r=m.decode(self.blob());self.assertEqual(len(r['registers']),23);self.assertEqual(len(r['gpu_8000_bfff']),16384)
 def test_truncated(self):
  with self.assertRaises(ValueError):m.decode(self.blob()[:-1])
 def test_header(self):
  b=self.blob();b[7]=2
  with self.assertRaises(ValueError):m.decode(b)
 def test_register_width(self):
  b=self.blob();struct.pack_into('>I',b,20,65536)
  with self.assertRaises(ValueError):m.decode(b)
if __name__=='__main__':unittest.main()
