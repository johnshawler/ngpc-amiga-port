# SPDX-License-Identifier: GPL-2.0-or-later
"""Private test-only resolution of borrowed world-map resource windows."""
import ctypes as C
U8=C.c_uint8;U32=C.c_uint32;P=C.c_void_p;SZ=C.c_size_t
class Span(C.Structure):_fields_=[('data',P),('size',SZ),('origin',SZ)]
class WorldResources(C.Structure):
 _fields_=[(n,Span) for n in ('layout','blocks','tile_indices','tiles','commands','colors','mono')]+[(n,U32) for n in ('layout_tag','blocks_tag','terrain_tag','tiles_tag')]+[('width',U8),('height',U8)]
class WorldState(C.Structure):_fields_=[('bytes',U8*52)]
def resolve_world(rom,offset,keep):
 """All reads below are fixture extraction, never used by native runtime."""
 def read(a,n):
  a=(a&0xffffff)-0x200000;assert 0<=a and a+n<=len(rom);return rom[a:a+n]
 def w(a):return int.from_bytes(read(a,2),'little')
 def l(a):return int.from_bytes(read(a,4),'little')
 def s(n):return n if n<32768 else n-65536
 def span(a,n,origin=0):
  b=(U8*n).from_buffer_copy(read(a,n));keep.append(b);return Span(C.addressof(b),n,origin)
 record=l(0x360000)+offset;blocks=l(record);layout=blocks+s(w(record+4))+2
 indices=blocks+s(w(record+6));tiles=l(record+8);terrain=l(record+14)
 bank=read(record+3,1)[0];command=l(0x360008)+s(w(l(0x360004)+bank*2))
 # Windows preserve signed source indexing. Resource identities retain the
 # source bank byte, but host pointers reference bounded fixture bytes only.
 return WorldResources(span(layout-32768,65568,32768),span(blocks-32768,65568,32768),
  span(indices,2+2*read(indices,1)[0]),span(tiles,16*(1+max([w(indices+2+2*i) for i in range(read(indices,1)[0])]+[0]))),span(command,43),
  span(l(0x26001c),65536),span(l(0x260018),1024),
  layout,blocks,terrain,tiles,*read(layout-2,2))
