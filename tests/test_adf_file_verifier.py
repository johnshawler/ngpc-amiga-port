# SPDX-License-Identifier: GPL-2.0-or-later
"""Authored synthetic DD filesystems; no firmware or game assets."""
import importlib.util
import struct
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('adf_verify',ROOT/'tools/verify-adf-file.py')
V=importlib.util.module_from_spec(spec);spec.loader.exec_module(V)

def checksum(block):
    struct.pack_into('>I',block,20,0)
    struct.pack_into('>I',block,20,(-sum(struct.unpack('>128I',block)))&0xffffffff)
    return block

def header(kind,secondary,key=0,name=b'',parent=0):
    words=[0]*128;words[0]=kind;words[1]=key;words[125]=parent;words[127]=secondary&0xffffffff
    block=bytearray(struct.pack('>128I',*words));block[432]=len(name);block[433:433+len(name)]=name
    return block

def image(ffs,payload,nested=False):
    disk=bytearray(901120);disk[:4]=b'DOS'+bytes([int(ffs)])
    root=header(2,1,name=b'Test');struct.pack_into('>I',root,12,72)
    struct.pack_into('>I',root,24,11 if nested else 10)
    disk[880*512:881*512]=checksum(root)
    if nested:
        directory=header(2,2,11,b'S',880);struct.pack_into('>I',directory,24,10);disk[11*512:12*512]=checksum(directory)
    blocksize=512 if ffs else 488
    data=[payload[i:i+blocksize] for i in range(0,len(payload),blocksize)]
    pointers=list(range(20,20+len(data)))
    for offset in range(0,max(1,len(data)),72):
        at=10 if offset==0 else 12+offset//72
        part=header(2 if offset==0 else 16,-3,at,b'Game' if offset==0 else b'',11 if nested else 880)
        current=pointers[offset:offset+72];struct.pack_into('>I',part,8,len(current))
        for n,pointer in enumerate(current):struct.pack_into('>I',part,(77-n)*4,pointer)
        if offset==0:struct.pack_into('>I',part,324,len(payload));struct.pack_into('>I',part,16,pointers[0] if pointers else 0)
        if offset+72<len(data):struct.pack_into('>I',part,504,12+(offset+72)//72)
        disk[at*512:(at+1)*512]=checksum(part)
    for i,(pointer,chunk) in enumerate(zip(pointers,data)):
        if ffs:block=chunk+bytes(512-len(chunk))
        else:
            block=bytearray(512);struct.pack_into('>5I',block,0,8,10,i+1,len(chunk),pointers[i+1] if i+1<len(pointers) else 0);block[24:24+len(chunk)]=chunk;checksum(block)
        disk[pointer*512:(pointer+1)*512]=block
    return disk

class ADFVerifierTests(unittest.TestCase):
    def test_complete_files_and_extensions(self):
        for ffs in (False,True):
            for size in (0,1,488,512,513,40000):
                for nested in (False,True):
                    payload=bytes((i*73)&255 for i in range(size));disk=image(ffs,payload,nested)
                    report=V.verify(disk,'s/game' if nested else 'game',payload)
                    self.assertEqual(report['file_bytes'],size)
    def test_malformed_images_and_sources(self):
        for ffs in (False,True):
            source=bytes(range(256))*4;disk=image(ffs,source)
            with self.assertRaises(ValueError):V.verify(disk,'Game',source+b'\0')
            with self.assertRaises(ValueError):V.verify(disk[:-1],'Game',source)
            with self.assertRaises(ValueError):V.verify(disk,'missing',source)
            with self.assertRaises(ValueError):V.verify(disk,'../Game',source)
            bad=bytearray(disk);bad[10*512+325]^=1
            with self.assertRaises(ValueError):V.verify(bad,'Game',source)
            bad=bytearray(disk);bad[20*512+60]^=1
            with self.assertRaises(ValueError):V.verify(bad,'Game',source)
            bad=bytearray(disk);h=bad[10*512:11*512];struct.pack_into('>I',h,504,10);bad[10*512:11*512]=checksum(h)
            with self.assertRaises(ValueError):V.verify(bad,'Game',source)
            if not ffs:
                bad=bytearray(disk);h=bad[10*512:11*512];struct.pack_into('>I',h,77*4,23);bad[10*512:11*512]=checksum(h)
                with self.assertRaisesRegex(ValueError,'disagree'):V.verify(bad,'Game',source)
