# SPDX-License-Identifier: GPL-2.0-or-later
import ctypes as C,importlib.util,random,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('packer',ROOT/'tools/aim_pack.py');packer=importlib.util.module_from_spec(spec);spec.loader.exec_module(packer)
class Packing(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.temp=tempfile.TemporaryDirectory();so=Path(cls.temp.name)/'unpack.so'
  subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC',str(ROOT/'native-renderer/amiga/aim_unpack.c'),'-o',str(so)],check=True)
  cls.lib=C.CDLL(str(so));cls.lib.aim_unpack.argtypes=[C.c_void_p,C.c_size_t,C.c_void_p,C.c_size_t]
 @classmethod
 def tearDownClass(cls):cls.temp.cleanup()
 def decode(self,data,n):
  out=(C.c_ubyte*(n+2))(*([165]*(n+2)));source=C.create_string_buffer(data)
  result=self.lib.aim_unpack(source,len(data),C.byref(out,1),n)
  self.assertEqual(out[0],165);self.assertEqual(out[-1],165)
  if result:self.assertEqual(bytes(out),bytes([165])*(n+2))
  return result,bytes(out)[1:-1]
 def test_roundtrip(self):
  r=random.Random(0x209830)
  for n in [0,1,2,3,4,127,128,129,130,131,255,256,65535,65536,66000]:
   for data in [bytes(n),bytes(r.randrange(256) for _ in range(n)),(bytes(range(256))*(n//256+1))[:n]]:
    encoded=packer.pack(data);self.assertEqual(packer.unpack(encoded,n),data);self.assertEqual(self.decode(encoded,n),(0,data))
 def test_malformed(self):
  r=random.Random(57)
  for _ in range(20000):
   data=bytes(r.randrange(256) for _ in range(r.randrange(30)));n=r.randrange(300)
   try:expected=packer.unpack(data,n)
   except ValueError:self.assertNotEqual(self.decode(data,n)[0],0)
   else:self.assertEqual(self.decode(data,n),(0,expected))
 def test_truncations(self):
  data=b'a'*2048+bytes(range(256))*8;encoded=packer.pack(data)
  for i in range(len(encoded)):self.assertNotEqual(self.decode(encoded[:i],len(data))[0],0)

class GeneratedStartup(unittest.TestCase):
 def test_private_destination_guard(self):
  import sys
  sys.path.insert(0,str(ROOT/'tools'))
  spec=importlib.util.spec_from_file_location('exporter',ROOT/'tools/export-aim-assets.py');exporter=importlib.util.module_from_spec(spec);spec.loader.exec_module(exporter)
  with self.assertRaises(ValueError):exporter.export(b'',Path('/tmp/not-private.inc'))
 def test_array_sizeof_preserved(self):
  import sys
  sys.path.insert(0,str(ROOT/'tools'))
  spec=importlib.util.spec_from_file_location('startup',ROOT/'tools/pack-aim-startup.py');startup=importlib.util.module_from_spec(spec);spec.loader.exec_module(startup)
  out=ROOT/'build/aim-pack-public-tests';out.mkdir(parents=True,exist_ok=True)
  source=out/'example.inc';target=out/'packed.inc'
  source.write_text('static const uint8_t example[5]={1,2,3,4,5};\nstatic unsigned size(void){return sizeof example;}\n')
  startup.generate(source,target)
  test=out/'test.c';test.write_text('#include "aim_unpack.h"\n#include "packed.inc"\nint main(void){return intro_assets_unpack() || size()!=5;}\n')
  subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I'+str(ROOT/'native-renderer/amiga'),str(test),str(ROOT/'native-renderer/amiga/aim_unpack.c'),'-o',str(out/'test')],check=True)
  subprocess.run([str(out/'test')],check=True)

if __name__=='__main__':unittest.main()
