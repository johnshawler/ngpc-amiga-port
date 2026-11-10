"""NGR1 read-only frame-boundary snapshot schema, version 1."""
import struct
SIZE = 32876
REGISTER_NAMES = ['pc', 'sr', 'f_dash'] + ['bank%d_r%d' % (b, r) for b in range(4) for r in range(4)] + ['gpr%d' % r for r in range(4)]
def decode(blob):
    if len(blob) != SIZE:
        raise ValueError('Incorrect NGR1 size')
    magic, version, size, count = struct.unpack_from('>4sIII', blob)
    if (magic, version, size, count) != (b'NGR1', 1, SIZE, 23):
        raise ValueError('Unsupported NGR1 header')
    registers = dict(zip(REGISTER_NAMES, struct.unpack_from('>23I', blob, 16)))
    if registers['pc'] > 0xffffff or registers['sr'] > 0xffff or registers['f_dash'] > 0xff:
        raise ValueError('Register outside source width')
    return {'registers': registers, 'ram_4000_7fff': blob[108:16492], 'gpu_8000_bfff': blob[16492:]}
