import importlib.util, tempfile, unittest
from pathlib import Path
P=Path(__file__).resolve().parents[1]/"tools/recovery-check.py"
spec=importlib.util.spec_from_file_location("recovery_check",P);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class RecoveryTests(unittest.TestCase):
 def test_source_integrity(self):self.assertEqual(m.check()["result"],"PASS")
 def test_wrong_cartridge_rejected(self):
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/"bad.ngc";p.write_bytes(b"not a cartridge")
   with self.assertRaises(AssertionError):m.verify_rom(p)
 def test_same_size_wrong_cartridge_rejected(self):
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/"bad.ngc";p.write_bytes(bytes(2097152))
   with self.assertRaises(AssertionError):m.verify_rom(p)
if __name__=="__main__":unittest.main()
