#!/usr/bin/env python3
"""Run verifier corruption checks without an emulator or private ROM."""
import struct
import unittest
from blitter_verify import BYTES, BLITS, fnv, oracle, verify


class VerifierTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.log = '\n'.join(['cpu_model=68020', 'cpu_cycle_exact=true',
            'cpu_memory_cycle_exact=true', 'blitter_cycle_exact=true',
            'immediate_blits=false', 'cpu_throttle=0.0', 'ntsc=false',
            'chipmem_size=4', 'bogomem_size=0', 'chipset=aga',
            'Model: "A1200OG".', 'fastmem_size=0'])
        cls.marker = 0x20000
        cls.pointer = cls.marker + 245920
        ram = bytearray(2097152)
        ram[cls.marker:cls.marker + 8] = b'NGPDISP1'
        struct.pack_into('>3I', ram, cls.marker + 32, cls.pointer, 512, 0x444f4e45)
        sources, masks, backdrop, expected = oracle()
        for n, data in enumerate(sources + masks + [expected, backdrop]):
            ptr = cls.marker + 100000 + n * BYTES
            ram[ptr:ptr + BYTES] = data
        r = [0] * 128
        r[:16] = [0x424c5452, 2, 709379, 14203, cls.marker + 197280, BYTES,
                  cls.marker + 221600, 0x03c0] + [cls.marker + 100000 + n * BYTES
                  for n in range(4)] + [4000000, fnv(backdrop), 16, 14]
        for sample in range(14):
            variant = sample if sample < 7 else 13 - sample
            r[16 + sample * 8:24 + sample * 8] = [variant, 16, 400000 + variant * 50000,
                fnv(expected), 0, BLITS[variant], 0x07c0 if variant == 6 else 0x03c0, BYTES]
        struct.pack_into('>128I', ram, cls.pointer, *r)
        cls.ram = bytes(ram)

    def test_valid(self):
        self.assertEqual(verify(self.ram, self.log)['result'], 'PASS')

    def test_report_corruption(self):
        # Word index, replacement: clock, calibration, pointer, DMA, metadata,
        # checksum, mismatches, number of actual operations, incomplete DMA.
        for index, value in [(0, 0), (1, 1), (2, 700000), (3, 1), (4, 0),
                             (5, 0), (6, 0), (7, 0x0380), (7, 0x07c0),
                             (8, 0), (12, 0), (13, 0), (14, 1), (15, 11),
                             (16, 1), (17, 0), (18, 0), (19, 0), (20, 1),
                             (21, 10), (22, 0x43c0), (23, 0),
                             (16 + 13 * 8 + 2, 900000)]:
            with self.subTest(index=index, value=value):
                data = bytearray(self.ram)
                struct.pack_into('>I', data, self.pointer + index * 4, value)
                with self.assertRaises(ValueError):
                    verify(data, self.log)

    def test_all_buffers_corruption(self):
        for buffer in range(6):
            with self.subTest(buffer=buffer):
                data = bytearray(self.ram)
                data[self.marker + 100000 + buffer * BYTES + 100] ^= 0x80
                with self.assertRaises(ValueError):
                    verify(data, self.log)

    def test_header_corruption(self):
        for offset, value in [(32, 0), (36, 0), (40, 10)]:
            with self.subTest(offset=offset):
                data = bytearray(self.ram)
                struct.pack_into('>I', data, self.marker + offset, value)
                with self.assertRaises(ValueError):
                    verify(data, self.log)

    def test_invalid_machine(self):
        for before, after in [('fastmem_size=0', 'fastmem_size=8'),
                              ('chipmem_size=4', 'chipmem_size=8'),
                              ('cpu_cycle_exact=true', 'cpu_cycle_exact=false'),
                              ('ntsc=false', 'ntsc=true'),
                              ('blitter_cycle_exact=true', 'blitter_cycle_exact=false'),
                              ('immediate_blits=false', 'immediate_blits=true')]:
            with self.subTest(before=before):
                with self.assertRaises(ValueError):
                    verify(self.ram, self.log.replace(before, after))
        with self.assertRaises(ValueError):
            verify(self.ram[:-1], self.log)


if __name__ == '__main__':
    unittest.main()
